// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_DRAW_CHROME_H
#define TTK_DRAW_CHROME_H


#include <cstdint>

#include <SDL3/SDL.h>

//! Native frame adjustments for a borderless window. Calls do nothing outside Windows.
 namespace ttk::Chrome {
    //! Fits the native frame of `window` to a borderless window.
    //!
    //! On Windows it subclasses the window procedure to keep one row of frame for the compositor to round, outline
    //! and shadow, to keep a maximized window inside the monitor's work area and a pixel clear of an auto-hidden
    //! taskbar, asks for rounded corners and applies a colour already given to \ref outline(). Safe to call right
    //! after the window is created. Only one window is tracked, the last one passed.
    void apply(SDL_Window *window);

    //! Sets the colour of the border the compositor draws around the window passed to \ref apply().
    //!
    //! `red`, `green` and `blue` are 0 to 255.
    //!
    //! The colour is kept and applied by \ref apply() when no window has been applied yet. Windows versions without
    //! border colours ignore it.
    void outline(std::uint8_t red, std::uint8_t green, std::uint8_t blue);

    //! Sets `told` as the function called with true when the desktop enters a modal move or resize loop of its own,
    //! and with false when it leaves it.
    //!
    //! Never called on platforms without such a loop. Only the window passed to \ref apply() reports it. Null
    //! removes the function.
    void while_resizing(void (*told)(bool));
}


#endif //TTK_DRAW_CHROME_H
