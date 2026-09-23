// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <utility>

#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/controls/GlyphButton.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/controls/Stepper.h"

#include "ttk/draw/Glyphs.h"
#include "ttk/toolkit/layout/Box.h"
#include "ttk/toolkit/layout/Spacer.h"

namespace ttk {
    Stepper::Stepper(std::string label, std::function<void(int)> stepped)
        : Box(Flow::Column), _stepped(std::move(stepped)) {
        spacing(6.0);

        minWidth = 170.0;

        _caption = append(std::make_unique<Label>(std::move(label)));
        _caption->section();
        _caption->set_visible(!_caption->text().empty());

        Box *frame = append(Box::row());

        frame->fixedHeight = Theme::control;
        frame->pad(4.0, 0.0);
        frame->cross(Place::Centre);

        _less = frame->append(std::make_unique<GlyphButton>(Glyphs::Glyph::Minus, [this] { step(-1); }));
        _less->size(30.0);
        _less->fixedWidth = 30.0;
        _less->fixedHeight = 30.0;

        frame->append(std::make_unique<Spacer>());

        _more = frame->append(std::make_unique<GlyphButton>(Glyphs::Glyph::Plus, [this] { step(1); }));
        _more->size(30.0);
        _more->fixedWidth = 30.0;
        _more->fixedHeight = 30.0;
    }

    Stepper *Stepper::range(const int from, const int to) {
        _from = from;
        _to = to;

        return this;
    }

    Stepper *Stepper::clearable(const int offValue, std::string placeholder) {
        _clearable = true;
        _off = offValue;
        _placeholder = std::move(placeholder);

        return this;
    }

    Stepper *Stepper::tooltip(std::string text) {
        hint = std::move(text);

        return this;
    }

    void Stepper::set_value(const int value) {
        if (_value == value) {
            return;
        }

        _value = value;

        invalidate();
    }

    void Stepper::step(const int by) const {
        if (unset()) {
            if (by > 0 && _stepped) {
                _stepped(_from);
            }

            return;
        }

        if (_value + by < _from) {
            if (_clearable && _stepped) {
                _stepped(_off);
            }
        } else if (_value + by <= _to && _stepped) {
            _stepped(_value + by);
        }
    }

    double Stepper::natural_width(Typeface &type) {
        return fixedWidth >= 0.0 ? fixedWidth : std::max(170.0, Box::natural_width(type));
    }

    void Stepper::arrange(Typeface &type) {
        Box::arrange(type);

        _frame = children().back()->box();

        _less->set_enabled(enabled() && !unset() && (_clearable || _value > _from));
        _more->set_enabled(enabled() && (unset() || _value < _to));
    }

    void Stepper::paint(const Painter &painter) {
        const Theme::Palette &palette = Theme::palette();
        const double dim = enabled() ? 1.0 : 0.45;

        painter.round(_frame, Theme::radiusSmall, Theme::alpha(palette.field, dim));
        painter.outline(_frame, Theme::radiusSmall, 1.0,
                        Theme::alpha(hovered() ? palette.borderStrong : palette.border, dim));

        const std::string said = unset() ? _placeholder : std::to_string(_value);

        painter.label(painter.font(Typeface::pick(unset() ? 400 : 600, !unset()), Theme::fontBody),
                      _frame, Align::Centre, said,
                      Theme::alpha(unset() ? palette.faint : palette.text, dim));

        Box::paint(painter);
    }
}
