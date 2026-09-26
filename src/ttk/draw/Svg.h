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

namespace ttk::Svg {

    // Fills `out`, which is cleared first. False on the first token that makes no
    // sense, with whatever parsed cleanly left in place.
    bool parse(const char *commands, BLPath &out);

    // Scaled from a viewbox of `box` units square into a `size` square.
    BLPath glyph(const char *commands, float box, float size);

}


#endif //TTK_DRAW_SVG_H
