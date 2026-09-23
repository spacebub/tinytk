// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "ttk/draw/Glyphs.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/controls/GlyphButton.h"

namespace ttk {
    GlyphButton::GlyphButton(const Glyphs::Glyph glyph, std::function<void()> clicked)
        : _glyph(glyph), _clicked(std::move(clicked)) {
        _takesPointer = true;
        cursor = Cursor::Pointer;
    }

    GlyphButton *GlyphButton::glyph(const Glyphs::Glyph glyph) {
        if (_glyph != glyph) {
            _glyph = glyph;

            invalidate();
        }

        return this;
    }

    GlyphButton *GlyphButton::size(const double value) {
        _size = value;

        return this;
    }

    GlyphButton *GlyphButton::tone(const Theme::Tone rest, const Theme::Tone lit) {
        _rest = rest;
        _hot = lit;

        return this;
    }

    GlyphButton *GlyphButton::wash(const Theme::Tone tone) {
        _wash = tone.colour().a() > 0 ? tone : Theme::Tone(&Theme::Palette::hover);

        return this;
    }

    void GlyphButton::restyle() {
        _rest.restyle();
        _hot.restyle();
        _wash.restyle();
    }

    GlyphButton *GlyphButton::outlined(const bool value) {
        _outlined = value;

        return this;
    }

    GlyphButton *GlyphButton::turn(const double degrees) {
        _turn = degrees;

        return this;
    }

    GlyphButton *GlyphButton::spin(const double degrees) {
        _spinBy = degrees;

        return this;
    }

    GlyphButton *GlyphButton::tooltip(std::string text) {
        hint = std::move(text);

        return this;
    }

    double GlyphButton::natural_width(Typeface & /*type*/) {
        return fixedWidth >= 0.0 ? fixedWidth : _size;
    }

    double GlyphButton::natural_height(Typeface & /*type*/, double /*width*/) {
        return fixedHeight >= 0.0 ? fixedHeight : _size;
    }

    void GlyphButton::paint(const Painter &painter) {
        const Theme::Palette &palette = Theme::palette();
        const double lit = _lit.value();

        const BLRect body{_box.x + ((_box.w - _size) / 2.0), _box.y + ((_box.h - _size) / 2.0), _size,
                          _size};

        if (_outlined) {
            painter.round(body, Theme::radiusSmall, palette.raised);
        }

        if (lit > 0.0) {
            painter.round(body, Theme::radiusSmall,
                          Theme::alpha(_wash.colour(), lit));
        }

        if (_outlined) {
            painter.outline(body, Theme::radiusSmall, 1.0,
                            Theme::mix(palette.borderStrong, _hot.colour(), lit));
        }

        constexpr float weight = 1.2F;
        const double side = Glyphs::span(weight);
        const BLRgba32 ink = Theme::mix(_rest.colour(), _hot.colour(), lit);

        Glyphs::draw(painter.context(), _glyph,
                     BLPoint{body.x + ((body.w - side) / 2.0), body.y + ((body.h - side) / 2.0)}, weight,
                     enabled() ? ink : Theme::alpha(ink, 0.4),
                     static_cast<float>(_turn) + _spun.value());
    }

    bool GlyphButton::press(const Pointer & /*at*/) {
        return enabled();
    }

    void GlyphButton::release(const Pointer &at) {
        if (holds(at.x, at.y) && enabled() && _clicked) {
            _clicked();
        }
    }

    void GlyphButton::enter() {
        Widget::enter();

        _lit.toward(1.0F, now(), 0.1, Anim::Curve::CubicOut);
        wake();
    }

    void GlyphButton::leave() {
        Widget::leave();

        _lit.toward(0.0F, now(), 0.1, Anim::Curve::CubicOut);
        wake();
    }

    void GlyphButton::spun(const bool on) {
        _spun.run(on ? static_cast<float>(_spinBy) : 0.0F, now(), 0.16, Anim::Curve::CubicOut);

        wake();
        invalidate();
    }

    bool GlyphButton::advance(const double now) {
        _lit.advance(now);
        _spun.advance(now);

        invalidate();

        return _lit.live() || _spun.live();
    }
}
