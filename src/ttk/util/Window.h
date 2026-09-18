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
    // The window itself, as the title bar's buttons see it. A view that reached for the
    // whole Shell dragged SDL, the surface and the frame loop in behind it.
    class Window {
    public:
        Window() = default;
        virtual ~Window() = default;

        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;
        Window(Window &&) = delete;
        Window &operator=(Window &&) = delete;

        virtual void minimize() const = 0;
        virtual void toggle_maximize() const = 0;

        [[nodiscard]] virtual bool maximized() const = 0;

        // Ends the frame loop, which closes the window.
        virtual void stop() = 0;
    };
}


#endif //TTK_UTIL_WINDOW_H
