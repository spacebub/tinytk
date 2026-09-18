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
    // A caption, a box, and whatever hangs off it.
    class Field : public Box {
    public:
        Field(std::string label, std::function<void(const std::string &)> edited);

        Field *placeholder(std::string text);
        Field *value(const std::string &text);
        Field *leading_glyph(Glyphs::Glyph glyph);
        Field *prefix(std::string text);
        Field *mono(bool value = true);
        Field *read_only(bool value = true);
        Field *secret(bool value = true);
        Field *note(std::string text);
        Field *badge(std::string text, Pill::Kind kind);

        // A glyph button inside the box, or a full button beside it.
        Field *icon(Glyphs::Glyph glyph, std::string text, std::function<void()> pressed);
        Field *action(std::string label, std::function<void()> pressed);

        void set_text(const std::string &text) const;

        // Takes the glyph button out of the box and puts it back, for one that only
        // applies some of the time.
        void set_icon_visible(bool value) const;
        void set_icon_hint(std::string text) const;

        [[nodiscard]] const std::string &text() const { return _input->text(); }

        [[nodiscard]] TextBox *input() const { return _input; }

        void take_focus() const;

        void paint(const Painter &painter) override;

        void arrange(Typeface &type) override;

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
