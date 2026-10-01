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

//! Drop shadows, gradients and images drawn with Blend2D.
namespace ttk::Paint {

    //! Returns a premultiplied sprite of the drop shadow of a rounded rectangle `width` by `height` pixels with
    //! corner `radius`, blurred by `blur` and coloured `tint`.
    //!
    //! `blur` is read as CSS reads it, twice the Gaussian standard deviation. The sprite extends \ref bleed() pixels
    //! beyond the rectangle on every side. Sprites are cached by size, `radius` and `blur` to a quarter pixel, and
    //! `tint`. The cache is emptied when it holds more than 24 sprites, which ends the life of every reference it
    //! returned. Returns an empty image when the sprite cannot be allocated.
    const BLImage &shadow(int width, int height, double radius, double blur, BLRgba32 tint);

    //! Returns the whole number of pixels a \ref shadow() sprite for `blur` extends beyond its rectangle on each side.
    //!
    //! For a rectangle at `[x, y]` the sprite's top-left corner goes at `[x - bleed(blur), y - bleed(blur)]`.
    double bleed(double blur);

    //! Returns a linear gradient with no stops that runs from the top edge of `box` to its bottom edge.
    BLGradient down(const BLRect &box);

    //! Returns the image decoded from the file at `path` by Blend2D's codecs, such as PNG, or an empty image when
    //! the file cannot be read or decoded.
    BLImage load(const std::string &path);

    //! Draws `source` scaled to cover `box`, keeping its aspect ratio, centred and cropped to `box` as CSS `cover`
    //! does.
    //!
    //! A positive `radius` rounds the corners of the drawn area, clamped to half the shorter side of `box`. Does
    //! nothing when `source` is empty.
    void cover(BLContext &context, const BLRect &box, const BLImage &source, double radius = 0.0);

}


#endif //TTK_DRAW_PAINT_H
