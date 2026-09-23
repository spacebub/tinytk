// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "ttk/draw/Glyphs.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/controls/Pill.h"

namespace ttk {
    Pill::Pill(std::string text) : _text(std::move(text)) {
        resolve();
    }

    void Pill::set_text(std::string text) {
        if (_text == text) {
            return;
        }

        _text = std::move(text);

        invalidate();
    }

    Pill *Pill::kind(const Kind value) {
        _tone = Theme::Tone(tone_slot(value));
        _wash = Theme::Tone(wash_slot(value));

        resolve();

        return this;
    }

    Pill *Pill::dot(const bool value) {
        _dot = value;

        return this;
    }

    Pill *Pill::glyph(const Glyphs::Glyph glyph) {
        _glyph = glyph;

        return this;
    }

    Pill *Pill::tones(const Theme::Tone tone, const Theme::Tone wash) {
        _tone = tone;
        _wash = wash;

        resolve();

        return this;
    }

    void Pill::restyle() {
        _tone.restyle();
        _wash.restyle();

        resolve();
    }

    void Pill::resolve() {
        _ink = Theme::palette().dark ? _tone.colour() : Theme::darker(_tone.colour(), 0.35);
    }

    BLRgba32 Theme::Palette::*Pill::tone_slot(const Kind kind) {
        switch (kind) {
            case Kind::Success:
                return &Theme::Palette::success;
            case Kind::Warning:
                return &Theme::Palette::warning;
            case Kind::Danger:
                return &Theme::Palette::danger;
            case Kind::Muted:
                return &Theme::Palette::muted;
            case Kind::None:
                break;
        }

        return &Theme::Palette::accent;
    }

    BLRgba32 Theme::Palette::*Pill::wash_slot(const Kind kind) {
        switch (kind) {
            case Kind::Success:
                return &Theme::Palette::successSoft;
            case Kind::Warning:
                return &Theme::Palette::warningSoft;
            case Kind::Danger:
                return &Theme::Palette::dangerSoft;
            case Kind::Muted:
                return &Theme::Palette::mutedSoft;
            case Kind::None:
                break;
        }

        return &Theme::Palette::accentSoft;
    }

    BLRgba32 Pill::tone_of(const Kind kind) {
        return Theme::palette().*tone_slot(kind);
    }

    BLRgba32 Pill::wash_of(const Kind kind) {
        return Theme::palette().*wash_slot(kind);
    }

    double Pill::natural_width(Typeface &type) {
        if (fixedWidth >= 0.0) {
            return fixedWidth;
        }

        double content = _glyph == Glyphs::Glyph::Empty
            ? type.width(type.at(600, Theme::fontSmall), _text)
            : 12.0 * 1.4;

        if (_dot) {
            content += 8.0 + 6.0;
        }

        return content + 22.0;
    }

    void Pill::paint(const Painter &painter) {
        const BLRgba32 ink = _ink;
        const double radius = _box.h / 2.0;

        painter.round(_box, radius, _wash.colour());
        painter.outline(_box, radius, 1.0, Theme::alpha(ink, 0.3));

        const BLFont &face = painter.font(600, Theme::fontSmall);
        const double label = _glyph == Glyphs::Glyph::Empty
            ? painter.width(face, _text)
            : 12.0 * 1.4;
        const double content = label + (_dot ? 8.0 + 6.0 : 0.0);

        double x = _box.x + ((_box.w - content) / 2.0);

        if (_dot) {
            painter.circle(BLPoint{x + 4.0, _box.y + (_box.h / 2.0)}, 4.0, ink);

            x += 8.0 + 6.0;
        }

        if (_glyph == Glyphs::Glyph::Empty) {
            painter.label(face, BLRect{x, _box.y, label + 2.0, _box.h}, Align::Start, _text, ink);
        } else {
            Glyphs::draw(painter.context(), _glyph,
                         BLPoint{x, _box.y + ((_box.h - (12.0 * 1.4)) / 2.0)}, 1.4F, ink);
        }
    }
}
