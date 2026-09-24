// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <initializer_list>
#include <memory>
#include <mutex>

#include <dbus/dbus.h>

#include "ttk/system/Notify.h"
#include "ttk/system/Paths.h"

namespace ttk::Notify {
    namespace {
        constexpr const char *SERVICE = "org.freedesktop.Notifications";
        constexpr const char *OBJECT = "/org/freedesktop/Notifications";
        constexpr int ANSWER_MILLIS = 2000;

        struct Error {
            DBusError value{};

            Error() { dbus_error_init(&value); }
            ~Error() { dbus_error_free(&value); }

            Error(const Error &) = delete;
            Error &operator=(const Error &) = delete;
            Error(Error &&) = delete;
            Error &operator=(Error &&) = delete;

            [[nodiscard]] std::string text(const char *otherwise) const {
                return dbus_error_is_set(&value) != 0 && value.message != nullptr ? value.message : otherwise;
            }
        };

        struct Unref {
            void operator()(DBusMessage *message) const { dbus_message_unref(message); }
        };

        using Owned = std::unique_ptr<DBusMessage, Unref>;

        std::mutex guard;
        DBusConnection *bus = nullptr;

        void say(std::string *why, const std::string &text) {
            if (why != nullptr) {
                *why = text;
            }
        }

        void drop() {
            if (bus != nullptr) {
                dbus_connection_close(bus);
                dbus_connection_unref(bus);
                bus = nullptr;
            }
        }

        // The server's signals land in the queue whether or not anyone listens, and nothing
        // here dispatches, so they are thrown away before they pile up.
        void drain() {
            while (DBusMessage *unasked = dbus_connection_pop_message(bus)) {
                dbus_message_unref(unasked);
            }
        }

        // Private, so draining it cannot starve another user of the shared connection.
        bool connect(std::string *why) {
            if (bus != nullptr && dbus_connection_get_is_connected(bus) != 0) {
                return true;
            }

            drop();

            Error error;
            bus = dbus_bus_get_private(DBUS_BUS_SESSION, &error.value);

            if (bus == nullptr) {
                say(why, error.text("there is no session bus"));

                return false;
            }

            // Otherwise libdbus calls _exit when the bus goes away.
            dbus_connection_set_exit_on_disconnect(bus, FALSE);

            return true;
        }

        bool add_string(DBusMessageIter *into, const std::string &value) {
            const char *raw = value.c_str();

            return dbus_message_iter_append_basic(into, DBUS_TYPE_STRING, static_cast<const void *>(&raw)) != 0;
        }

        bool add_urgency(DBusMessageIter *into, const Urgency urgency) {
            const char *key = "urgency";
            const auto level = static_cast<unsigned char>(urgency);

            DBusMessageIter hints;
            DBusMessageIter entry;
            DBusMessageIter variant;

            return dbus_message_iter_open_container(into, DBUS_TYPE_ARRAY, "{sv}", &hints) != 0
                && dbus_message_iter_open_container(&hints, DBUS_TYPE_DICT_ENTRY, nullptr, &entry) != 0
                && dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, static_cast<const void *>(&key)) != 0
                && dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "y", &variant) != 0
                && dbus_message_iter_append_basic(&variant, DBUS_TYPE_BYTE, &level) != 0
                && dbus_message_iter_close_container(&entry, &variant) != 0
                && dbus_message_iter_close_container(&hints, &entry) != 0
                && dbus_message_iter_close_container(into, &hints) != 0;
        }

        bool add_arguments(DBusMessage *call, const Message &message, const dbus_uint32_t replaces) {
            const dbus_int32_t timeout = message.timeout;

            DBusMessageIter into;
            DBusMessageIter actions;

            dbus_message_iter_init_append(call, &into);

            return add_string(&into, Paths::application())
                && dbus_message_iter_append_basic(&into, DBUS_TYPE_UINT32, &replaces) != 0
                && add_string(&into, message.icon)
                && add_string(&into, message.title)
                && add_string(&into, message.body)
                && dbus_message_iter_open_container(&into, DBUS_TYPE_ARRAY, "s", &actions) != 0
                && dbus_message_iter_close_container(&into, &actions) != 0
                && add_urgency(&into, message.urgency)
                && dbus_message_iter_append_basic(&into, DBUS_TYPE_INT32, &timeout) != 0;
        }
    }

    Id show(const Message &message, const Id replaces, std::string *why) {
        // libdbus aborts on a string that is not UTF-8 rather than refusing it.
        for (const std::string *text : {&Paths::application(), &message.icon, &message.title, &message.body}) {
            if (dbus_validate_utf8(text->c_str(), nullptr) == 0) {
                say(why, "the notification is not valid UTF-8");

                return 0;
            }
        }

        const std::scoped_lock lock(guard);

        if (!connect(why)) {
            return 0;
        }

        const Owned call(dbus_message_new_method_call(SERVICE, OBJECT, SERVICE, "Notify"));

        if (!call || !add_arguments(call.get(), message, replaces)) {
            say(why, "out of memory");

            return 0;
        }

        Error error;
        const Owned reply(dbus_connection_send_with_reply_and_block(bus, call.get(), ANSWER_MILLIS, &error.value));

        drain();

        dbus_uint32_t id = 0;

        if (!reply || dbus_message_get_args(reply.get(), &error.value,
                                            DBUS_TYPE_UINT32, &id, DBUS_TYPE_INVALID) == 0) {
            say(why, error.text("the notification server did not answer"));

            return 0;
        }

        return id;
    }

    void close(const Id id) {
        if (id == 0) {
            return;
        }

        const std::scoped_lock lock(guard);

        if (!connect(nullptr)) {
            return;
        }

        const Owned call(dbus_message_new_method_call(SERVICE, OBJECT, SERVICE, "CloseNotification"));
        const dbus_uint32_t which = id;

        if (call && dbus_message_append_args(call.get(), DBUS_TYPE_UINT32, &which, DBUS_TYPE_INVALID) != 0) {
            dbus_connection_send(bus, call.get(), nullptr);
            dbus_connection_flush(bus);
        }

        drain();
    }

    void stop() {
        const std::scoped_lock lock(guard);

        drop();
    }
}
