// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_TEXTBOX_H
#define TTK_CONTROLS_TEXTBOX_H


#include <functional>
#include <string>
#include <string_view>

#include "ttk/toolkit/Widget.h"

namespace ttk {
    // The caret is a byte offset into the run, and the box scrolls sideways so the
    // caret stays inside.
    class TextBox : public Widget {
    public:
        explicit TextBox(std::function<void(const std::string &)> edited);

        [[nodiscard]] const std::string &text() const { return _text; }

        // Pushed, not bound: a set while the field has focus would fight the typing.
        void set_text(std::string text);

        TextBox *placeholder(std::string text);
        TextBox *mono(bool value = true);
        TextBox *read_only(bool value = true);

        // Drawn as one bullet per character, for a password.
        TextBox *secret(bool value = true);

        void select_all();

        // Return pressed.
        std::function<void()> accepted;

        // Escape pressed, for a field that closes something.
        std::function<void()> cancelled;

        double natural_width(Typeface &type) override;
        double natural_height(Typeface &type, double width) override;

        void paint(const Painter &painter) override;

        bool press(const Pointer &at) override;
        void drag(const Pointer &at) override;

        bool key(const Key &pressed) override;
        void wrote(const std::string &text) override;

        [[nodiscard]] bool takes_focus() const override { return enabled(); }
        void gained_focus() override;
        void lost_focus() override;

        bool advance(double now) override;

    private:
        // The byte offset nearest `x`.
        size_t offset_at(Typeface &type, double x) const;

        double width_to(Typeface &type, size_t offset) const;

        void move_to(size_t offset, bool selecting);

        void erase(size_t from, size_t to);

        void insert(const std::string &what);

        // The selection in order, or the caret twice over.
        void span(size_t &from, size_t &to) const;

        [[nodiscard]] size_t before(size_t at) const;
        [[nodiscard]] size_t after(size_t at) const;

        // The start of the word on either side, for ctrl-arrow and ctrl-backspace.
        [[nodiscard]] size_t word_left(size_t at) const;
        [[nodiscard]] size_t word_right(size_t at) const;

        void keep_caret(Typeface &type);

        // What is drawn for the bytes between `from` and `to`: the text itself, or a
        // bullet per character of it.
        [[nodiscard]] std::string_view shown(size_t from, size_t to) const;

        std::string _text;
        std::string _placeholder;

        std::function<void(const std::string &)> _edited;

        size_t _caret = 0;
        size_t _anchor = 0;

        // How far the run is pushed left so the caret stays in view.
        double _shift = 0.0;

        bool _mono = false;
        bool _readOnly = false;
        bool _secret = false;
        mutable std::string _dots;

        // Blinks while focused, which is the one thing that keeps the loop awake.
        double _blinked = 0.0;
        bool _showCaret = true;
    };
}


#endif //TTK_CONTROLS_TEXTBOX_H
