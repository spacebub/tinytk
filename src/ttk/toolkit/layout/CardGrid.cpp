// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <utility>

#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/layout/CardGrid.h"

namespace ttk {
    Card *CardGrid::add(std::unique_ptr<Card> card) {
        Card *raw = append(std::move(card));

        raw->draggable = true;

        raw->drag_started = [this, raw](const double x, const double y) {
            const auto found = std::ranges::find(_cards, raw);

            if (found != _cards.end()) {
                _reorder.grabbed(static_cast<int>(found - _cards.begin()), x, y);
            }
        };

        raw->drag_moved = [this](const double x, const double y) {
            _reorder.carried(static_cast<int>(_cards.size()), x, y);
        };

        raw->drag_ended = [this] { land(); };

        _cards.push_back(raw);

        if (root() != nullptr) {
            root()->relayout();
        }

        return raw;
    }

    void CardGrid::clear_cards() {
        _reorder.landed();
        _settle.clear();
        _settling = -1;
        _cards.clear();
        clear();

        if (root() != nullptr) {
            root()->relayout();
        }
    }

    double CardGrid::natural_height(Typeface & /*type*/, const double width) {
        _reorder.measure(width);

        const int rows = _reorder.rows_for(static_cast<int>(_cards.size()));

        return rows > 0 ? (rows * (_reorder.row_height() + _reorder.gutter())) - _reorder.gutter() + _reorder.top() : 0.0;
    }

    void CardGrid::arrange(Typeface &type) {
        _reorder.place(_box);

        const int count = static_cast<int>(_cards.size());

        for (int index = 0; index < count; ++index) {
            Card *card = _cards[static_cast<size_t>(index)];
            const int at = _reorder.slot(index);
            const bool carried = index == _reorder.origin();
            const double carryX = carried ? _reorder.carry_x() : 0.0;
            const double carryY = carried ? _reorder.carry_y() : 0.0;

            const BLRect was = card->box();
            const BLRect cell{_reorder.cell_x(at), _reorder.cell_y(at), _reorder.cell(), _reorder.row_height()};

            // The walk is between two cells of the grid as it stands now. Comparing
            // against where the card was drawn would make a scroll look like a reorder.
            if (const int wasAt = card->slot(); !carried && wasAt >= 0 && wasAt != at) {
                card->slide_from(_reorder.cell_x(wasAt) - cell.x, _reorder.cell_y(wasAt) - cell.y, card->now());
            }

            card->set_slot(at);

            const BLRect now{cell.x + carryX + card->slide_x(), cell.y + carryY + card->slide_y(), cell.w, cell.h};

            card->place(now, type);

            if (was.x != now.x || was.y != now.y || was.w != now.w) {
                card->invalidate(was);
                card->invalidate(now);
            }
        }
    }

    void CardGrid::paint(const Painter &painter) {
        const int carried = _reorder.origin() >= 0 ? _reorder.origin() : _settling;

        for (int index = 0; std::cmp_less(index, _cards.size()); ++index) {
            if (index == carried) {
                continue;
            }

            if (Card *card = _cards[static_cast<size_t>(index)]; painter.needed(card->box())) {
                card->paint(painter);
            }
        }

        // Last, so a card in hand is over the ones it is being carried past.
        if (carried >= 0 && std::cmp_less(carried, _cards.size())) {
            _cards[static_cast<size_t>(carried)]->paint(painter);
        }
    }

    void CardGrid::land() {
        const int from = _reorder.origin();
        const int to = _reorder.target();

        if (from < 0) {
            _reorder.landed();

            return;
        }

        // Before the grid forgets the drag: where each card is now, by the index it
        // is about to take.
        _settle = _reorder.offsets(_cards);
        _settling = to;

        _reorder.landed();

        if (to != from) {
            Card *moved = _cards[static_cast<size_t>(from)];

            _cards.erase(_cards.begin() + from);
            _cards.insert(_cards.begin() + to, moved);

            // Every card now sits where its index says. Without this the dropped one
            // would remember the cell it came from and walk from there instead of
            // from where it was let go.
            for (size_t index = 0; index < _cards.size(); ++index) {
                _cards[index]->set_slot(static_cast<int>(index));
            }

            if (reordered) {
                reordered(from, to);
            }
        }

        wake();
        invalidate();
    }

    void CardGrid::settle(const double now) {
        if (_settle.empty()) {
            return;
        }

        if (_settle.size() != _cards.size()) {
            _settling = -1;
            _settle.clear();

            return;
        }

        for (size_t index = 0; index < _cards.size(); ++index) {
            if (const BLPoint &from = _settle[index]; from.x != 0.0 || from.y != 0.0) {
                _cards[index]->slide_from(from.x, from.y, now);
            }
        }

        _settle.clear();
    }

    bool CardGrid::advance(const double now) {
        settle(now);

        // Laid out here rather than through a relayout, so the carried card follows
        // the pointer on this frame and the walkers move under it on every one.
        if (root() != nullptr) {
            arrange(root()->type());
        }

        if (_reorder.dragging()) {
            return true;
        }

        const bool walking = std::ranges::any_of(_cards, [](const Card *card) { return card->sliding(); });

        if (!walking && _settling >= 0) {
            _settling = -1;
            invalidate();
        }

        return walking;
    }
}
