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
    // A tile on a grid: a title, a line under it, badges along the top, a status
    // line and a row of buttons. A CardGrid carries it from one cell to another.
    class Card : public Panel {
    public:
        struct Tag {
            std::string text;
            Pill::Kind kind = Pill::Kind::None;
            bool dot = false;
            // Said when the badge is rested on.
            std::string hint;
        };

        Card();

        bool draggable = false;

        std::function<void(double, double)> drag_started;
        std::function<void(double, double)> drag_moved;
        std::function<void()> drag_ended;
        std::function<void()> opened;

        std::string title;
        std::string subtitle;
        // A path, fitted to the width by dropping leading directories.
        std::string file;
        std::vector<Tag> tags;

        // Under the buttons: what is known, or what went wrong.
        std::string told;
        bool trouble = false;

        // Shown as a bar instead, while something is being fetched.
        bool working = false;
        double progress = 0.0;

        [[nodiscard]] Box *buttons() const { return _row; }

        // The cell it was last given. A card walks to a new one, not to a scroll.
        [[nodiscard]] int slot() const { return _slot; }
        void set_slot(const int at) { _slot = at; }

        // The walk to a new cell, while the cards shuffle around a carried one.
        void slide_from(double x, double y, double now);
        [[nodiscard]] double slide_x() const { return _slideX.value(); }
        [[nodiscard]] double slide_y() const { return _slideY.value(); }
        [[nodiscard]] bool sliding() const { return _slideX.live() || _slideY.live(); }
        [[nodiscard]] bool carrying() const { return _carrying; }

        void arrange(Typeface &type) override;
        void paint(const Painter &painter) override;
        bool press(const Pointer &at) override;
        void drag(const Pointer &at) override;
        void release(const Pointer &at) override;
        [[nodiscard]] Cursor cursor_at(double x, double y) const override;
        void hover(const Pointer &at) override;
        void leave() override;
        void within(bool inside) override;
        bool advance(double now) override;

    private:
        // How far the pointer moves before a press becomes a drag.
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
