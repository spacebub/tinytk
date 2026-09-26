// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_UTIL_DESKTOP_H
#define TTK_UTIL_DESKTOP_H


#include <string>
#include <vector>

namespace ttk::Desktop {

    bool open(const std::string &target, std::string *why = nullptr);

    // Windows only. Empty elsewhere.
    [[nodiscard]] std::vector<std::string> drives();

    [[nodiscard]] std::string monospace_family();

}


#endif //TTK_UTIL_DESKTOP_H
