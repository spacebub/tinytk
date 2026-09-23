// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>

#include "ttk/draw/Glyphs.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/controls/Button.h"
#include "ttk/toolkit/controls/Field.h"
#include "ttk/toolkit/controls/GlyphButton.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/controls/Pill.h"
#include "ttk/toolkit/controls/TextBox.h"
#include "ttk/toolkit/layout/Box.h"

namespace ttk {
    Field::Field(std::string label, std::function<void(const std::string &)> edited)
        : Box(Flow::Column) {
        spacing(6.0);

        minWidth = 240.0;

        _caption_row = append(Box::row());

        Box *caption = _caption_row;

        caption->fixedHeight = 22.0;
        caption->cross(Place::Centre);

        _caption = caption->append(std::make_unique<Label>(std::move(label)));
        _caption->section();
        _caption->stretch = 1.0;

        _badge = caption->append(std::make_unique<Pill>());
        _badge->set_visible(false);
        _badge->fixedHeight = 22.0;

        caption->set_visible(!_caption->text().empty());

        _row = append(Box::row());
        _row->spacing(8.0);
        _row->fixedHeight = Theme::control;
        _row->cross(Place::Centre);

        _input = _row->append(std::make_unique<TextBox>(std::move(edited)));
        _input->stretch = 1.0;
        _input->fixedHeight = Theme::control - 12.0;

        _input->accepted = [this] {
            if (accepted) {
                accepted();
            }
        };

        _note = append(std::make_unique<Label>());
        _note->font(400, Theme::fontSmall)->tone(&Theme::Palette::faint)->wrap();
        _note->set_visible(false);
    }

    Field *Field::placeholder(std::string text) {
        _input->placeholder(std::move(text));

        return this;
    }

    Field *Field::value(const std::string &text) {
        _input->set_text(text);

        return this;
    }

    Field *Field::leading_glyph(const Glyphs::Glyph glyph) {
        _leadingGlyph = glyph;

        return this;
    }

    Field *Field::prefix(std::string text) {
        _prefix = std::move(text);

        return this;
    }

    Field *Field::mono(const bool value) {
        _input->mono(value);

        return this;
    }

    Field *Field::read_only(const bool value) {
        _input->read_only(value);

        return this;
    }

    Field *Field::secret(const bool value) {
        _input->secret(value);

        return this;
    }

    Field *Field::note(std::string text) {
        _note->set_text(std::move(text));
        _note->set_visible(!_note->text().empty());

        return this;
    }

    Field *Field::badge(std::string text, const Pill::Kind kind) {
        const bool shown = !text.empty();

        _badge->set_text(std::move(text));
        _badge->kind(kind);
        _badge->set_visible(shown);

        _caption_row->set_visible(!_caption->text().empty() || shown);

        return this;
    }

    Field *Field::icon(Glyphs::Glyph glyph, std::string text, std::function<void()> pressed) {
        _icon = _row->append(std::make_unique<GlyphButton>(glyph, std::move(pressed)));
        _icon->size(30.0)->tooltip(std::move(text));
        _icon->fixedWidth = 30.0;
        _icon->fixedHeight = 30.0;

        return this;
    }

    Field *Field::action(std::string label, std::function<void()> pressed) {
        _action = _row->append(std::make_unique<Button>(std::move(label), std::move(pressed)));
        _action->compact();

        return this;
    }

    void Field::set_text(const std::string &text) const {
        _input->set_text(text);
    }

    void Field::set_icon_visible(const bool value) const {
        if (_icon != nullptr && _icon->visible() != value) {
            _icon->set_visible(value);
        }
    }

    void Field::set_icon_hint(std::string text) const {
        if (_icon != nullptr) {
            _icon->tooltip(std::move(text));
        }
    }

    void Field::take_focus() const {
        if (root() != nullptr) {
            root()->focus(_input);
            _input->select_all();
        }
    }

    void Field::arrange(Typeface &type) {
        Box::arrange(type);

        // The box wraps the input, the leading glyph and the icon, but not the action
        // button beside it.
        const BLRect row = _row->box();
        double right = row.x + row.w;

        if (_action != nullptr && _action->visible()) {
            right = _action->box().x - 8.0;
        }

        _frame = BLRect{row.x, row.y, std::max(0.0, right - row.x), Theme::control};

        double left = _frame.x + 12.0;

        if (_leadingGlyph != Glyphs::Glyph::Empty) {
            left += (12.0 * 1.1) + 9.0;
        }

        if (!_prefix.empty()) {
            left += type.width(type.at(Typeface::mono, Theme::fontBody), _prefix) + 19.0;
        }

        const double inset = _icon != nullptr && _icon->visible() ? 42.0 : 12.0;

        _input->place(BLRect{left, _frame.y + 6.0, std::max(0.0, _frame.x + _frame.w - inset - left),
                             _frame.h - 12.0},
                      type);

        if (_icon != nullptr && _icon->visible()) {
            _icon->place(BLRect{_frame.x + _frame.w - 34.0, _frame.y + ((_frame.h - 30.0) / 2.0), 30.0,
                                30.0},
                         type);
        }
    }

    void Field::paint(const Painter &painter) {
        const Theme::Palette &palette = Theme::palette();

        painter.round(_frame, Theme::radiusSmall, palette.field);
        painter.outline(_frame, Theme::radiusSmall, 1.0,
                        _input->focused() ? palette.accent : palette.borderStrong);

        if (_leadingGlyph != Glyphs::Glyph::Empty) {
            constexpr float weight = 1.1F;
            const double side = Glyphs::span(weight);

            Glyphs::draw(painter.context(), _leadingGlyph,
                         BLPoint{_frame.x + 12.0, _frame.y + ((_frame.h - side) / 2.0)}, weight,
                         _input->focused() ? palette.accent : palette.faint);
        }

        if (!_prefix.empty()) {
            const BLFont &face = painter.font(Typeface::mono, Theme::fontBody);
            const double taken = painter.width(face, _prefix);
            const double at = _frame.x + 12.0 + (_leadingGlyph == Glyphs::Glyph::Empty ? 0.0 : (12.0 * 1.1) + 9.0);

            painter.label(face, BLRect{at, _frame.y, taken + 2.0, _frame.h}, Align::Start, _prefix,
                          palette.faint);
            painter.fill(BLRect{at + taken + 9.0, _frame.y + 6.0, 1.0, _frame.h - 12.0},
                         palette.borderStrong);
        }

        Box::paint(painter);
    }
}
