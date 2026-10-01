// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_LAYOUT_WRAP_H
#define TTK_LAYOUT_WRAP_H


#include "ttk/toolkit/Widget.h"

namespace ttk {
    //! Layout that places its visible children left to right and starts a new line when the next child would run
    //! past the width.
    //!
    //! Each child takes its wanted width, capped at the layout's width, and its wanted height at that width. A line
    //! is as tall as its tallest child and children sit at the top of their line.
    class Wrap : public Widget {
    public:
        //! Sets the gap between neighbouring children on a line to `across` and the gap between lines to `down`,
        //! in pixels, and returns the wrap. Both default to 8.
        Wrap *spacing(double across, double down);

        //! Returns \ref fixedWidth when it is set, otherwise the widest wanted width among the visible children.
        double natural_width(Typeface &type) override;

        //! Returns \ref fixedHeight when it is set, otherwise the total height of the lines the visible children
        //! wrap into at `width`.
        double natural_height(Typeface &type, double width) override;

        //! Places the visible children in lines across the box width, starting at its top-left corner.
        void arrange(Typeface &type) override;

    private:
        double lay(Typeface &type, double width, bool place) const;

        double _across = 8.0;
        double _down = 8.0;
    };
}


#endif //TTK_LAYOUT_WRAP_H
