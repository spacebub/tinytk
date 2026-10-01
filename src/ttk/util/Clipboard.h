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

//! Text on the system clipboard, through SDL.
namespace ttk::Clipboard {

    //! Returns the text on the clipboard as UTF-8, or an empty string when it holds no text or cannot be read.
    std::string read();

    //! Replaces the clipboard's contents with `text`, which is UTF-8 and ends at its first nul byte.
    void write(const std::string &text);

}


#endif //TTK_UTIL_CLIPBOARD_H
