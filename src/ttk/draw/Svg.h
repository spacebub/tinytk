// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_DRAW_SVG_H
#define TTK_DRAW_SVG_H


#include <blend2d/blend2d.h>

//! Parsing of SVG path data into Blend2D paths.
namespace ttk::Svg {

    //! Parses the SVG path data in `commands` into `out`, which is cleared first.
    //!
    //! Accepts every SVG path command in absolute and relative form, with commas, whitespace or nothing as
    //! separators where the grammar allows. Returns false when `commands` is null or on the first token that does
    //! not parse, leaving in `out` whatever was parsed before it.
    bool parse(const char *commands, BLPath &out);

    //! Returns the SVG path data in `commands` scaled from a square viewbox `box` units across to a square `size`
    //! units across. `box` must be positive.
    //!
    //! When \ref parse() fails, returns the part parsed before the failure without scaling it.
    BLPath glyph(const char *commands, float box, float size);

}


#endif //TTK_DRAW_SVG_H
