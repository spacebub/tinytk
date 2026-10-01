// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_SYSTEM_NOTIFY_H
#define TTK_SYSTEM_NOTIFY_H


#include <cstdint>
#include <string>

//! Desktop notifications sent to the freedesktop notification server over the D-Bus session bus.
//!
//! On Windows and macOS every call is a stub: \ref show() fails and the others do nothing.
namespace ttk::Notify {
    //! Identifier the notification server gives a notification. Zero means none.
    using Id = std::uint32_t;

    //! Urgency level sent with a notification.
    enum class Urgency : std::uint8_t {
        //! Low urgency.
        Low,
        //! Normal urgency.
        Normal,
        //! Critical urgency. Notification servers keep it until it is dismissed and show it through do not disturb.
        Critical,
    };

    //! Content of a desktop notification. Every string must be valid UTF-8 without NUL bytes.
    struct Message {
        //! Summary line.
        std::string title{};

        //! Body text below the summary.
        std::string body{};

        //! Icon theme name or file path of the icon. Empty sends no icon and leaves the choice to the server.
        std::string icon{};

        //! Urgency level.
        Urgency urgency{Urgency::Normal};

        //! Time in milliseconds before the notification expires. Negative leaves it to the server, zero keeps it
        //! until it is dismissed.
        int timeout{-1};
    };

    //! Shows `message` as sent by \ref Paths::application() and returns its id, or 0 on failure.
    //!
    //! A non-zero `replaces` updates that notification in place instead of showing a new one. Blocks until the
    //! server answers, for up to two seconds. On failure stores a message in `why` when it is not null, including
    //! when a string is not valid UTF-8, no session bus is found, or the server does not answer. The bus is
    //! connected on first use and again after it is lost. Safe to call from any thread.
    Id show(const Message &message, Id replaces = 0, std::string *why = nullptr);

    //! Asks the server to close the notification `id` without waiting for an answer. Does nothing when `id` is 0
    //! or no session bus is found.
    void close(Id id);

    //! Closes the connection to the session bus. A later \ref show() or \ref close() connects again.
    void stop();
}


#endif //TTK_SYSTEM_NOTIFY_H
