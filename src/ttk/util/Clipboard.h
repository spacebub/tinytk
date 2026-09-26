// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_UTIL_CLIPBOARD_H
#define TTK_UTIL_CLIPBOARD_H


#include <string>

namespace ttk::Clipboard {

    std::string read();
    void write(const std::string &text);

}


#endif //TTK_UTIL_CLIPBOARD_H
