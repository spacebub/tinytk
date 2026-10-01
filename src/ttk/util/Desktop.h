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

//! Requests to the desktop and facts about the machine it runs on.
namespace ttk::Desktop {

    //! Opens `target`, a URL or a path in UTF-8, with the desktop's default handler for it.
    //!
    //! Uses `ShellExecute` on Windows, `open` on macOS and `xdg-open` elsewhere, and does not wait for the handler.
    //! Returns false for an empty `target` or when the handler cannot be started, and then stores the reason in
    //! `why` unless it is null. On Windows a true result means the shell accepted `target`, elsewhere it only
    //! means the opener was started.
    bool open(const std::string &target, std::string *why = nullptr);

    //! Returns the root of each drive letter present, such as `C:/`, in letter order. Empty except on Windows.
    [[nodiscard]] std::vector<std::string> drives();

    //! Returns the family name of the preferred monospace font installed, or `monospace` when none is found.
    //!
    //! Looks through the system and user font directories for files of JetBrains Mono, Fira Code, DejaVu Sans Mono
    //! and Noto Sans Mono, then of Consolas and Courier New on Windows or Menlo and Monaco on macOS, in that order
    //! of preference. Each call walks the directories again.
    [[nodiscard]] std::string monospace_family();

}


#endif //TTK_UTIL_DESKTOP_H
