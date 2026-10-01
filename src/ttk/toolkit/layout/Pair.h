// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_LAYOUT_PAIR_H
#define TTK_LAYOUT_PAIR_H


#include "ttk/toolkit/Widget.h"
#include "ttk/toolkit/layout/Box.h"

namespace ttk {
    //! Row of children that turns into a column when it is too narrow.
    //!
    //! Side by side, the visible children split the width less spacing into equal parts, whatever their own
    //! sizes. Stacked, each child takes the full width.
    class Pair : public Box {
    public:
        //! Creates a pair that sits side by side when given at least `widest` pixels across and stacks otherwise.
        explicit Pair(const double widest) : Box(Flow::Row), _widest(widest) {}

        //! Chooses side by side or stacked for `width`, then returns the height \ref Box::natural_height() measures
        //! for that arrangement.
        double natural_height(Typeface &type, double width) override;

        //! Sets where the children sit across the row while side by side, and returns the pair. Stacked, they
        //! always fill the width.
        Box *cross(Place where) override;

        //! Chooses side by side or stacked for the current box width, then places the children as
        //! \ref Box::arrange() does.
        void arrange(Typeface &type) override;

    protected:
        //! Returns the equal part of the row each child takes while side by side. Returns the child's wanted
        //! width when stacked or when fewer than two children are visible.
        [[nodiscard]] double main_of(const Ptr &child, Typeface &type) const override;

    private:
        void reflow(double width);

        double _widest;

        double _half = -1.0;

        Place _rowCross = Place::Fill;
    };
}


#endif //TTK_LAYOUT_PAIR_H
