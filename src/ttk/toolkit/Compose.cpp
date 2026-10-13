// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "ttk/toolkit/Compose.h"
#include "ttk/draw/Theme.h"

namespace ttk {
std::size_t compose(Root &root, Surface &surface) {
    root.settle();

    // A frame repainted whole has no pixels worth moving.
    if (root.whole()) {
        surface.damage_all();
    }

    for (const Root::Shift &moved : root.take_shifts()) {
        surface.shift(moved.region, moved.dy);
    }

    // Root moved its own pending regions with the pixels, so they go in after the moves.
    for (const BLRect &region : root.take()) {
        surface.damage(region);
    }

    if (!surface.dirty()) {
        return 0;
    }

    BLContext &context = surface.context();
    std::size_t painted = 0;

    for (const BLRectI &region : surface.regions()) {
        context.save();
        context.clip_to_rect(region);
        context.fill_rect(BLRect{static_cast<double>(region.x),
                                 static_cast<double>(region.y),
                                 static_cast<double>(region.w),
                                 static_cast<double>(region.h)},
                          Theme::palette().background);

        root.paint(context, region);

        context.restore();

        painted += static_cast<std::size_t>(region.w) * static_cast<std::size_t>(region.h);
    }

    return painted;
}
}
