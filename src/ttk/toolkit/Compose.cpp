// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Compose.h"

namespace ttk {
    std::size_t compose(Root &root, Surface &surface) {
        root.settle();

        for (const BLRect &region : root.take()) {
            surface.damage(region);
        }

        for (const Root::Shift &moved : root.take_shifts()) {
            surface.shift(moved.region, moved.dy);
        }

        if (!surface.dirty()) {
            return 0;
        }

        BLContext &context = surface.context();
        std::size_t painted = 0;

        for (const BLRectI &region : surface.regions()) {
            context.save();
            context.clip_to_rect(region);
            context.fill_rect(BLRect{static_cast<double>(region.x), static_cast<double>(region.y),
                                     static_cast<double>(region.w), static_cast<double>(region.h)},
                              Theme::of().background);

            root.paint(context, region);

            context.restore();

            painted += static_cast<std::size_t>(region.w) * static_cast<std::size_t>(region.h);
        }

        return painted;
    }
}
