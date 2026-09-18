// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "ttk/draw/Chrome.h"

namespace ttk {
    // X11, Wayland and macOS have nothing to say here yet: SDL's hit test already
    // drives move and resize, and the rest (rounding, outline, maximize bounds) is
    // the compositor's own business on those platforms.

    void Chrome::apply(SDL_Window * /*unused*/) {
    }

    void Chrome::outline(std::uint8_t /*unused*/, std::uint8_t /*unused*/, std::uint8_t /*unused*/) {
    }

    void Chrome::while_resizing(void (* /*unused*/)(bool)) {
    }
}
