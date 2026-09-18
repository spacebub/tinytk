// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_DRAW_PAINT_H
#define TTK_DRAW_PAINT_H


#include <string>

#include <blend2d/blend2d.h>

namespace ttk {
    namespace Paint {

        // A drop shadow, as a sprite: Blend2D has no blur of any kind, so a rounded
        // rectangle is blurred by hand and kept, cached by its shape. `blur` is read the
        // way CSS reads it, as twice the Gaussian sigma.
        const BLImage &shadow(int width, int height, double radius, double blur, BLRgba32 tint);

        // Where the sprite's top-left goes if the box it belongs to is at (x, y).
        double bleed(double blur);

        // A vertical gradient across `box`, top to bottom.
        BLGradient down(const BLRect &box);

        // An image, or an empty one. Blend2D decodes PNG itself.
        BLImage load(const std::string &path);

        // `source` drawn to fill `box` and cropped to it, which is CSS's `cover`. A radius
        // rounds the corners off what is drawn.
        void cover(BLContext &context, const BLRect &box, const BLImage &source, double radius = 0.0);

    }
}


#endif //TTK_DRAW_PAINT_H
