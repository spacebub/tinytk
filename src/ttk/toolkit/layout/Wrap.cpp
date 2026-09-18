// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>

#include "ttk/toolkit/layout/Wrap.h"

namespace ttk {
    Wrap *Wrap::spacing(const double across, const double down) {
        _across = across;
        _down = down;

        return this;
    }

    double Wrap::lay(Typeface &type, const double width, const bool place) const {
        double x = 0.0;
        double y = 0.0;
        double line = 0.0;

        for (const Ptr &child : children()) {
            if (!child->visible()) {
                continue;
            }

            const double wanted = std::min(width, child->wanted_width(type));
            const double tall = child->wanted_height(type, wanted);

            if (x > 0.0 && x + wanted > width) {
                x = 0.0;
                y += line + _down;
                line = 0.0;
            }

            if (place) {
                child->place(BLRect{_box.x + x, _box.y + y, wanted, tall}, type);
            }

            x += wanted + _across;
            line = std::max(line, tall);
        }

        return y + line;
    }

    double Wrap::natural_width(Typeface &type) {
        if (fixedWidth >= 0.0) {
            return fixedWidth;
        }

        // The widest child, not the sum: the layout fits whatever it is given.
        double widest = 0.0;

        for (const Ptr &child : children()) {
            if (child->visible()) {
                widest = std::max(widest, child->wanted_width(type));
            }
        }

        return widest;
    }

    double Wrap::natural_height(Typeface &type, const double width) {
        return fixedHeight >= 0.0 ? fixedHeight : lay(type, width, false);
    }

    void Wrap::arrange(Typeface &type) {
        lay(type, _box.w, true);
    }
}
