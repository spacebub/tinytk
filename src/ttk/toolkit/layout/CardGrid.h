// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_LAYOUT_CARDGRID_H
#define TTK_LAYOUT_CARDGRID_H


#include <functional>
#include <memory>
#include <vector>

#include "ttk/toolkit/controls/Card.h"
#include "ttk/toolkit/layout/ReorderGrid.h"

namespace ttk {
    //! Grid of cards in as many equal columns as fit, which the user reorders by dragging a card onto another cell.
    //!
    //! Cards must be added through \ref add(), which takes over their drag callbacks. While a card is carried the
    //! cards between its old and new cell shift by one, and after the drop every displaced card slides into place.
    class CardGrid : public Widget {
    public:
        //! Creates a grid whose margins, gutter, narrowest column, row height and width step come from `metrics`.
        explicit CardGrid(const ReorderGrid::Metrics &metrics) : _reorder(this, metrics) {}

        //! Called after a drop has moved a card, with the index it left in `from` and the index it now holds in
        //! `to`. Not called when a card is dropped back on its own cell. \ref cards() is already in the new order.
        std::function<void(int from, int to)> reordered;

        //! Appends `card` as the last card, makes it draggable and returns it.
        //!
        //! Replaces the card's \ref Card::drag_started, \ref Card::drag_moved and \ref Card::drag_ended callbacks
        //! and asks the root for a new layout.
        Card *add(std::unique_ptr<Card> card);

        //! Destroys all children, cancels any drag in progress and asks the root for a new layout.
        void clear_cards();

        //! Returns the cards in grid order, left to right and then top to bottom.
        [[nodiscard]] const std::vector<Card *> &cards() const { return _cards; }

        //! Returns the height of the rows the cards fill at `width`, from the top margin to the bottom of the last
        //! row, or 0 when there are no cards.
        double natural_height(Typeface &type, double width) override;

        //! Places each card in its cell, offset by the drag or slide it is in, and repaints every card that moved.
        void arrange(Typeface &type) override;

        //! Paints the cards that intersect the area being repainted. The carried card, or the one still sliding to
        //! where it was dropped, is painted last so it stays on top.
        void paint(const Painter &painter) override;

        //! Lays the grid out again for the frame time `now`, so the carried card follows the pointer and the others
        //! slide. Returns true while a drag is in progress or any card is still sliding.
        bool advance(double now) override;

    private:
        void land();
        void settle(double now);

        ReorderGrid _reorder;
        std::vector<Card *> _cards;

        std::vector<BLPoint> _settle;
        int _settling = -1;
    };
}


#endif //TTK_LAYOUT_CARDGRID_H
