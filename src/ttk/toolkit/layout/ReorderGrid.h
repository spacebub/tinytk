// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_LAYOUT_REORDERGRID_H
#define TTK_LAYOUT_REORDERGRID_H


#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include <blend2d/blend2d.h>

#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
    // Grids differ in their metrics and in what a finished drag does with the two
    // indices. Everything between those two ends is this.
    class ReorderGrid {
    public:
        // The page's own spacing. `narrowest` is the width below which a column is
        // dropped, and `step` is what the cell width is rounded down to.
        struct Metrics {
            double bleed = Theme::bleed;
            double gutter = Theme::gutter;
            double top = 0.0;
            double narrowest = 0.0;
            double row_height = 0.0;
            // Cell widths are stepped, since every distinct width is a sprite rebuilt from scratch.
            double step = 1.0;
        };

        ReorderGrid(Widget *page, const Metrics &metrics) : _page(page) { _metrics = metrics; }

        void set_metrics(const Metrics &metrics) { _metrics = metrics; }

        void set_row_height(const double height) { _metrics.row_height = height; }

        void measure(const double width) {
            const double room = std::max(_metrics.narrowest, width - (_metrics.bleed * 2.0));

            _columns = std::max(1, static_cast<int>(std::floor((room + _metrics.gutter)
                                                               / (_metrics.narrowest
                                                                  + _metrics.gutter))));

            // Stepped: every distinct width is a card sprite and a shadow sprite built
            // from scratch, and a drag walks through one per frame. The row already ends
            // short of the header by up to a column's rounding.
            _cell = std::floor((room - ((_columns - 1) * _metrics.gutter)) / _columns
                               / _metrics.step)
                * _metrics.step;
        }

        // Where the shelf was put, which every cell is measured from.
        void place(const BLRect &box) {
            _box = box;

            measure(box.w);
        }

        [[nodiscard]] int columns() const { return _columns; }
        [[nodiscard]] double cell() const { return _cell; }
        [[nodiscard]] double row_height() const { return _metrics.row_height; }
        [[nodiscard]] double gutter() const { return _metrics.gutter; }
        [[nodiscard]] double top() const { return _metrics.top; }

        [[nodiscard]] int rows_for(const int count) const {
            return (count + _columns - 1) / _columns;
        }

        // With room for the tile that adds one, which always follows the last card.
        [[nodiscard]] int rows_with_adder(const int count) const {
            return (count / _columns) + 1;
        }

        [[nodiscard]] double cell_x(const int index) const {
            return _box.x + _metrics.bleed + ((index % _columns) * (_cell + _metrics.gutter));
        }

        [[nodiscard]] double cell_y(const int index) const {
            const int row = index / _columns;

            return _box.y + _metrics.top + (row * (_metrics.row_height + _metrics.gutter));
        }

        // Where a card shows while another is carried: the ones between the two ends
        // shuffle up or down by one.
        [[nodiscard]] int slot(const int index) const {
            if (_origin < 0 || index == _origin) {
                return index;
            }

            if (_origin < _target && index > _origin && index <= _target) {
                return index - 1;
            }

            if (_origin > _target && index >= _target && index < _origin) {
                return index + 1;
            }

            return index;
        }

        [[nodiscard]] bool dragging() const { return _dragging; }
        [[nodiscard]] int origin() const { return _origin; }
        [[nodiscard]] int target() const { return _target; }

        [[nodiscard]] double carry_x() const { return _carryX; }
        [[nodiscard]] double carry_y() const { return _carryY; }

        // Where each card is drawn, by the index it is about to take, as an offset
        // from that index's cell. Valid only while the drag is still on the grid.
        template <typename Card>
        [[nodiscard]] std::vector<BLPoint> offsets(const std::vector<Card *> &cards) const {
            std::vector<BLPoint> where(cards.size());

            for (int index = 0; std::cmp_less(index, cards.size()); ++index) {
                const int at = index == _origin ? _target : slot(index);

                if (at < 0 || std::cmp_greater_equal(at, cards.size())) {
                    continue;
                }

                const Card *card = cards[static_cast<size_t>(index)];

                where[static_cast<size_t>(at)] = index == _origin
                    ? BLPoint{cell_x(_origin) + _carryX + card->slide_x() - cell_x(_target),
                              cell_y(_origin) + _carryY + card->slide_y() - cell_y(_target)}
                    : BLPoint{card->slide_x(), card->slide_y()};
            }

            return where;
        }

        void grabbed(const int index, const double x, const double y) {
            _dragging = true;
            _origin = index;
            _target = index;
            _grabX = x - cell_x(index);
            _grabY = y - cell_y(index);

            _carryX = 0.0;
            _carryY = 0.0;
        }

        void carried(const int count, const double x, const double y) {
            _target = place_at(count, x, y);

            _carryX = x - _grabX - cell_x(_origin);
            _carryY = y - _grabY - cell_y(_origin);

            _page->wake();
        }

        void landed() {
            _dragging = false;
            _origin = -1;
            _target = -1;
            _carryX = 0.0;
            _carryY = 0.0;
        }

    private:
        // The index the pointer is over, clamped to the shelf.
        [[nodiscard]] int place_at(const int count, const double x, const double y) const {
            if (count == 0) {
                return 0;
            }

            const int row = static_cast<int>(std::floor((y - _box.y - _metrics.top)
                                                        / (_metrics.row_height + _metrics.gutter)));
            const int column = std::clamp(
                static_cast<int>(std::floor((x - _box.x - _metrics.bleed)
                                            / (_cell + _metrics.gutter))),
                0, _columns - 1);

            return std::clamp((row * _columns) + column, 0, count - 1);
        }

        Widget *_page;

        Metrics _metrics;

        BLRect _box{};

        int _columns = 1;
        double _cell = 0.0;

        int _origin = -1;
        int _target = -1;
        bool _dragging = false;

        // Where in the card the pointer took hold.
        double _grabX = 0.0;
        double _grabY = 0.0;

        double _carryX = 0.0;
        double _carryY = 0.0;
    };
}


#endif //TTK_LAYOUT_REORDERGRID_H
