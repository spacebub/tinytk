// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_SYSTEM_ENV_H
#define TTK_SYSTEM_ENV_H


#include <string>

namespace ttk {
    namespace Env {

        // Empty when unset.
        [[nodiscard]] std::string get(const char *name);

        // On Windows both the C runtime and the Win32 environment are set, since libraries read either.
        void set(const char *name, const char *value);

    }
}


#endif //TTK_SYSTEM_ENV_H
