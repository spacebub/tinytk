// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "ttk/system/Notify.h"

namespace ttk::Notify {
    Id show(const Message & /*unused*/, const Id /*unused*/, std::string *why) {
        if (why != nullptr) {
            *why = "notifications are not supported here";
        }

        return 0;
    }

    void close(const Id /*unused*/) {
    }

    void stop() {
    }
}
