// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_DRAW_DAMAGE_H
#define TTK_DRAW_DAMAGE_H


#include <vector>

#include <blend2d/blend2d.h>

namespace ttk {
    // Which rectangles of a frame changed. The window and the benchmark canvas both
    // keep one, so a change to how regions are merged cannot leave the two measuring
    // different things.
    class Damage {
    public:
        // More than this many separate rectangles and it is cheaper to present one that
        // covers them all than to hand the desktop a long list.
        static constexpr size_t CROWDED = 12;

        void resize(int width, int height);

        // Clamped to the frame, dropped when something held already covers it, and the
        // lot folded into one once there are too many.
        void add(const BLRect &region);

        void all();

        void clear() { _regions.clear(); }

        [[nodiscard]] bool empty() const { return _regions.empty(); }

        [[nodiscard]] const std::vector<BLRectI> &regions() const { return _regions; }

        // `region` clipped to the frame. Empty when none of it is inside.
        [[nodiscard]] BLRectI clamp_to(const BLRect &region) const;

    private:
        std::vector<BLRectI> _regions;

        int _width = 0;
        int _height = 0;
    };
}


#endif //TTK_DRAW_DAMAGE_H
