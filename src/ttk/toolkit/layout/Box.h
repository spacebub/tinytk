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
    // A child takes its natural size along the main axis unless it carries a stretch,
    // in which case it shares what is left over. Across the axis it fills, unless the
    // layout says otherwise or the child asks for a width of its own.
    class Box : public Widget {
    public:
        enum class Flow : std::uint8_t {
            Row,
            Column,
        };

        enum class Place : std::uint8_t {
            Fill,
            Start,
            Centre,
            End,
        };

        explicit Box(const Flow flow) : _flow(flow) {}

        static std::unique_ptr<Box> row() { return std::make_unique<Box>(Flow::Row); }

        static std::unique_ptr<Box> column() { return std::make_unique<Box>(Flow::Column); }

        Box *spacing(double value);
        Box *pad(double all);
        Box *pad(double sides, double ends);
        Box *pad(double left, double top, double right, double bottom);

        // Where the children sit when they do not fill the main axis.
        Box *align(Place where);

        // Where a child sits across the axis.
        virtual Box *cross(Place where);

        Box *grow(double weight);

        double natural_width(Typeface &type) override;
        double natural_height(Typeface &type, double width) override;

        void arrange(Typeface &type) override;

    private:
        struct Slot {
            Widget *who = nullptr;
            double main = 0.0;
        };

    protected:
        [[nodiscard]] double gap_total() const;

        // What each visible child gets along the main axis, in order.
        std::vector<double> share(Typeface &type, double room, double across) const;

        // What a child takes along a row. A layout that divides the row itself answers
        // its own share here, rather than writing a width onto the child.
        [[nodiscard]] virtual double main_of(const Ptr &child, Typeface &type) const;

    private:

    protected:
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
