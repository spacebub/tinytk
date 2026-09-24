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

namespace ttk::Notify {
    // Zero means none.
    using Id = std::uint32_t;

    enum class Urgency : std::uint8_t {
        Low,
        Normal,

        // Stays until dismissed, and is shown through do not disturb.
        Critical,
    };

    struct Message {
        std::string title{};
        std::string body{};

        // An icon theme name or a file.
        // Empty uses the application's own.
        std::string icon{};

        Urgency urgency{Urgency::Normal};

        // Milliseconds. Negative leaves it to the desktop, zero keeps it until dismissed.
        int timeout{-1};
    };

    // Sent under Paths::application(). Blocks until the desktop answers. A non zero
    // `replaces` updates that one in place.
    Id show(const Message &message, Id replaces = 0, std::string *why = nullptr);

    void close(Id id);

    // Drops the bus connection.
    void stop();
}


#endif //TTK_SYSTEM_NOTIFY_H
