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
    //! Geometry and drag state of a grid of equal cells whose order is changed by dragging one of them.
    //!
    //! The grid is not a widget. Its owner forwards the box from layout and the drag from the pointer, positions
    //! its items with \ref cell_x(), \ref cell_y() and \ref slot(), and moves the dragged item from \ref origin()
    //! to \ref target() when the drag lands. Indices start at 0 and fill the grid row by row.
    class ReorderGrid {
    public:
        //! Spacing of a \ref ReorderGrid, in pixels.
        struct Metrics {
            //! Margin left and right of the cells inside the grid's box.
            double bleed = Theme::bleed;
            //! Gap between neighbouring cells, both across and down.
            double gutter = Theme::gutter;
            //! Space above the first row.
            double top = 0.0;
            //! Narrowest a cell may be. The grid has as many columns as fit at this width, and at least one.
            double narrowest = 0.0;
            //! Height of every row.
            double row_height = 0.0;
            //! Multiple the cell width is rounded down to. A coarser step gives fewer distinct cell widths while
            //! the grid is resized, which helps cells that cache a drawing per width.
            double step = 1.0;
        };

        //! Creates a grid with `metrics` for the widget `page`, which is woken by \ref carried(). The grid does
        //! not own `page`.
        ReorderGrid(Widget *page, const Metrics &metrics) : _page(page), _metrics(metrics) { }

        //! Replaces all metrics with `metrics`. They take effect at the next \ref measure() or \ref place().
        void set_metrics(const Metrics &metrics) { _metrics = metrics; }

        //! Sets \ref Metrics::row_height to `height`.
        void set_row_height(const double height) { _metrics.row_height = height; }

        //! Computes \ref columns() and \ref cell() for a box `width` pixels wide, without changing the box.
        //!
        //! When `width` leaves less than \ref Metrics::narrowest between the margins, the grid has one column of
        //! that width and overflows the box.
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

        //! Sets the box, in window coordinates, that every cell is positioned in and measures the grid for its
        //! width.
        void place(const BLRect &box) {
            _box = box;

            measure(box.w);
        }

        //! Returns the number of columns as of the last \ref measure() or \ref place().
        [[nodiscard]] int columns() const { return _columns; }
        //! Returns the cell width in pixels as of the last \ref measure() or \ref place().
        [[nodiscard]] double cell() const { return _cell; }
        //! Returns \ref Metrics::row_height.
        [[nodiscard]] double row_height() const { return _metrics.row_height; }
        //! Returns \ref Metrics::gutter.
        [[nodiscard]] double gutter() const { return _metrics.gutter; }
        //! Returns \ref Metrics::top.
        [[nodiscard]] double top() const { return _metrics.top; }

        //! Returns the number of rows that `count` cells fill, 0 when `count` is 0.
        [[nodiscard]] int rows_for(const int count) const {
            return (count + _columns - 1) / _columns;
        }

        //! Returns the number of rows that `count` cells fill with one more tile after the last of them, such as a
        //! tile that adds an item. Returns 1 when `count` is 0.
        [[nodiscard]] int rows_with_adder(const int count) const {
            return (count / _columns) + 1;
        }

        //! Returns the left edge, in window coordinates, of the cell at `index` in the box given to \ref place().
        [[nodiscard]] double cell_x(const int index) const {
            return _box.x + _metrics.bleed + ((index % _columns) * (_cell + _metrics.gutter));
        }

        //! Returns the top edge, in window coordinates, of the cell at `index` in the box given to \ref place().
        [[nodiscard]] double cell_y(const int index) const {
            const int row = index / _columns;

            return _box.y + _metrics.top + (row * (_metrics.row_height + _metrics.gutter));
        }

        //! Returns the cell that the item at `index` shows in while an item is being dragged.
        //!
        //! Items between \ref origin() and \ref target() shift one cell toward the origin to open a gap at the
        //! target. Returns `index` itself for every other item, for the dragged item, and when no drag is in
        //! progress.
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

        //! Tests whether a drag is in progress, from \ref grabbed() until \ref landed().
        [[nodiscard]] bool dragging() const { return _dragging; }
        //! Returns the index of the item being dragged, or -1 when no drag is in progress.
        [[nodiscard]] int origin() const { return _origin; }
        //! Returns the index the dragged item would take if dropped now, or -1 when no drag is in progress.
        [[nodiscard]] int target() const { return _target; }

        //! Returns how far the dragged item has been carried across from its own cell, in pixels.
        [[nodiscard]] double carry_x() const { return _carryX; }
        //! Returns how far the dragged item has been carried down from its own cell, in pixels.
        [[nodiscard]] double carry_y() const { return _carryY; }

        //! Returns, for each index the items will hold once the drag lands, where the item moving to that index
        //! is drawn now, as an offset from that index's cell.
        //!
        //! `Card` must provide `slide_x()` and `slide_y()`, the offset an item is currently drawn at from its
        //! slot. Call it before \ref landed(), while \ref origin() and \ref target() still describe the drag.
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
                              cell_y(_origin) + _carryY + card->slide_y() - cell_y(_target),}
                    : BLPoint{card->slide_x(), card->slide_y()};
            }

            return where;
        }

        //! Starts a drag of the item at `index`, held by the pointer at `[x, y]` in window coordinates.
        void grabbed(const int index, const double x, const double y) {
            _dragging = true;
            _origin = index;
            _target = index;
            _grabX = x - cell_x(index);
            _grabY = y - cell_y(index);

            _carryX = 0.0;
            _carryY = 0.0;
        }

        //! Moves the dragged item with the pointer at `[x, y]` and wakes the page.
        //!
        //! \ref target() becomes the cell under the pointer, clamped to the grid's columns and to the `count`
        //! items it holds. Must follow \ref grabbed().
        void carried(const int count, const double x, const double y) {
            _target = place_at(count, x, y);

            _carryX = x - _grabX - cell_x(_origin);
            _carryY = y - _grabY - cell_y(_origin);

            _page->wake();
        }

        //! Ends the drag and clears \ref origin(), \ref target() and the carry offsets. Reorders nothing, so the
        //! owner reads both indices first.
        void landed() {
            _dragging = false;
            _origin = -1;
            _target = -1;
            _carryX = 0.0;
            _carryY = 0.0;
        }

    private:
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

        double _grabX = 0.0;
        double _grabY = 0.0;

        double _carryX = 0.0;
        double _carryY = 0.0;
    };
}


#endif //TTK_LAYOUT_REORDERGRID_H
