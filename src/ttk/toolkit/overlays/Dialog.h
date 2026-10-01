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
    //! Modal dialog: a scrim over everything under it with a card centred on top.
    //!
    //! The dialog takes every pointer event its card does not, so nothing under it answers the pointer, and a click
    //! on the scrim calls \ref dismissed. Subclasses fill \ref card(). Dialogs are normally shown through a
    //! \ref DialogLayer, which calls \ref opened(), \ref closing() and \ref sync().
    class Dialog : public Widget {
    public:
        //! Creates an open dialog with an empty card. The card grows into place from 95% scale on its first layout.
        Dialog();

        //! Returns the panel that holds the dialog's content.
        [[nodiscard]] Panel *card() const { return _card; }

        //! Width of the card in pixels, reduced to leave at least 24 pixels on each side of the dialog's box.
        double wanted = 460.0;
        //! Height of the card in pixels, or 0 to size it to its content at \ref wanted width. Either way it is reduced
        //! to leave at least 24 pixels above and below.
        double tall = 0.0;

        //! Called when the dialog asks to be closed: on a click on the scrim, and by subclasses for a Cancel button.
        //!
        //! A press and its release must both land outside the card. A press that closed a popup does not count, see
        //! \ref Root::just_dismissed(). \ref DialogLayer::show() sets it to \ref DialogLayer::close().
        std::function<void()> dismissed;

        //! Called by \ref DialogLayer::close() before taking the dialog down. Returns false to stay up, having dealt
        //! with the request itself. The default implementation returns true.
        virtual bool closing() { return true; }

        //! Called by \ref DialogLayer once the dialog is attached and on top, both when it is shown and when the
        //! dialog above it closes. Override it to take keyboard focus. The default implementation does nothing.
        virtual void opened() {}

        //! Called by \ref DialogLayer::sync() while the dialog is on top, for a dialog that reflects state changing
        //! under it. The default implementation does nothing.
        virtual void sync() {}

    protected:
        //! Called after the card and its children are painted, under the same scale while the card grows. The default
        //! implementation does nothing.
        virtual void paint_over(const Painter & /*painter*/) {}

        //! Appends a wrapped heading label with `text` to `into` and returns it.
        static Label *heading(Box *into, const std::string &text);
        //! Appends a wrapped body label with `text`, in the muted colour, to `into` and returns it.
        static Label *body(Box *into, const std::string &text);

    public:

        //! Tests whether the dialog is open. A new dialog is open.
        [[nodiscard]] bool open() const { return _open; }

    protected:
        //! Opens or closes the dialog, showing or hiding it. Opening again plays the grow on the next layout. Does
        //! nothing when the state does not change.
        void set_open(bool open);

    public:

        //! Centres the card in the dialog's box, sized by \ref wanted and \ref tall, and starts the grow when it is
        //! due.
        void arrange(Typeface &type) override;

        //! Fills the box with the scrim, then paints the card and \ref paint_over() scaled about the card's centre
        //! while it grows.
        void paint(const Painter &painter) override;

        //! Takes every press and notes whether it landed on the scrim.
        bool press(const Pointer &at) override;
        //! Calls \ref dismissed when both the press and `at` are outside the card.
        void release(const Pointer &at) override;

        //! Returns the card's widget at `[x, y]`, otherwise the dialog itself, wherever the point is. Returns null
        //! while the dialog is hidden.
        Widget *at(double x, double y) override;

        //! Steps the grow animation and repaints, returning true while it runs.
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
