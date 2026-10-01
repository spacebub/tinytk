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
    //! Single-line text field.
    //!
    //! The field paints only its text, caret and selection, clipped to its box, with no frame of its own. The
    //! text is UTF-8 and the caret moves by whole code points. When the text is wider than the box, it scrolls
    //! sideways to keep the caret in view.
    class TextBox : public Widget {
    public:
        //! Creates an empty field that reports edits to `edited`.
        //!
        //! `edited` is called with the whole text after every change made from the keyboard or the clipboard, but
        //! not after \ref set_text().
        explicit TextBox(std::function<void(const std::string &)> edited);

        //! Returns the text, in UTF-8.
        [[nodiscard]] const std::string &text() const { return _text; }

        //! Sets the text to `text` without calling the `edited` callback, and repaints.
        //!
        //! The caret is kept where it was, clamped to the end of the new text, and the selection is cleared. Does
        //! nothing when `text` equals the current text.
        void set_text(std::string text);

        //! Sets the text shown in a faint colour while the field is empty, and returns this.
        TextBox *placeholder(std::string text);

        //! Sets whether the text is drawn in the fixed-width face, and returns this.
        TextBox *mono(bool value = true);

        //! Sets whether the text is protected from editing, and returns this.
        //!
        //! A read-only field still takes focus, moves the caret, selects and copies. Typing, pasting, cutting and
        //! deleting are ignored.
        TextBox *read_only(bool value = true);

        //! Sets whether the text is drawn as one bullet per character, as for a password, and returns this.
        //!
        //! Only the drawing changes. \ref text() and copying to the clipboard still give the real text.
        TextBox *secret(bool value = true);

        //! Selects the whole text, with the caret at the end.
        void select_all();

        //! Called when Return is pressed while the field has focus.
        std::function<void()> accepted;

        //! Called when Escape is pressed while the field has focus. While it is null, Escape is not consumed and
        //! passes on to the parent.
        std::function<void()> cancelled;

        //! Returns \ref Widget::fixedWidth when it is set, otherwise 120 pixels.
        double natural_width(Typeface &type) override;

        //! Returns \ref Widget::fixedHeight when it is set, otherwise the line height of the body font.
        double natural_height(Typeface &type, double width) override;

        //! Paints the text, or the placeholder when the text is empty, and the caret and selection while focused.
        void paint(const Painter &painter) override;

        //! Takes focus and moves the caret to the character boundary nearest `at`, extending the selection when
        //! Shift is held. Returns false when the field is disabled.
        bool press(const Pointer &at) override;

        //! Extends the selection to the character boundary nearest `at`.
        void drag(const Pointer &at) override;

        //! Handles editing and caret keys and returns true when the key was consumed.
        //!
        //! Left and Right move by a character, or by a word with Ctrl, and Home and End to either end. Shift
        //! extends the selection. Backspace and Delete remove the selection, or a character or a word with Ctrl.
        //! Ctrl+A selects all, Ctrl+C copies, Ctrl+X cuts and Ctrl+V pastes. Return calls \ref accepted and is
        //! always consumed. Escape calls \ref cancelled when it is set. A word is a run of ASCII letters, digits,
        //! underscores and non-ASCII characters.
        bool key(const Key &pressed) override;

        //! Replaces the selection with `text`, in UTF-8, or inserts it at the caret. Ignored when read-only.
        void wrote(const std::string &text) override;

        //! Tests whether the field can receive keyboard focus, which it can while enabled.
        [[nodiscard]] bool takes_focus() const override { return enabled(); }

        //! Called when the field receives focus. Shows the caret, starts it blinking and repaints the parent, which
        //! may draw a focus border around the field.
        void gained_focus() override;

        //! Called when the field loses focus. Clears the selection, scrolls back to the start of the text and
        //! repaints the parent.
        void lost_focus() override;

        //! Blinks the caret while the field has focus, sleeping between blinks. Returns false once focus is lost.
        bool advance(double now) override;

    private:
        size_t offset_at(Typeface &type, double x) const;

        double width_to(Typeface &type, size_t offset) const;

        void move_to(size_t offset, bool selecting);

        void erase(size_t from, size_t to);

        void insert(const std::string &what);

        void span(size_t &from, size_t &to) const;

        [[nodiscard]] size_t before(size_t at) const;
        [[nodiscard]] size_t after(size_t at) const;

        [[nodiscard]] size_t word_left(size_t at) const;
        [[nodiscard]] size_t word_right(size_t at) const;

        void keep_caret(Typeface &type);

        [[nodiscard]] std::string_view shown(size_t from, size_t to) const;

        std::string _text;
        std::string _placeholder;

        std::function<void(const std::string &)> _edited;

        size_t _caret = 0;
        size_t _anchor = 0;

        double _shift = 0.0;

        bool _mono = false;
        bool _readOnly = false;
        bool _secret = false;
        mutable std::string _dots;

        double _blinked = 0.0;
        bool _showCaret = true;
    };
}


#endif //TTK_CONTROLS_TEXTBOX_H
