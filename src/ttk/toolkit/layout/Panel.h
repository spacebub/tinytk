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
    //! Rounded surface with an optional border that other widgets sit on.
    //!
    //! The panel never sets \ref lit itself. Its owner sets it, for example while the pointer is inside, and
    //! repaints.
    class Panel : public Widget {
    public:
        //! Whether the panel is filled with the theme's sunken colour instead of its surface colour.
        bool inset = false;

        //! Whether the hover colour is painted over the fill while \ref lit is set.
        bool hoverable = false;

        //! Whether the panel is highlighted. A lit panel draws its border in the theme's strong border colour, and
        //! also paints the hover colour when \ref hoverable is set.
        bool lit = false;

        //! Corner radius in pixels.
        double rounding = 12.0;

        //! Whether a 1 pixel border is drawn around the panel.
        bool bordered = true;

        //! Whether \ref arrange() gives every child the panel's box. Clear it when the owner places the children
        //! itself, so they are not first measured against a size they will not have.
        bool fills = true;

        //! Gives every child the panel's box when \ref fills is set, otherwise does nothing.
        void arrange(Typeface &type) override {
            if (fills) {
                Widget::arrange(type);
            }
        }

        //! Paints the fill, the hover colour and the border as configured, then the children.
        void paint(const Painter &painter) override;
    };
}


#endif //TTK_LAYOUT_PANEL_H
