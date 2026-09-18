// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "ttk/toolkit/layout/Box.h"
#include "ttk/toolkit/layout/Pair.h"

namespace ttk {
    Box *Pair::cross(const Place where) {
        _rowCross = where;

        return this;
    }

    // Side by side the two share the row exactly. A third of a pixel either way would
    // leave the fields and the switches under them out of line.
    void Pair::reflow(const double width) {
        const bool across = width >= _widest;

        set_flow(across ? Flow::Row : Flow::Column);
        Box::cross(across ? _rowCross : Place::Fill);

        size_t shown = 0;

        for (const Ptr &child : children()) {
            if (child->visible()) {
                ++shown;
            }
        }

        _half = across && shown > 1 ? (width - gap_total()) / static_cast<double>(shown) : -1.0;
    }

    double Pair::main_of(const Ptr &child, Typeface &type) const {
        return _half >= 0.0 ? _half : Box::main_of(child, type);
    }

    double Pair::natural_height(Typeface &type, const double width) {
        reflow(width);

        return Box::natural_height(type, width);
    }

    void Pair::arrange(Typeface &type) {
        reflow(_box.w);

        Box::arrange(type);
    }
}
