// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_NOTICES_NOTICE_H
#define TTK_NOTICES_NOTICE_H


#include <cstdint>
#include <string>

namespace ttk {
    // How loud a notice is, and how long it stays.
    enum class Severity : std::uint8_t {
        Info,
        Success,
        Warning,
        Error,
    };

    struct Notice {
        int id = 0;
        Severity severity = Severity::Info;
        std::string title;
        std::string body;
        int duration = 0;
    };
}


#endif //TTK_NOTICES_NOTICE_H
