// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_OVERLAYS_DIALOG_H
#define TTK_OVERLAYS_DIALOG_H


#include <functional>
#include <string>

#include "ttk/draw/Anim.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/layout/Box.h"
#include "ttk/toolkit/layout/Panel.h"

namespace ttk {
    // The dialog itself only darkens what is under it, catches the press that
    // dismisses it, and keeps the rest of the window from answering the pointer.
    class Dialog : public Widget {
    public:
        Dialog();

        [[nodiscard]] Panel *card() const { return _card; }

        // What the card would like, before the window's own room is taken into account.
        double wanted = 460.0;
        double tall = 0.0;

        std::function<void()> dismissed;

        // False when the dialog dealt with the dismissal itself and means to stay up.
        virtual bool closing() { return true; }

        // Called once it is up and can reach the tree, for whatever wants the keyboard.
        virtual void opened() {}

        // Called each turn while it is up, for a dialog whose content moves under it.
        virtual void sync() {}

    protected:
        // Drawn over the card's children, under the same scale while the card grows.
        virtual void paint_over(const Painter & /*painter*/) {}

        // The heading and the paragraph under it, which every dialog opens with.
        static Label *heading(Box *into, const std::string &text);
        static Label *body(Box *into, const std::string &text);

    public:

        [[nodiscard]] bool open() const { return _open; }

    protected:
        void set_open(bool open);

    public:

        void arrange(Typeface &type) override;

        void paint(const Painter &painter) override;

        bool press(const Pointer &at) override;
        void release(const Pointer &at) override;

        Widget *at(double x, double y) override;

        bool advance(double now) override;

    private:
        Panel *_card = nullptr;

        bool _open = false;
        bool _onScrim = false;

        // The grow waits for the first layout, the first time the frame clock is in reach.
        bool _grow = false;

        Anim::Tween _grown;
    };
}


#endif //TTK_OVERLAYS_DIALOG_H
