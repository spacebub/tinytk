// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_SELECT_H
#define TTK_CONTROLS_SELECT_H


#include <functional>
#include <string>
#include <vector>

#include "ttk/draw/Anim.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/layout/Box.h"

namespace ttk {
    //! Drop-down that picks one option from a list of strings.
    //!
    //! The control is a column holding an optional caption above a field of \ref Theme::control height. Only the
    //! field takes the pointer. A click on it opens the option list on the root's popup layer, below the field
    //! when it fits in the window and above it otherwise. The list shows up to 9 rows and scrolls beyond that.
    //!
    //! Picking an option does not change \ref current() by itself. It reports the index through the `selected`
    //! callback, and the owner calls \ref set_current() to accept it.
    class Select : public Box {
    public:
        //! Creates a select with the caption `label`, hidden when empty, that reports picks to `selected`.
        //!
        //! `selected` is called with the index of the option picked from the list or stepped to with the arrow
        //! keys, or with -1 for the placeholder row of a \ref clearable() select. A picked option closes the list
        //! before `selected` is called.
        Select(std::string label, std::function<void(int)> selected);

        //! Sets the options shown in the list and repaints the field.
        //!
        //! \ref current() is kept as is, and an index out of range shows the placeholder. A list already open
        //! keeps the options it was opened with.
        void set_options(std::vector<std::string> options);

        //! Sets a short tag drawn at the right of each option, matched to the options by index.
        //!
        //! An option with no entry, because `badges` is shorter than the options, or with an empty string has no
        //! badge. The badge of \ref current() is also drawn in the field.
        void set_badges(std::vector<std::string> badges);

        //! Sets the index of the option shown in the field without calling the `selected` callback.
        //!
        //! A negative or out of range `index` shows the placeholder.
        void set_current(int index);

        //! Returns the index of the option shown in the field, -1 when none was set.
        [[nodiscard]] int current() const { return _current; }

        //! Sets the text shown in the field while no option is current, `(Default)` unless set, and returns this.
        Select *placeholder(std::string text);

        //! Sets whether the list starts with the placeholder as a row of its own, and returns this.
        //!
        //! Picking that row, or stepping up to it with the arrow keys, calls the `selected` callback with -1.
        //! Takes effect the next time the list opens.
        Select *clearable(bool value = true);

        //! Sets \ref Widget::hint to `text` and returns this.
        Select *tooltip(std::string text);

        //! Tests whether the option list is open.
        [[nodiscard]] bool open() const { return _list != nullptr; }

        //! Closes the option list and releases the root's dismiss handler. Does nothing when the list is not open.
        void close();

        //! Lays out the caption and the field, and records the field's box for painting and hit testing.
        void arrange(Typeface &type) override;

        //! Returns \ref Widget::fixedWidth when it is set, otherwise the column's width raised to 200 pixels.
        double natural_width(Typeface &type) override;

        //! Paints the field with the current option or the placeholder, its badge and a chevron that turns while
        //! the list is open, then the caption.
        void paint(const Painter &painter) override;

        //! Takes the press when the select is enabled.
        bool press(const Pointer &at) override;

        //! Opens the list, or closes it when it is open, when `at` is still inside the box.
        //!
        //! Opening does nothing when there are no options or the select is not attached to a root.
        void release(const Pointer &at) override;

        //! Called when the pointer moves onto the field. Starts the hover tint.
        void enter() override;

        //! Called when the pointer moves off the field. Fades the hover tint.
        void leave() override;

        //! Tests whether the select can receive keyboard focus, which it can while enabled.
        [[nodiscard]] bool takes_focus() const override { return enabled(); }

        //! Handles Return and Space, which open or close the list, and Down and Up, which call the `selected`
        //! callback with the next or previous index. Returns false for other keys and at either end of the
        //! options, where the top end is the placeholder row of a \ref clearable() select.
        bool key(const Key &pressed) override;

        //! Steps the hover and chevron animations and returns whether either is still running.
        bool advance(double now) override;

        //! Returns this select when `[x, y]` is inside the field and the select is visible and enabled, otherwise
        //! null. The caption does not take the pointer.
        Widget *at(double x, double y) override;

    private:
        void show();

        Label *_caption = nullptr;

        BLRect _frame{};

        std::vector<std::string> _options;
        std::vector<std::string> _badges;

        std::string _placeholder = "(Default)";

        int _current = -1;
        bool _clearable = false;

        std::function<void(int)> _selected;

        // Owned by the popup layer while it is up.
        Widget *_list = nullptr;

        Anim::Tween _lit;
        Anim::Tween _turn;
    };
}


#endif //TTK_CONTROLS_SELECT_H
