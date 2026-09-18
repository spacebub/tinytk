// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_LAYOUT_PANEL_H
#define TTK_LAYOUT_PANEL_H


#include "ttk/toolkit/Widget.h"

namespace ttk {
    // The panel everything on a page sits in.
    class Panel : public Widget {
    public:
        bool inset = false;
        bool hoverable = false;
        bool lit = false;

        double rounding = 12.0;

        // Set to draw no border, which a bare group does.
        bool bordered = true;

        // Off where the owner places the children: filling them first measures a
        // scroller against a height it will not have.
        bool fills = true;

        void arrange(Typeface &type) override {
            if (fills) {
                Widget::arrange(type);
            }
        }

        void paint(const Painter &painter) override;
    };
}


#endif //TTK_LAYOUT_PANEL_H
