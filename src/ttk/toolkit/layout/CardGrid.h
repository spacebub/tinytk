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
    // Cards in columns, as many as fit, reordered by dragging one to another cell.
    class CardGrid : public Widget {
    public:
        explicit CardGrid(const ReorderGrid::Metrics &metrics) : _reorder(this, metrics) {}

        // Called once a drop has moved a card, with the index it left and the one it took.
        std::function<void(int from, int to)> reordered;

        Card *add(std::unique_ptr<Card> card);
        void clear_cards();

        [[nodiscard]] const std::vector<Card *> &cards() const { return _cards; }

        double natural_height(Typeface &type, double width) override;
        void arrange(Typeface &type) override;
        void paint(const Painter &painter) override;
        bool advance(double now) override;

    private:
        void land();
        void settle(double now);

        ReorderGrid _reorder;
        std::vector<Card *> _cards;

        // Where each card was drawn as the drop happened, so it can walk from there.
        std::vector<BLPoint> _settle;
        // The dropped card, kept on top until it has walked to its cell.
        int _settling = -1;
    };
}


#endif //TTK_LAYOUT_CARDGRID_H
