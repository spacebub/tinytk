// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_CARD_H
#define TTK_CONTROLS_CARD_H


#include <functional>
#include <string>
#include <vector>

#include "ttk/draw/Anim.h"
#include "ttk/toolkit/controls/Pill.h"
#include "ttk/toolkit/layout/Box.h"
#include "ttk/toolkit/layout/Panel.h"

namespace ttk {
    //! Tile for a \ref CardGrid with a title, a subtitle or file path, badges along the top, a status line and a
    //! row of buttons along the bottom.
    //!
    //! The public fields are read at paint time. Call \ref invalidate() after changing them. A \ref CardGrid sets
    //! \ref draggable and the drag callbacks on the cards added to it, and moves them between cells with
    //! \ref slide_from().
    class Card : public Panel {
    public:
        //! Badge drawn along the top edge of a card.
        struct Tag {
            //! Text of the badge.
            std::string text;
            //! Colour scheme of the badge, as for \ref Pill::kind().
            Pill::Kind kind = Pill::Kind::None;
            //! Whether a dot is drawn before the text.
            bool dot = false;
            //! Tooltip shown while the pointer rests on the badge. Empty for none.
            std::string hint;
        };

        Card();

        //! Whether a press on the card can turn into a drag that calls \ref drag_started.
        bool draggable = false;

        //! Called once when a press on a \ref draggable card has moved 6 pixels or more along either axis, with
        //! the `[x, y]` the press started at, in window coordinates.
        std::function<void(double, double)> drag_started;

        //! Called with the pointer `[x, y]` in window coordinates on every pointer motion of a drag, starting with
        //! the motion that started it.
        std::function<void(double, double)> drag_moved;

        //! Called when a drag started by \ref drag_started is released.
        std::function<void()> drag_ended;

        //! Called when a press that did not become a drag is released inside the card and above its button row.
        std::function<void()> opened;

        //! Title drawn at the top left in semibold type, left of the badges.
        std::string title;

        //! Text wrapped under the title in small faint type. It shares its place with \ref file, so set only one.
        std::string subtitle;

        //! File path drawn under the title in small fixed width type, with the home directory written as `~` and
        //! leading directories dropped to fit. It shares its place with \ref subtitle, so set only one.
        std::string file;

        //! Badges drawn right to left from the top right corner, the last one rightmost.
        std::vector<Tag> tags;

        //! Status line drawn above the button row. Hidden while \ref working is set.
        std::string told;

        //! Whether the title and \ref told are drawn in the danger colour.
        bool trouble = false;

        //! Whether a progress bar is drawn in place of \ref told.
        bool working = false;

        //! Fraction of the progress bar that is filled while \ref working is set, clamped to `[0, 1]`.
        double progress = 0.0;

        //! Returns the row along the bottom edge that holds the card's buttons. It starts empty.
        [[nodiscard]] Box *buttons() const { return _row; }

        //! Returns the grid cell the card was last placed in, or -1 before its first placement.
        [[nodiscard]] int slot() const { return _slot; }

        //! Sets the grid cell the card is placed in to `at`.
        void set_slot(const int at) { _slot = at; }

        //! Starts a slide of the card from an offset of `[x, y]` pixels back to its cell, beginning at `now`.
        //!
        //! The offset eases to zero over a fixed 0.19 seconds and is read through \ref slide_x() and
        //! \ref slide_y() by the grid, which adds it to the card's box.
        void slide_from(double x, double y, double now);

        //! Returns the current horizontal slide offset from the card's cell, in pixels.
        [[nodiscard]] double slide_x() const { return _slideX.value(); }

        //! Returns the current vertical slide offset from the card's cell, in pixels.
        [[nodiscard]] double slide_y() const { return _slideY.value(); }

        //! Tests whether a slide started by \ref slide_from() is in progress.
        [[nodiscard]] bool sliding() const { return _slideX.live() || _slideY.live(); }

        //! Tests whether the card is being dragged.
        [[nodiscard]] bool carrying() const { return _carrying; }

        //! Places the button row along the bottom edge, inset 16 pixels. Other children are not placed.
        void arrange(Typeface &type) override;

        //! Paints the panel and its buttons, lit while the pointer is on the card or one of its buttons or while it
        //! is dragged, then the badges, title, subtitle or file, and the status line or progress bar.
        void paint(const Painter &painter) override;

        //! Takes every press and remembers where it started, so it can become a drag or an \ref opened click.
        bool press(const Pointer &at) override;

        //! Starts a drag once the pointer has moved far enough from the press on a \ref draggable card, then
        //! reports each motion to \ref drag_moved.
        void drag(const Pointer &at) override;

        //! Ends the press, calling \ref drag_ended after a drag or \ref opened after a click.
        void release(const Pointer &at) override;

        //! Returns \ref Cursor::Grabbing while the card is dragged and \ref Cursor::Default otherwise.
        [[nodiscard]] Cursor cursor_at(double x, double y) const override;

        //! Sets \ref Widget::hint to the hint of the badge under `at`, or clears it when no badge is there.
        void hover(const Pointer &at) override;

        //! Called when the pointer moves off the card. Clears the hover state and \ref Widget::hint.
        void leave() override;

        //! Called when the pointer moves onto or off the card and its buttons. Repaints the card so its hover
        //! light follows the pointer onto the buttons.
        void within(bool inside) override;

        //! Steps the slide and repaints both where the card was and where it is. Returns \ref sliding().
        bool advance(double now) override;

    private:
        static constexpr double SLACK = 6.0;
        static constexpr double SETTLING = 0.19;

        Box *_row = nullptr;
        double _pressX = 0.0;
        double _pressY = 0.0;
        bool _armed = false;
        bool _carrying = false;
        int _slot = -1;

        // Where the badges were last drawn, so one can be rested on.
        std::vector<BLRect> _pills;

        Anim::Tween _slideX;
        Anim::Tween _slideY;
    };
}


#endif //TTK_CONTROLS_CARD_H
