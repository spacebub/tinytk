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

namespace ttk {
    // Dragging and resizing are not here: SDL's hit test hands those to the platform.
    namespace Chrome {

        // Safe to call right after the window is created. The native handle already exists.
        void apply(SDL_Window *window);

        void outline(std::uint8_t red, std::uint8_t green, std::uint8_t blue);

        // Called with true when the desktop opens a modal move/resize loop of its own and
        // false when it closes it. Never called where there is no such thing.
        void while_resizing(void (*told)(bool));

    }
}


#endif //TTK_DRAW_CHROME_H
