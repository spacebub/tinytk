// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <utility>

#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/controls/Chip.h"

namespace ttk {
    namespace {

        constexpr double PAD = 7.0;
        constexpr double LIFT = 3.0;

    }

    Chip::Chip(std::string text, std::string about) : _text(std::move(text)) {
        _takesPointer = true;
        hint = std::move(about);
    }

    void Chip::set_text(std::string text) {
        if (_text == text) {
            return;
        }

        _text = std::move(text);

        invalidate();
    }

    void Chip::set_tight(const bool value) {
        if (_tight == value) {
            return;
        }

        _tight = value;

        invalidate();
    }

    Chip *Chip::plain() {
        _mono = false;

        return this;
    }

    const BLFont &Chip::face(Typeface &type) const {
        return type.at(_mono ? Typeface::mono : Typeface::regular, Theme::fontSmall);
    }

    double Chip::natural_width(Typeface &type) {
        if (fixedWidth >= 0.0) {
            return fixedWidth;
        }

        return type.width(face(type), _text) + (PAD * 2.0);
    }

    double Chip::natural_height(Typeface &type, const double /*width*/) {
        if (fixedHeight >= 0.0) {
            return fixedHeight;
        }

        return type.line_height(face(type)) + (LIFT * 2.0);
    }

    void Chip::paint(const Painter &painter) {
        if (_text.empty()) {
            return;
        }

        const Theme::Palette &palette = Theme::palette();
        const BLRgba32 ink = _tight    ? palette.warning
                           : hovered() ? palette.text
                                       : palette.faint;

        if (_tight || hovered()) {
            painter.round(_box, Theme::radiusSmall, _tight ? palette.warningSoft : palette.raised);
            painter.outline(_box, Theme::radiusSmall, 1.0, Theme::alpha(ink, 0.3));
        }

        painter.label(painter.font(_mono ? Typeface::mono : Typeface::regular, Theme::fontSmall),
                      BLRect{_box.x + PAD, _box.y, _box.w - (PAD * 2.0), _box.h}, Align::Start,
                      _text, ink);
    }
}
