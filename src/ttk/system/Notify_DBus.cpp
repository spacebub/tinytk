// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <array>
#include <bit>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "ttk/system/Env.h"
#include "ttk/system/Notify.h"
#include "ttk/system/Paths.h"
#include "ttk/system/Text.h"

namespace ttk::Notify {
    namespace {
        using Clock = std::chrono::steady_clock;

        constexpr std::string_view SERVICE = "org.freedesktop.Notifications";
        constexpr std::string_view OBJECT = "/org/freedesktop/Notifications";
        constexpr std::string_view BUS = "org.freedesktop.DBus";
        constexpr std::string_view BUS_OBJECT = "/org/freedesktop/DBus";
        constexpr std::chrono::milliseconds ANSWER{2000};

        // The specification's cap on a whole message.
        constexpr std::size_t MESSAGE_LIMIT = std::size_t{1} << 27U;

        constexpr char NATIVE_ORDER = std::endian::native == std::endian::little ? 'l' : 'B';
        constexpr std::uint8_t NO_REPLY_EXPECTED = 0x1;

        enum class Kind : std::uint8_t {
            Call = 1,
            Return = 2,
            Error = 3,
            Signal = 4,
        };

        enum class Field : std::uint8_t {
            Path = 1,
            Interface = 2,
            Member = 3,
            ErrorName = 4,
            ReplySerial = 5,
            Destination = 6,
            Sender = 7,
            Signature = 8,
        };

        // Offsets align against the start of the message, and a body starts on an 8 byte
        // boundary, so a body written or read on its own aligns the same.
        class Writer {
        public:
            struct Array {
                std::size_t length_at;
                std::size_t start;
            };

            void align(const std::size_t to) {
                bytes.resize((bytes.size() + to - 1) / to * to, '\0');
            }

            void byte(const std::uint8_t value) {
                bytes.push_back(static_cast<char>(value));
            }

            void u32(const std::uint32_t value) {
                align(4);
                bytes.append(std::bit_cast<std::array<char, 4>>(value).data(), 4);
            }

            void i32(const std::int32_t value) {
                u32(std::bit_cast<std::uint32_t>(value));
            }

            void string(const std::string_view value) {
                u32(static_cast<std::uint32_t>(value.size()));
                bytes.append(value);
                bytes.push_back('\0');
            }

            void signature(const std::string_view value) {
                byte(static_cast<std::uint8_t>(value.size()));
                bytes.append(value);
                bytes.push_back('\0');
            }

            // The length leaves out the padding up to the first element.
            Array open_array(const std::size_t element_alignment) {
                align(4);
                const std::size_t length_at = bytes.size();
                u32(0);
                align(element_alignment);

                return {length_at, bytes.size()};
            }

            void close_array(const Array &array) {
                const auto length = static_cast<std::uint32_t>(bytes.size() - array.start);
                std::memcpy(bytes.data() + array.length_at, &length, sizeof length);
            }

            std::string bytes;
        };

        class Reader {
        public:
            Reader(const std::string_view bytes, const bool swapped) : bytes(bytes), swapped(swapped) {
            }

            [[nodiscard]] bool done() const { return at >= bytes.size(); }

            bool align(const std::size_t to) {
                at = (at + to - 1) / to * to;

                return at <= bytes.size();
            }

            std::optional<std::uint8_t> byte() {
                if (at >= bytes.size()) {
                    return std::nullopt;
                }

                return static_cast<std::uint8_t>(bytes[at++]);
            }

            std::optional<std::uint32_t> u32() {
                if (!align(4) || bytes.size() - at < 4) {
                    return std::nullopt;
                }

                std::uint32_t value = 0;
                std::memcpy(&value, bytes.data() + at, sizeof value);
                at += 4;

                return swapped ? std::byteswap(value) : value;
            }

            std::optional<std::string_view> string() {
                const std::optional<std::uint32_t> length = u32();

                return length ? text(*length) : std::nullopt;
            }

            std::optional<std::string_view> signature() {
                const std::optional<std::uint8_t> length = byte();

                return length ? text(*length) : std::nullopt;
            }

        private:
            std::optional<std::string_view> text(const std::size_t length) {
                if (bytes.size() - at <= length || bytes[at + length] != '\0') {
                    return std::nullopt;
                }

                const std::string_view value = bytes.substr(at, length);
                at += length + 1;

                return value;
            }

            std::string_view bytes;
            bool swapped;
            std::size_t at{0};
        };

        struct Incoming {
            Kind kind{Kind::Signal};
            std::uint32_t reply_serial{0};
            std::string error_name{};
            std::string signature{};
            std::string body{};
            bool swapped{false};
        };

        struct Call {
            std::string_view destination;
            std::string_view object;
            std::string_view interface;
            std::string_view member;
            std::string_view signature{};
            std::string body{};
            std::uint8_t flags{0};
        };

        struct Connection {
            int socket{-1};
            std::uint32_t serial{0};

            // What has been read but not yet taken as a whole message.
            std::string inbox{};
        };

        std::mutex guard;
        Connection bus;

        void say(std::string *why, const std::string &text) {
            if (why != nullptr) {
                *why = text;
            }
        }

        // The bus drops a sender over a string that is not UTF-8 or holds a NUL, so those
        // are refused before they are sent.
        bool is_text(const std::string_view value) {
            constexpr std::uint32_t least[] = {0, 0x80, 0x800, 0x10000};

            for (std::size_t i = 0; i < value.size();) {
                const auto lead = static_cast<unsigned char>(value[i]);
                std::size_t extra = 0;
                std::uint32_t point = 0;

                if (lead == 0) {
                    return false;
                }

                if (lead < 0x80) {
                    ++i;
                    continue;
                }

                if ((lead & 0xE0U) == 0xC0U) {
                    extra = 1;
                    point = lead & 0x1FU;
                } else if ((lead & 0xF0U) == 0xE0U) {
                    extra = 2;
                    point = lead & 0x0FU;
                } else if ((lead & 0xF8U) == 0xF0U) {
                    extra = 3;
                    point = lead & 0x07U;
                } else {
                    return false;
                }

                if (value.size() - i <= extra) {
                    return false;
                }

                for (std::size_t k = 1; k <= extra; ++k) {
                    const auto next = static_cast<unsigned char>(value[i + k]);

                    if ((next & 0xC0U) != 0x80U) {
                        return false;
                    }

                    point = point << 6U | (next & 0x3FU);
                }

                if (point < least[extra] || point > 0x10FFFF || (point >= 0xD800 && point <= 0xDFFF)) {
                    return false;
                }

                i += extra + 1;
            }

            return true;
        }

        std::string encode(const Call &call, const std::uint32_t serial) {
            Writer out;

            out.byte(static_cast<std::uint8_t>(NATIVE_ORDER));
            out.byte(static_cast<std::uint8_t>(Kind::Call));
            out.byte(call.flags);
            out.byte(1);
            out.u32(static_cast<std::uint32_t>(call.body.size()));
            out.u32(serial);

            const auto field = [&out](const Field code, const char type, const std::string_view value) {
                out.align(8);
                out.byte(static_cast<std::uint8_t>(code));
                out.signature({&type, 1});

                if (type == 'g') {
                    out.signature(value);
                } else {
                    out.string(value);
                }
            };

            const Writer::Array fields = out.open_array(8);

            field(Field::Path, 'o', call.object);
            field(Field::Destination, 's', call.destination);
            field(Field::Interface, 's', call.interface);
            field(Field::Member, 's', call.member);

            if (!call.signature.empty()) {
                field(Field::Signature, 'g', call.signature);
            }

            out.close_array(fields);
            out.align(8);
            out.bytes += call.body;

            return std::move(out.bytes);
        }

        enum class Taken : std::uint8_t {
            Whole,
            Partial,
            Broken,
        };

        // Takes one whole message off the front of the inbox.
        Taken take(Incoming &into) {
            constexpr std::size_t FIXED = 16;

            if (bus.inbox.size() < FIXED) {
                return Taken::Partial;
            }

            const char order = bus.inbox[0];

            if (order != 'l' && order != 'B') {
                return Taken::Broken;
            }

            into = {};
            into.swapped = order != NATIVE_ORDER;

            Reader fixed({bus.inbox.data(), FIXED}, into.swapped);
            (void) fixed.u32();
            const std::uint32_t body_length = fixed.u32().value_or(0);
            (void) fixed.u32();
            const std::uint32_t fields_length = fixed.u32().value_or(0);

            if (body_length > MESSAGE_LIMIT || fields_length > MESSAGE_LIMIT) {
                return Taken::Broken;
            }

            const std::size_t header_end = (FIXED + fields_length + 7) / 8 * 8;
            const std::size_t total = header_end + body_length;

            if (total > MESSAGE_LIMIT) {
                return Taken::Broken;
            }

            if (bus.inbox.size() < total) {
                return Taken::Partial;
            }

            into.kind = static_cast<Kind>(bus.inbox[1]);

            Reader fields({bus.inbox.data(), FIXED + fields_length}, into.swapped);
            (void) fields.u32();
            (void) fields.u32();
            (void) fields.u32();
            (void) fields.u32();

            while (!fields.done()) {
                const std::optional<std::uint8_t> code = fields.align(8) ? fields.byte() : std::nullopt;
                const std::optional<std::string_view> type = code ? fields.signature() : std::nullopt;

                if (!type) {
                    return Taken::Broken;
                }

                if (*type == "u") {
                    const std::optional<std::uint32_t> value = fields.u32();

                    if (!value) {
                        return Taken::Broken;
                    }

                    if (*code == static_cast<std::uint8_t>(Field::ReplySerial)) {
                        into.reply_serial = *value;
                    }
                } else if (*type == "s" || *type == "o" || *type == "g") {
                    const std::optional<std::string_view> value = *type == "g" ? fields.signature() : fields.string();

                    if (!value) {
                        return Taken::Broken;
                    }

                    if (*code == static_cast<std::uint8_t>(Field::ErrorName)) {
                        into.error_name = *value;
                    } else if (*code == static_cast<std::uint8_t>(Field::Signature)) {
                        into.signature = *value;
                    }
                } else {
                    return Taken::Broken;
                }
            }

            into.body = bus.inbox.substr(header_end, body_length);
            bus.inbox.erase(0, total);

            return Taken::Whole;
        }

        void drop() {
            if (bus.socket >= 0) {
                ::close(bus.socket);
            }

            bus = {};
        }

        bool send_all(const std::string_view bytes) {
            std::size_t sent = 0;

            while (sent < bytes.size()) {
                const ssize_t wrote = ::send(bus.socket, bytes.data() + sent, bytes.size() - sent, MSG_NOSIGNAL);

                if (wrote < 0 && errno == EINTR) {
                    continue;
                }

                if (wrote <= 0) {
                    return false;
                }

                sent += static_cast<std::size_t>(wrote);
            }

            return true;
        }

        // Reads what has arrived, waiting until the deadline for anything at all.
        bool receive(const Clock::time_point deadline) {
            const auto left = std::chrono::ceil<std::chrono::milliseconds>(deadline - Clock::now());
            pollfd asked{bus.socket, POLLIN, 0};

            if (::poll(&asked, 1, static_cast<int>(std::max<std::int64_t>(left.count(), 0))) <= 0) {
                return false;
            }

            char chunk[4096];
            // NOLINTNEXTLINE(clang-analyzer-unix.BlockInCriticalSection): poll said it is readable, and show() blocks by contract.
            const ssize_t got = ::recv(bus.socket, chunk, sizeof chunk, 0);

            if (got <= 0) {
                return false;
            }

            bus.inbox.append(chunk, static_cast<std::size_t>(got));

            return true;
        }

        // Signals and stale replies are thrown away before they pile up, and a bus that has
        // gone away is noticed here rather than on the next write.
        void drain() {
            while (bus.socket >= 0) {
                pollfd asked{bus.socket, POLLIN, 0};

                if (::poll(&asked, 1, 0) <= 0) {
                    break;
                }

                if (!receive(Clock::now())) {
                    drop();

                    return;
                }
            }

            Incoming unasked;
            Taken taken = Taken::Whole;

            while (bus.socket >= 0 && taken == Taken::Whole) {
                taken = take(unasked);
            }

            if (taken == Taken::Broken) {
                drop();
            }
        }

        // Zero is not a serial.
        std::uint32_t next_serial() {
            if (++bus.serial == 0) {
                ++bus.serial;
            }

            return bus.serial;
        }

        std::optional<Incoming> call(const Call &request, std::string *why) {
            const std::uint32_t serial = next_serial();

            if (!send_all(encode(request, serial))) {
                drop();
                say(why, "the session bus went away");

                return std::nullopt;
            }

            const Clock::time_point deadline = Clock::now() + ANSWER;
            Incoming reply;

            while (true) {
                const Taken taken = take(reply);

                if (taken == Taken::Broken) {
                    drop();
                    say(why, "the session bus sent a malformed message");

                    return std::nullopt;
                }

                if (taken == Taken::Whole) {
                    if ((reply.kind == Kind::Return || reply.kind == Kind::Error) && reply.reply_serial == serial) {
                        return reply;
                    }

                    continue;
                }

                if (!receive(deadline)) {
                    drop();
                    say(why, "the notification server did not answer");

                    return std::nullopt;
                }
            }
        }

        std::string describe(const Incoming &error) {
            if (error.signature.starts_with('s')) {
                Reader body(error.body, error.swapped);

                if (const std::optional<std::string_view> text = body.string(); text && !text->empty()) {
                    return std::string(*text);
                }
            }

            return error.error_name.empty() ? "the call failed" : error.error_name;
        }

        bool connect_socket(const std::string_view path, const bool abstract) {
            sockaddr_un address{};
            address.sun_family = AF_UNIX;

            const std::size_t skip = abstract ? 1 : 0;

            if (path.empty() || path.size() + skip >= sizeof address.sun_path) {
                return false;
            }

            std::memcpy(address.sun_path + skip, path.data(), path.size());

            // An abstract name is counted exactly and is not NUL terminated.
            const auto length = static_cast<socklen_t>(offsetof(sockaddr_un, sun_path) + skip + path.size()
                                                       + (abstract ? 0 : 1));

            bus.socket = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

            if (bus.socket < 0) {
                return false;
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): the sockets API takes the generic address.
            if (::connect(bus.socket, reinterpret_cast<const sockaddr *>(&address), length) != 0) {
                drop();

                return false;
            }

            return true;
        }

        std::string unescape(const std::string_view value) {
            std::string out;

            for (std::size_t i = 0; i < value.size(); ++i) {
                unsigned char byte = 0;

                if (value[i] == '%' && value.size() - i > 2
                    && std::from_chars(value.data() + i + 1, value.data() + i + 3, byte, 16).ptr == value.data() + i + 3) {
                    out.push_back(static_cast<char>(byte));
                    i += 2;
                } else {
                    out.push_back(value[i]);
                }
            }

            return out;
        }

        // Tries each `unix:` entry of a bus address in turn. Transports other than a local
        // socket are left alone.
        bool open_address(const std::string &addresses) {
            for (const std::string &entry : Text::split(addresses, ';')) {
                if (!entry.starts_with("unix:")) {
                    continue;
                }

                for (const std::string &pair : Text::split(std::string_view(entry).substr(5), ',')) {
                    const std::size_t equals = pair.find('=');

                    if (equals == std::string::npos) {
                        continue;
                    }

                    const std::string_view key = std::string_view(pair).substr(0, equals);
                    const std::string value = unescape(std::string_view(pair).substr(equals + 1));

#ifdef __linux__
                    if (key == "abstract" && connect_socket(value, true)) {
                        return true;
                    }
#endif

                    if (key == "path" && connect_socket(value, false)) {
                        return true;
                    }
                }
            }

            return false;
        }

        bool authenticate() {
            std::string hello("\0AUTH EXTERNAL ", 15);

            for (const char digit : std::to_string(::getuid())) {
                constexpr std::string_view HEX = "0123456789abcdef";
                const auto code = static_cast<unsigned char>(digit);
                hello.push_back(HEX[code >> 4U]);
                hello.push_back(HEX[code & 0xFU]);
            }

            hello += "\r\n";

            if (!send_all(hello)) {
                return false;
            }

            const Clock::time_point deadline = Clock::now() + ANSWER;
            std::size_t end = std::string::npos;

            while ((end = bus.inbox.find("\r\n")) == std::string::npos) {
                if (!receive(deadline)) {
                    return false;
                }
            }

            const bool accepted = bus.inbox.starts_with("OK ");
            bus.inbox.erase(0, end + 2);

            return accepted && send_all("BEGIN\r\n");
        }

        bool connect(std::string *why) {
            drain();

            if (bus.socket >= 0) {
                return true;
            }

            std::string address = Env::get("DBUS_SESSION_BUS_ADDRESS");

            if (address.empty()) {
                if (const std::string runtime = Env::get("XDG_RUNTIME_DIR"); !runtime.empty()) {
                    address = "unix:path=" + runtime + "/bus";
                }
            }

            if (address.empty() || !open_address(address)) {
                say(why, "there is no session bus");

                return false;
            }

            if (!authenticate()) {
                drop();
                say(why, "the session bus refused the connection");

                return false;
            }

            const std::optional<Incoming> welcome = call({BUS, BUS_OBJECT, BUS, "Hello"}, why);

            if (!welcome) {
                return false;
            }

            if (welcome->kind == Kind::Error) {
                drop();
                say(why, describe(*welcome));

                return false;
            }

            return true;
        }

        std::string notify_body(const Message &message, const Id replaces) {
            Writer out;

            out.string(Paths::application());
            out.u32(replaces);
            out.string(message.icon);
            out.string(message.title);
            out.string(message.body);
            out.close_array(out.open_array(4));

            const Writer::Array hints = out.open_array(8);
            out.align(8);
            out.string("urgency");
            out.signature("y");
            out.byte(static_cast<std::uint8_t>(message.urgency));
            out.close_array(hints);

            out.i32(message.timeout);

            return std::move(out.bytes);
        }
    }

    Id show(const Message &message, const Id replaces, std::string *why) {
        for (const std::string *text : {&Paths::application(), &message.icon, &message.title, &message.body}) {
            if (!is_text(*text)) {
                say(why, "the notification is not valid UTF-8");

                return 0;
            }
        }

        const std::scoped_lock lock(guard);

        if (!connect(why)) {
            return 0;
        }

        const std::optional<Incoming> reply = call({
            SERVICE, OBJECT, SERVICE, "Notify", "susssasa{sv}i", notify_body(message, replaces),
        }, why);

        if (!reply) {
            return 0;
        }

        if (reply->kind == Kind::Error) {
            say(why, describe(*reply));

            return 0;
        }

        Reader body(reply->body, reply->swapped);
        const std::optional<std::uint32_t> id = reply->signature == "u" ? body.u32() : std::nullopt;

        if (!id) {
            say(why, "the notification server answered with no id");

            return 0;
        }

        return *id;
    }

    void close(const Id id) {
        if (id == 0) {
            return;
        }

        const std::scoped_lock lock(guard);

        if (!connect(nullptr)) {
            return;
        }

        Writer body;
        body.u32(id);

        if (!send_all(encode({SERVICE, OBJECT, SERVICE, "CloseNotification", "u", std::move(body.bytes),
                              NO_REPLY_EXPECTED}, next_serial()))) {
            drop();
        }
    }

    void stop() {
        const std::scoped_lock lock(guard);

        drop();
    }
}
