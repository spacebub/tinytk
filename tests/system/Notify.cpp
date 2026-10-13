// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <array>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <thread>

#include <gtest/gtest.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "support/Sandbox.h"
#include "ttk/system/Env.h"
#include "ttk/system/Notify.h"
#include "ttk/system/Paths.h"

namespace {
    namespace fs = std::filesystem;
    namespace Notify = ttk::Notify;

    constexpr const char *NO_BUS = "there is no session bus";
    constexpr const char *NOT_TEXT = "the notification is not valid UTF-8";
    constexpr const char *REFUSED = "the session bus refused the connection";

    // Every case points the bus address at the sandbox, so no message can reach the real desktop.
    class NotifyOffline : public ::testing::Test {
    protected:
        void SetUp() override {
            Notify::stop();
            _application = ttk::Paths::application();
            _bus = tests::Sandbox::scratch("notify-bus") / "absent";
            ttk::Env::set("DBUS_SESSION_BUS_ADDRESS", ("unix:path=" + _bus.string()).c_str());
            ttk::Env::set("XDG_RUNTIME_DIR", tests::Sandbox::scratch("notify-runtime").c_str());
        }

        void TearDown() override {
            Notify::stop();
            ttk::Paths::set_application(_application);
        }

        [[nodiscard]] const fs::path &bus() const { return _bus; }

    private:
        std::string _application;
        fs::path _bus;
    };

    // Answers the first authentication line with a rejection and hangs up.
    class RefusingBus {
    public:
        explicit RefusingBus(const fs::path &path) : _socket(::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) {
            sockaddr_un address{};
            address.sun_family = AF_UNIX;
            std::strncpy(address.sun_path, path.c_str(), sizeof(address.sun_path) - 1);

            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): the sockets API takes the generic address.
            _ready = _socket >= 0 && ::bind(_socket, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == 0
                     && ::listen(_socket, 1) == 0;

            if (_ready) {
                _thread = std::thread([this] { serve(); });
            }
        }

        ~RefusingBus() {
            if (_thread.joinable()) {
                _thread.join();
            }

            if (_socket >= 0) {
                ::close(_socket);
            }
        }

        RefusingBus(const RefusingBus &) = delete;
        RefusingBus &operator=(const RefusingBus &) = delete;
        RefusingBus(RefusingBus &&) = delete;
        RefusingBus &operator=(RefusingBus &&) = delete;

        [[nodiscard]] bool ready() const { return _ready; }

    private:
        void serve() const {
            const int peer = ::accept(_socket, nullptr, nullptr);

            if (peer < 0) {
                return;
            }

            std::string heard;
            std::array<char, 256> buffer{};

            while (!heard.contains("\r\n")) {
                const ssize_t got = ::read(peer, buffer.data(), buffer.size());

                if (got <= 0) {
                    break;
                }

                heard.append(buffer.data(), static_cast<std::size_t>(got));
            }

            constexpr std::string_view ANSWER = "REJECTED EXTERNAL\r\n";

            [[maybe_unused]] const ssize_t wrote = ::write(peer, ANSWER.data(), ANSWER.size());

            ::close(peer);
        }

        int _socket{-1};
        bool _ready{false};
        std::thread _thread;
    };
}

TEST(NotifyMessage, DefaultsToNormalUrgencyAndServerTimeout) {
    const Notify::Message message;

    EXPECT_EQ(message.title, "");
    EXPECT_EQ(message.body, "");
    EXPECT_EQ(message.icon, "");
    EXPECT_EQ(message.urgency, Notify::Urgency::Normal);
    EXPECT_EQ(message.timeout, -1);
}

TEST(NotifyMessage, UrgencyLevelsAreOrdered) {
    EXPECT_LT(static_cast<int>(Notify::Urgency::Low), static_cast<int>(Notify::Urgency::Normal));
    EXPECT_LT(static_cast<int>(Notify::Urgency::Normal), static_cast<int>(Notify::Urgency::Critical));
    EXPECT_EQ(static_cast<int>(Notify::Urgency::Low), 0) << "the specification's byte values";
    EXPECT_EQ(static_cast<int>(Notify::Urgency::Critical), 2);
}

TEST_F(NotifyOffline, MissingBusFails) {
    std::string why;

    EXPECT_EQ(Notify::show({.title = "title", .body = "body"}, 0, &why), 0U);
    EXPECT_EQ(why, NO_BUS);
}

TEST_F(NotifyOffline, MissingBusFailsWithoutAReasonPointer) {
    EXPECT_EQ(Notify::show({.title = "title"}), 0U);
}

TEST_F(NotifyOffline, AddressWithoutUnixEntriesFails) {
    ttk::Env::set("DBUS_SESSION_BUS_ADDRESS", "tcp:host=ttk.invalid,port=1;nonsense");

    std::string why;

    EXPECT_EQ(Notify::show({.title = "title"}, 0, &why), 0U);
    EXPECT_EQ(why, NO_BUS);
}

TEST_F(NotifyOffline, EmptyAddressFallsBackToTheRuntimeDirectory) {
    ttk::Env::set("DBUS_SESSION_BUS_ADDRESS", "");

    std::string why;

    EXPECT_EQ(Notify::show({.title = "title"}, 0, &why), 0U);
    EXPECT_EQ(why, NO_BUS) << "the sandboxed runtime directory holds no bus";
}

TEST_F(NotifyOffline, NoAddressAndNoRuntimeDirectoryFails) {
    ttk::Env::set("DBUS_SESSION_BUS_ADDRESS", "");
    ttk::Env::set("XDG_RUNTIME_DIR", "");

    std::string why;

    EXPECT_EQ(Notify::show({.title = "title"}, 0, &why), 0U);
    EXPECT_EQ(why, NO_BUS);
}

TEST_F(NotifyOffline, ValidUtf8PassesTheTextCheck) {
    std::string why;
    const Notify::Message message{
            .title = "\xE6\x97\xA5\xE6\x9C\xAC",
            .body = "\xC3\xA4 \xF0\x9F\x98\x80 \xF4\x8F\xBF\xBF",
            .icon = "dialog-information",
            .urgency = Notify::Urgency::Critical,
            .timeout = 0,
    };

    EXPECT_EQ(Notify::show(message, 7, &why), 0U);
    EXPECT_EQ(why, NO_BUS) << "the text was accepted and the bus was looked for";
}

TEST_F(NotifyOffline, EmptyStringsPassTheTextCheck) {
    std::string why;

    EXPECT_EQ(Notify::show({}, 0, &why), 0U);
    EXPECT_EQ(why, NO_BUS);
}

TEST_F(NotifyOffline, InvalidTitleIsRejected) {
    std::string why;

    EXPECT_EQ(Notify::show({.title = "\xFF"}, 0, &why), 0U);
    EXPECT_EQ(why, NOT_TEXT);
}

TEST_F(NotifyOffline, InvalidBodyIsRejected) {
    std::string why;

    EXPECT_EQ(Notify::show({.title = "ok", .body = "bad \xC3"}, 0, &why), 0U);
    EXPECT_EQ(why, NOT_TEXT);
}

TEST_F(NotifyOffline, InvalidIconIsRejected) {
    std::string why;

    EXPECT_EQ(Notify::show({.title = "ok", .icon = "\x80icon"}, 0, &why), 0U);
    EXPECT_EQ(why, NOT_TEXT);
}

TEST_F(NotifyOffline, InvalidApplicationNameIsRejected) {
    ttk::Paths::set_application("app\xFE");

    std::string why;

    EXPECT_EQ(Notify::show({.title = "ok"}, 0, &why), 0U);
    EXPECT_EQ(why, NOT_TEXT);
}

TEST_F(NotifyOffline, EmbeddedNulIsRejected) {
    std::string why;

    EXPECT_EQ(Notify::show({.title = std::string("a\0b", 3)}, 0, &why), 0U);
    EXPECT_EQ(why, NOT_TEXT);
}

TEST_F(NotifyOffline, OverlongEncodingIsRejected) {
    std::string why;

    EXPECT_EQ(Notify::show({.title = "\xC0\xAF"}, 0, &why), 0U);
    EXPECT_EQ(why, NOT_TEXT);

    EXPECT_EQ(Notify::show({.title = "\xE0\x80\xAF"}, 0, &why), 0U);
    EXPECT_EQ(why, NOT_TEXT);
}

TEST_F(NotifyOffline, SurrogateIsRejected) {
    std::string why;

    EXPECT_EQ(Notify::show({.title = "\xED\xA0\x80"}, 0, &why), 0U);
    EXPECT_EQ(why, NOT_TEXT);
}

TEST_F(NotifyOffline, BeyondUnicodeIsRejected) {
    std::string why;

    EXPECT_EQ(Notify::show({.title = "\xF4\x90\x80\x80"}, 0, &why), 0U);
    EXPECT_EQ(why, NOT_TEXT);

    EXPECT_EQ(Notify::show({.title = "\xF8\x88\x80\x80\x80"}, 0, &why), 0U);
    EXPECT_EQ(why, NOT_TEXT);
}

TEST_F(NotifyOffline, TruncatedSequenceIsRejected) {
    std::string why;

    EXPECT_EQ(Notify::show({.title = "\xE6\x97"}, 0, &why), 0U);
    EXPECT_EQ(why, NOT_TEXT);

    EXPECT_EQ(Notify::show({.title = "\xE6\x97x"}, 0, &why), 0U);
    EXPECT_EQ(why, NOT_TEXT);
}

TEST_F(NotifyOffline, RefusedAuthenticationFails) {
    const RefusingBus server(bus());

    ASSERT_TRUE(server.ready());

    std::string why;

    EXPECT_EQ(Notify::show({.title = "title"}, 0, &why), 0U);
    EXPECT_EQ(why, REFUSED);
}

TEST_F(NotifyOffline, CloseZeroDoesNothing) {
    Notify::close(0);

    SUCCEED();
}

TEST_F(NotifyOffline, CloseWithoutABusDoesNothing) {
    Notify::close(42);

    std::string why;

    EXPECT_EQ(Notify::show({.title = "after"}, 0, &why), 0U);
    EXPECT_EQ(why, NO_BUS);
}

TEST_F(NotifyOffline, StopIsIdempotent) {
    Notify::stop();
    Notify::stop();

    SUCCEED();
}

TEST_F(NotifyOffline, ShowAfterStopLooksForTheBusAgain) {
    std::string why;

    EXPECT_EQ(Notify::show({.title = "one"}, 0, &why), 0U);
    Notify::stop();

    why.clear();

    EXPECT_EQ(Notify::show({.title = "two"}, 0, &why), 0U);
    EXPECT_EQ(why, NO_BUS);
}

TEST_F(NotifyOffline, ShowIsSafeFromAnotherThread) {
    std::string why;
    Notify::Id id = 1;

    std::thread([&] { id = Notify::show({.title = "threaded"}, 0, &why); }).join();

    EXPECT_EQ(id, 0U);
    EXPECT_EQ(why, NO_BUS);
}
