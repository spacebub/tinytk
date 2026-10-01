// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_TOOLKIT_COMPOSE_H
#define TTK_TOOLKIT_COMPOSE_H


#include <cstddef>

#include "ttk/draw/Surface.h"
#include "ttk/toolkit/Root.h"

namespace ttk {
    //! Brings `surface` up to date with `root` and returns the number of pixels repainted.
    //!
    //! Runs \ref Root::settle(), hands the damage from \ref Root::take() and the moves from \ref Root::take_shifts()
    //! to `surface`, then fills each damaged region of `surface` with the theme background and paints the tree over
    //! it. Returns 0 when nothing was damaged. Presenting `surface` is left to the caller.
    std::size_t compose(Root &root, Surface &surface);
}


#endif //TTK_TOOLKIT_COMPOSE_H
