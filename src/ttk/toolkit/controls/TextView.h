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
#include <string_view>
#include <utility>
#include <vector>

#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
    //! Read-only text whose rows can be selected with the pointer and copied.
    //!
    //! The view holds either rows given by the caller, through \ref set_rows(), \ref add_rows() and \ref append(),
    //! or a single run of text given through \ref set_run(), which it folds into rows at the width it is placed at.
    //! Rows given by the caller are drawn as they are and never wrapped. The view is as tall as all of its rows and
    //! does not scroll, so a long text is placed inside a \ref Scroll. Only the rows inside the area being
    //! repainted are drawn. The text is UTF-8.
    class TextView : public Widget {
    public:
        //! Creates an empty view in the monospace face at \ref Theme::fontSmall.
        TextView();

        //! Replaces the content with `rows`, one string per row with no line breaks.
        //!
        //! Discards any run given to \ref set_run(). The selection is kept, clamped to the new rows. Does nothing
        //! when `rows` equals the rows held and no run is set.
        void set_rows(std::vector<std::string> rows);

        //! Appends `rows` after the rows held. Does nothing when `rows` is empty.
        //!
        //! A run given to \ref set_run() is discarded first, together with the rows folded from it. A row left
        //! open by \ref append() is closed, so the next \ref append() starts a new row.
        void add_rows(std::vector<std::string> rows);

        //! Appends `text`, such as process output as it arrives, splitting it into rows.
        //!
        //! A `\n` ends the current row, and a `\r\n` is treated as `\n`. A lone `\r` restarts the current row, so
        //! the row keeps only what follows the last `\r`. Text after the last `\n` stays in an open row that the
        //! next call continues. A run given to \ref set_run() is discarded first. Does nothing when `text` is
        //! empty.
        void append(std::string_view text);

        //! Removes the first `count` rows, or all of them when there are fewer.
        //!
        //! The selection moves up with the rows it was on, and an end of it that was on a removed row moves to the
        //! start of the first row. Does nothing while a run from \ref set_run() is held, since its rows are folded
        //! again at each layout.
        void drop_rows(size_t count);

        //! Returns the height of one row in pixels, which is the line height of the face. A caller that drops
        //! rows from a scrolled view uses it to know how far the remaining rows moved up.
        [[nodiscard]] double row_height(Typeface &type) const;

        //! Replaces the content with the single run of text `run`, folded into rows at the width the view is
        //! placed at.
        //!
        //! Rows break at `\n` and at spaces, and a word wider than the view is cut between characters. The
        //! selection is cleared. Does nothing when `run` equals the run already set.
        void set_run(std::string run);

        //! Sets the face to `weight` at `size` and returns this. A run is folded again at the next layout.
        TextView *face(int weight, float size);

        //! Sets the function that returns the colour of each row, given its index, and returns this.
        //!
        //! `pick` is called for every row drawn at each paint, so it can follow theme changes. While it is null,
        //! rows are drawn in the theme's text colour.
        TextView *ink(std::function<BLRgba32(size_t row)> pick);

        //! Returns the number of rows, which for a run is the number of rows it was last folded into.
        [[nodiscard]] size_t rows() const { return _rows.size(); }

        //! Selects all rows.
        void select_all();

        //! Clears the selection and repaints when there was one.
        void clear_selection();

        //! Returns the selected text, or an empty string when nothing is selected.
        //!
        //! For a run, the text is copied from the run itself, so its own line breaks and spaces come back as they
        //! were given. Otherwise the selected parts of the rows are joined with `\n`.
        [[nodiscard]] std::string selection() const;

        //! Folds a run to `width`, then returns the height of all rows.
        double natural_height(Typeface &type, double width) override;

        //! Folds a run to the width of the box when that width has changed.
        void arrange(Typeface &type) override;

        //! Paints the rows inside the area being repainted, with the selection behind them.
        void paint(const Painter &painter) override;

        //! Takes focus and moves the selection point to the character nearest `at`, extending the selection when
        //! Shift is held. Returns false when the view is not attached to a root.
        bool press(const Pointer &at) override;

        //! Extends the selection to the character nearest `at`.
        void drag(const Pointer &at) override;

        //! Handles Ctrl+A, which selects all, and Ctrl+C, which copies the selection. Returns false for other keys.
        bool key(const Key &pressed) override;

        //! Tests whether the view can receive keyboard focus, which it always can.
        [[nodiscard]] bool takes_focus() const override { return true; }

        //! Called when the view loses keyboard focus. Clears the selection.
        void lost_focus() override;

    private:
        struct Spot {
            size_t row = 0;
            size_t at = 0;

            auto operator<=>(const Spot &) const = default;
            bool operator==(const Spot &) const = default;
        };

        [[nodiscard]] const BLFont &font(Typeface &type) const;

        void refold(Typeface &type, double width);

        void unwrap();

        [[nodiscard]] Spot spot_at(Typeface &type, double x, double y) const;

        void span(Spot &from, Spot &to) const;

        void clamp();

        void move_to(const Spot &where, bool selecting);

        std::vector<std::string> _rows;

        std::vector<std::pair<size_t, size_t>> _spans;

        std::string _run;
        bool _wrapped = false;

        bool _open = false;

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
