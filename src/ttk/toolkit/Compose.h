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
    // Settles the tree, carries its damage and shifts onto the surface, and paints
    // what changed. Answers the pixels painted. Presenting is the caller's to do.
    std::size_t compose(Root &root, Surface &surface);
}


#endif //TTK_TOOLKIT_COMPOSE_H
