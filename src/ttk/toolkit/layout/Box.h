// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_LAYOUT_BOX_H
#define TTK_LAYOUT_BOX_H


#include <cstdint>
#include <memory>
#include <vector>

#include "ttk/toolkit/Widget.h"

namespace ttk {
    //! Layout that places its visible children one after another in a row or a column.
    //!
    //! Along the main axis each child takes its wanted size, and room left over is shared among the children in
    //! proportion to their \ref stretch. Across the axis each child fills the inner box unless \ref cross() says
    //! otherwise or the child has a fixed size on that axis. Padding and spacing are in pixels.
    class Box : public Widget {
    public:
        //! Main axis of a box.
        enum class Flow : std::uint8_t {
            //! Children run left to right.
            Row,
            //! Children run top to bottom.
            Column,
        };

        //! Position of children within the room they are given.
        enum class Place : std::uint8_t {
            //! Across the axis, the child takes the whole inner extent. Along the axis, children pack at the start.
            Fill,
            //! Children sit at the left or top.
            Start,
            //! Children are centred.
            Centre,
            //! Children sit at the right or bottom.
            End,
        };

        //! Creates a box whose main axis is `flow`.
        explicit Box(const Flow flow) : _flow(flow) {}

        //! Creates a box that places its children left to right.
        static std::unique_ptr<Box> row() { return std::make_unique<Box>(Flow::Row); }

        //! Creates a box that places its children top to bottom.
        static std::unique_ptr<Box> column() { return std::make_unique<Box>(Flow::Column); }

        //! Sets the gap between neighbouring visible children to `value` pixels and returns the box.
        Box *spacing(double value);

        //! Sets the padding on all four sides to `all` pixels and returns the box.
        Box *pad(double all);

        //! Sets the left and right padding to `sides` and the top and bottom padding to `ends`, in pixels, and
        //! returns the box.
        Box *pad(double sides, double ends);

        //! Sets the padding on each side to `left`, `top`, `right` and `bottom` pixels and returns the box.
        Box *pad(double left, double top, double right, double bottom);

        //! Sets where the children sit along the main axis when they leave room spare, and returns the box.
        //!
        //! Takes effect only while the \ref stretch of every visible child is zero, since otherwise the spare room
        //! is shared out. \ref Place::Fill packs the children at the start, as \ref Place::Start does.
        Box *align(Place where);

        //! Sets where each child sits across the main axis, and returns the box.
        //!
        //! With \ref Place::Fill a child takes the whole inner extent. Otherwise it takes its natural extent, capped
        //! at the inner extent. A child's \ref fixedHeight in a row, or \ref fixedWidth in a column, overrides both
        //! and is not capped.
        virtual Box *cross(Place where);

        //! Sets the box's own \ref stretch to `weight` and returns the box.
        Box *grow(double weight);

        //! Returns \ref fixedWidth when it is set. Otherwise returns the sum of the wanted widths of the visible
        //! children and the spacing between them for a row, or the widest of them for a column, plus the left
        //! and right padding.
        double natural_width(Typeface &type) override;

        //! Returns \ref fixedHeight when it is set. Otherwise returns the height needed at `width`, raised to
        //! \ref minHeight.
        //!
        //! A column sums the wanted heights of its visible children and the spacing between them, each measured at
        //! its own \ref fixedWidth or else the inner width. A row returns its tallest child, each measured at the
        //! width \ref arrange() would give it. Both add the top and bottom padding.
        double natural_height(Typeface &type, double width) override;

        //! Places the visible children inside the padded box, sized by \ref share() along the main axis and
        //! positioned by \ref align() and \ref cross().
        void arrange(Typeface &type) override;

    private:
        struct Slot {
            Widget *who = nullptr;
            double main = 0.0;
        };

    protected:
        //! Returns the total spacing between the visible children, which is zero with fewer than two of them.
        [[nodiscard]] double gap_total() const;

        //! Returns the size each visible child gets along the main axis, in child order, when `room` pixels are
        //! available.
        //!
        //! A row starts each child at \ref main_of(). A column starts each child at its wanted height, measured
        //! at its \ref fixedWidth or else at `across`. Spare room is added in proportion to \ref stretch. A
        //! shortfall is taken from the stretching children in the same proportion, or from all children in
        //! proportion to their size when none stretch, never shrinking a child below its \ref minWidth or
        //! \ref minHeight.
        std::vector<double> share(Typeface &type, double room, double across) const;

        //! Returns the width `child` takes along a row before spare room is shared out.
        //!
        //! The default implementation returns the child's \ref Widget::wanted_width(). A layout that divides the
        //! row itself returns its own share here instead of setting a width on the child.
        [[nodiscard]] virtual double main_of(const Ptr &child, Typeface &type) const;

    private:

    protected:
        //! Sets the main axis to `flow`. Does not ask for a new layout.
        void set_flow(const Flow flow) { _flow = flow; }

    private:
        Flow _flow;

        double _spacing = 0.0;

        double _left = 0.0;
        double _top = 0.0;
        double _right = 0.0;
        double _bottom = 0.0;

        Place _align = Place::Fill;
        Place _cross = Place::Fill;
    };
}


#endif //TTK_LAYOUT_BOX_H
