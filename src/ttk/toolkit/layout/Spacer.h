// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_LAYOUT_SPACER_H
#define TTK_LAYOUT_SPACER_H


#include "ttk/toolkit/Widget.h"

namespace ttk {
    // Takes whatever width or height is going and draws nothing.
    class Spacer : public Widget {
    public:
        explicit Spacer(const double weight = 1.0) { stretch = weight; }

        double natural_width(Typeface & /*type*/) override { return fixedWidth >= 0.0 ? fixedWidth : 0.0; }

        double natural_height(Typeface & /*type*/, double /*width*/) override {
            return fixedHeight >= 0.0 ? fixedHeight : 0.0;
        }
    };
}


#endif //TTK_LAYOUT_SPACER_H
