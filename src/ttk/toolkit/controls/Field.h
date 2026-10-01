// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_FIELD_H
#define TTK_CONTROLS_FIELD_H


#include <functional>
#include <string>

#include "ttk/toolkit/controls/Button.h"
#include "ttk/toolkit/controls/GlyphButton.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/controls/Pill.h"
#include "ttk/toolkit/controls/TextBox.h"
#include "ttk/toolkit/layout/Box.h"

namespace ttk {
    //! Labelled single line text input.
    //!
    //! A column of a caption row with an optional badge, a framed \ref TextBox with optional leading glyph, prefix
    //! and glyph button inside the frame, an optional button beside the frame, and an optional note below. The
    //! field is at least 240 pixels wide.
    class Field : public Box {
    public:
        //! Creates a field captioned `label` whose input calls `edited` with the whole text after every edit made
        //! by the user. The caption row is hidden while `label` is empty and no badge is shown.
        Field(std::string label, std::function<void(const std::string &)> edited);

        //! Sets the text shown in the empty input and returns this field.
        Field *placeholder(std::string text);

        //! Sets the input text to `text` without calling the edit callback and returns this field.
        Field *value(const std::string &text);

        //! Sets the glyph drawn at the left of the frame and returns this field. \ref Glyphs::Glyph::Empty shows
        //! none. Takes effect at the next layout.
        Field *leading_glyph(Glyphs::Glyph glyph);

        //! Sets fixed text drawn in faint fixed width type before the input, followed by a divider, and returns
        //! this field. Empty shows none. Takes effect at the next layout.
        Field *prefix(std::string text);

        //! Sets whether the input uses the fixed width face and returns this field.
        Field *mono(bool value = true);

        //! Sets whether the input refuses edits and returns this field.
        Field *read_only(bool value = true);

        //! Sets whether the input shows one bullet per character, for a password, and returns this field.
        Field *secret(bool value = true);

        //! Sets the wrapped note shown below the input and returns this field. Empty hides the note.
        Field *note(std::string text);

        //! Shows `text` as a badge of style `kind` at the right of the caption row and returns this field. Empty
        //! hides the badge.
        Field *badge(std::string text, Pill::Kind kind);

        //! Adds a 30 pixel glyph button inside the right end of the frame and returns this field.
        //!
        //! The button shows `glyph`, has `text` as its tooltip and calls `pressed` when clicked. Call it once per
        //! field, as only the last button added is managed by \ref set_icon_visible() and \ref set_icon_hint().
        Field *icon(Glyphs::Glyph glyph, std::string text, std::function<void()> pressed);

        //! Adds a compact \ref Button labelled `label` beside the frame that calls `pressed` when clicked, and
        //! returns this field.
        Field *action(std::string label, std::function<void()> pressed);

        //! Sets the input text to `text` without calling the edit callback.
        void set_text(const std::string &text) const;

        //! Shows or hides the glyph button added by \ref icon(). Does nothing when there is none.
        void set_icon_visible(bool value) const;

        //! Sets the tooltip of the glyph button added by \ref icon(). Does nothing when there is none.
        void set_icon_hint(std::string text) const;

        //! Returns the input text.
        [[nodiscard]] const std::string &text() const { return _input->text(); }

        //! Returns the text input the field wraps.
        [[nodiscard]] TextBox *input() const { return _input; }

        //! Gives keyboard focus to the input and selects all of its text. Does nothing while the field is not
        //! attached to a root.
        void take_focus() const;

        //! Paints the frame, outlined in the accent colour while the input has focus, the leading glyph and the
        //! prefix, then the children.
        void paint(const Painter &painter) override;

        //! Lays out the column, then fits the frame to the input row less the action button and places the input
        //! and glyph button inside it.
        void arrange(Typeface &type) override;

        //! Called when Return is pressed in the input.
        std::function<void()> accepted;

    private:
        // The box the input sits in, drawn by this widget rather than a child.
        BLRect _frame{};

        Box *_caption_row = nullptr;
        Label *_caption = nullptr;
        Pill *_badge = nullptr;
        TextBox *_input = nullptr;
        GlyphButton *_icon = nullptr;
        Button *_action = nullptr;
        Label *_note = nullptr;

        Box *_row = nullptr;

        Glyphs::Glyph _leadingGlyph{};
        std::string _prefix;
    };
}


#endif //TTK_CONTROLS_FIELD_H
