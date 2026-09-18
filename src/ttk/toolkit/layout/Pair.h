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
    // Two things side by side, stacked instead when the page is too narrow for them.
    class Pair : public Box {
    public:
        explicit Pair(const double widest) : Box(Flow::Row), _widest(widest) {}

        double natural_height(Typeface &type, double width) override;

        // Kept for the row. Stacked, the two always fill the width instead.
        Box *cross(Place where) override;

        void arrange(Typeface &type) override;

    protected:
        [[nodiscard]] double main_of(const Ptr &child, Typeface &type) const override;

    private:
        void reflow(double width);

        double _widest;

        // The share each half takes across a row. Negative when stacked.
        double _half = -1.0;

        Place _rowCross = Place::Fill;
    };
}


#endif //TTK_LAYOUT_PAIR_H
