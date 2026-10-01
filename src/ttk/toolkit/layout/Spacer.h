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
    //! Widget that draws nothing and takes up room in a layout.
    class Spacer : public Widget {
    public:
        //! Creates a spacer with a \ref stretch of `weight`, so a box gives it spare room in proportion to `weight`.
        //!
        //! With a `weight` of zero the spacer is a fixed gap sized by \ref fixedWidth or \ref fixedHeight.
        explicit Spacer(const double weight = 1.0) { stretch = weight; }

        //! Returns \ref fixedWidth when it is set, otherwise 0.
        double natural_width(Typeface & /*type*/) override { return fixedWidth >= 0.0 ? fixedWidth : 0.0; }

        //! Returns \ref fixedHeight when it is set, otherwise 0.
        double natural_height(Typeface & /*type*/, double /*width*/) override {
            return fixedHeight >= 0.0 ? fixedHeight : 0.0;
        }
    };
}


#endif //TTK_LAYOUT_SPACER_H
