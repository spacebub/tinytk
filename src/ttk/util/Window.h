// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_UTIL_WINDOW_H
#define TTK_UTIL_WINDOW_H

namespace ttk {
    //! Window operations a title bar needs, without the frame loop or surface behind them.
    class Window {
    public:
        Window() = default;
        virtual ~Window() = default;

        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;
        Window(Window &&) = delete;
        Window &operator=(Window &&) = delete;

        //! Asks the desktop to minimize the window.
        virtual void minimize() const = 0;

        //! Asks the desktop to restore the window when \ref maximized() is true, and to maximize it otherwise.
        virtual void toggle_maximize() const = 0;

        //! Tests whether the window is maximized, as last reported by the desktop.
        [[nodiscard]] virtual bool maximized() const = 0;

        //! Ends the frame loop once its current turn is over, which closes the window.
        virtual void stop() = 0;
    };
}


#endif //TTK_UTIL_WINDOW_H
