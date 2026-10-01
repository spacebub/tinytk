// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_LAYOUT_RULE_H
#define TTK_LAYOUT_RULE_H


#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Painter.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
    //! Horizontal hairline in the theme's border colour, used to divide a panel.
    class Rule : public Widget {
    public:
        //! Creates a rule with a \ref fixedHeight of 1 pixel.
        Rule() { fixedHeight = 1.0; }

        //! Fills the top pixel row of \ref box() with the theme's border colour.
        void paint(const Painter &painter) override {
            painter.fill(BLRect{_box.x, _box.y, _box.w, 1.0}, Theme::palette().border);
        }
    };
}


#endif //TTK_LAYOUT_RULE_H
