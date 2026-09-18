// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_TEXTVIEW_H
#define TTK_CONTROLS_TEXTVIEW_H


#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
    // Read-only type the pointer can select and copy: the output of a run, and the
    // command a launch would use. Held by a Scroll, which clips and offsets it.
    class TextView : public Widget {
    public:
        TextView();

        // Lines already broken, as a run's output arrives.
        void set_rows(std::vector<std::string> rows);

        // The rows already held, plus these. For a log that only ever grows.
        void add_rows(std::vector<std::string> rows);

        // One run, folded to the width it is given.
        void set_run(std::string run);

        TextView *face(int weight, float size);

        // What each row is drawn in, asked at paint so a change of theme is picked up.
        TextView *ink(std::function<BLRgba32(size_t row)> pick);

        [[nodiscard]] size_t rows() const { return _rows.size(); }

        void select_all();
        void clear_selection();

        // What is selected, or nothing at all when no two spots differ.
        [[nodiscard]] std::string selection() const;

        double natural_height(Typeface &type, double width) override;

        void arrange(Typeface &type) override;

        void paint(const Painter &painter) override;

        bool press(const Pointer &at) override;
        void drag(const Pointer &at) override;

        bool key(const Key &pressed) override;

        [[nodiscard]] bool takes_focus() const override { return true; }

        void lost_focus() override;

    private:
        // A place in the text: which row, and how far into it in bytes.
        struct Spot {
            size_t row = 0;
            size_t at = 0;

            auto operator<=>(const Spot &) const = default;
            bool operator==(const Spot &) const = default;
        };

        [[nodiscard]] const BLFont &font(Typeface &type) const;

        void refold(Typeface &type, double width);

        [[nodiscard]] Spot spot_at(Typeface &type, double x, double y) const;

        // The two spots in reading order.
        void span(Spot &from, Spot &to) const;

        // Holds a selection inside rows that have since changed under it.
        void clamp();

        void move_to(const Spot &where, bool selecting);

        std::vector<std::string> _rows;

        // Where each row sits in `_run`, so a part of one reads back off the run.
        std::vector<std::pair<size_t, size_t>> _spans;

        std::string _run;
        bool _wrapped = false;

        // The width `_rows` was folded at, so a relayout to the same width costs nothing.
        double _folded = -1.0;

        int _weight = Typeface::mono;
        float _size = Theme::fontSmall;

        std::function<BLRgba32(size_t)> _ink;

        Spot _anchor;
        Spot _caret;
    };
}


#endif //TTK_CONTROLS_TEXTVIEW_H
