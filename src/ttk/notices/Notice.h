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
    //! Severity of a \ref Notice, which sets the colour of its toast and how long \ref Notifier keeps it shown.
    enum class Severity : std::uint8_t {
        //! Neutral information, shown in the accent colour.
        Info,
        //! A completed action, shown in the success colour.
        Success,
        //! Something that needs attention, shown in the warning colour.
        Warning,
        //! A failure, shown in the danger colour. \ref Notifier keeps it until it is dismissed.
        Error,
    };

    //! In-window message shown as a toast by \ref Toasts.
    struct Notice {
        //! Identifier, unique within the \ref Notifier that posted the notice.
        int id = 0;

        //! Severity, which picks the colour of the toast.
        Severity severity = Severity::Info;

        //! Bold first line. Empty for none.
        std::string title;

        //! Message text, wrapped to the width of the toast.
        std::string body;

        //! Time in milliseconds the toast counts down before it closes itself. Zero or less keeps it until it is
        //! dismissed.
        int duration = 0;
    };
}


#endif //TTK_NOTICES_NOTICE_H
