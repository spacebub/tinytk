// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <cmath>
#include <cstdint>

#include "ttk/draw/Glyphs.h"
#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/controls/Button.h"

namespace ttk {
    namespace {

        constexpr double HOVER_SECONDS = 0.11;

        // How long the busy dot rests on each of its three positions.
        constexpr double TICK = 0.3;

        // Interpolates premultiplied, so a transparent fill fades in without darkening. Theme::mix keeps the
        // alpha of the colour below, which would leave such a fill transparent.
        BLRgba32 blend(const BLRgba32 from, const BLRgba32 to, const double amount) {
            const double weight = std::clamp(amount, 0.0, 1.0);
            const double fromAlpha = from.a() / 255.0;
            const double toAlpha = to.a() / 255.0;
            const double alpha = fromAlpha + ((toAlpha - fromAlpha) * weight);

            if (alpha <= 0.0) {
                return BLRgba32{0};
            }

            const auto channel = [=](const uint32_t below, const uint32_t above) {
                const double start = below * fromAlpha;
                const double end = above * toAlpha;

                const double mixed = (start + ((end - start) * weight)) / alpha;

                return static_cast<uint32_t>(std::lround(std::min(255.0, mixed)));
            };

            return BLRgba32{channel(from.r(), to.r()), channel(from.g(), to.g()), channel(from.b(), to.b()),
                            static_cast<uint32_t>(std::lround(alpha * 255.0))};
        }

        Button::Look default_look(const Theme::Palette &palette) {
            return {.ground = palette.raised,
                    .lit = Theme::mix(palette.raised, palette.hover, 1.0),
                    .down = palette.sunken,
                    .edge = palette.borderStrong,
                    .edgeLit = palette.accent,
                    .ink = palette.text,
                    .dots = palette.accent};
        }

        Button::Look primary_look(const Theme::Palette &palette) {
            return {.ground = palette.accent,
                    .lit = palette.accentHover,
                    .down = Theme::darker(palette.accent, 0.15),
                    .edge = {},
                    .edgeLit = {},
                    .ink = palette.accentText,
                    .dots = palette.accentText};
        }

        Button::Look danger_look(const Theme::Palette &palette) {
            return {.ground = palette.raised,
                    .lit = palette.dangerSoft,
                    .down = Theme::darker(palette.dangerSoft, 0.08),
                    .edge = Theme::alpha(palette.danger, 0.5),
                    .edgeLit = Theme::alpha(palette.danger, 0.5),
                    .ink = palette.danger,
                    .dots = palette.accent};
        }

        Button::Look ghost_look(const Theme::Palette &palette) {
            return {.ground = {},
                    .lit = palette.accentSoft,
                    .down = palette.accentSoft,
                    .edge = {},
                    .edgeLit = {},
                    .ink = palette.accent,
                    .dots = palette.accent};
        }

    }

    constinit const Button::Kind Button::Kind::Default{&default_look};
    constinit const Button::Kind Button::Kind::Primary{&primary_look};
    constinit const Button::Kind Button::Kind::Danger{&danger_look};
    constinit const Button::Kind Button::Kind::Ghost{&ghost_look};

    Button::Button(std::string text, std::function<void()> clicked)
        : _text(std::move(text)), _clicked(std::move(clicked)) {
        _takesPointer = true;
        cursor = Cursor::Pointer;
        _give.set(1.0F);
        derive_look();
    }

    void Button::set_text(std::string text) {
        if (_text == text) {
            return;
        }

        _text = std::move(text);

        invalidate();
    }

    Button *Button::kind(const Kind value) {
        _kind = value;
        derive_look();

        return this;
    }

    void Button::restyle() {
        derive_look();
    }

    void Button::derive_look() {
        _look = (_kind.look != nullptr ? _kind.look : default_look)(Theme::palette());
    }

    Button *Button::glyph(const Glyphs::Glyph glyph) {
        _glyph = glyph;

        return this;
    }

    Button *Button::compact(const bool value) {
        _compact = value;

        return this;
    }

    Button *Button::busy(const bool value) {
        if (_busy == value) {
            return this;
        }

        _busy = value;

        if (_busy) {
            wake();
        }

        invalidate();

        return this;
    }

    Button *Button::tooltip(std::string text) {
        hint = std::move(text);

        return this;
    }

    double Button::natural_width(Typeface &type) {
        if (fixedWidth >= 0.0) {
            return fixedWidth;
        }

        const double mark = 12.0 * (_compact ? 1.1 : 1.2);
        const BLFont &face = type.at(600, _compact ? Theme::fontSmall : Theme::fontBody);
        const double content = (_glyph == Glyphs::Glyph::Empty ? 0.0 : mark + 7.0) + type.width(face, _text);

        return _compact ? content + 26.0 : std::max(content + 38.0, Theme::buttonWidth);
    }

    double Button::natural_height(Typeface & /*type*/, double /*width*/) {
        return fixedHeight >= 0.0 ? fixedHeight : (_compact ? Theme::controlSmall : Theme::control);
    }

    void Button::paint(const Painter &painter) {
        const double lit = _lit.value();
        const bool down = pressed();

        // The give is the box drawn a little smaller, which reads as a press.
        const double give = _give.value();
        const BLRect body{_box.x + (_box.w * (1.0 - give) / 2.0), _box.y + (_box.h * (1.0 - give) / 2.0),
                          _box.w * give, _box.h * give};

        const BLRgba32 edge = blend(_look.edge, _look.edgeLit, lit);

        painter.round(body, Theme::radiusSmall, down ? _look.down : blend(_look.ground, _look.lit, lit));

        if (edge.a() > 0) {
            painter.outline(body, Theme::radiusSmall, 1.0, edge);
        }

        if (_busy) {
            constexpr double dot = 6.0;
            constexpr double span = (dot * 3.0) + (5.0 * 2.0);

            for (int at = 0; at < 3; ++at) {
                painter.circle(BLPoint{_box.x + ((_box.w - span) / 2.0) + (at * (dot + 5.0)) + (dot / 2.0),
                                       _box.y + (_box.h / 2.0)},
                               dot / 2.0, _tick == at ? Theme::alpha(_look.dots, 0.25) : _look.dots);
            }

            return;
        }

        const double mark = 12.0 * (_compact ? 1.1 : 1.2);
        const BLFont &face = painter.font(600, _compact ? Theme::fontSmall : Theme::fontBody);
        const double label = painter.width(face, _text);
        const double content = (_glyph == Glyphs::Glyph::Empty ? 0.0 : mark + 7.0) + label;

        double x = _box.x + ((_box.w - content) / 2.0);

        const BLRgba32 tint = enabled() ? _look.ink : Theme::alpha(_look.ink, 0.45);

        if (_glyph != Glyphs::Glyph::Empty) {
            Glyphs::draw(painter.context(), _glyph,
                         BLPoint{x, _box.y + ((_box.h - mark) / 2.0)},
                         static_cast<float>(_compact ? 1.1 : 1.2), tint);

            x += mark + 7.0;
        }

        painter.label(face, BLRect{x, _box.y, label + 2.0, _box.h}, Align::Start, _text, tint);
    }

    bool Button::press(const Pointer & /*at*/) {
        if (!enabled() || _busy) {
            return false;
        }

        _give.run(0.985F, now(), 0.09, Anim::Curve::CubicOut);
        wake();

        return true;
    }

    void Button::release(const Pointer &at) {
        _give.run(1.0F, now(), 0.09, Anim::Curve::CubicOut);
        wake();

        if (holds(at.x, at.y) && enabled() && !_busy && _clicked) {
            _clicked();
        }
    }

    void Button::enter() {
        Widget::enter();

        _lit.toward(1.0F, now(), HOVER_SECONDS, Anim::Curve::CubicOut);
        wake();
    }

    void Button::leave() {
        Widget::leave();

        _lit.toward(0.0F, now(), HOVER_SECONDS, Anim::Curve::CubicOut);
        wake();
    }

    bool Button::key(const Key &pressed) {
        if (pressed.code != Code::Return && pressed.code != Code::Space) {
            return false;
        }

        if (enabled() && !_busy && _clicked) {
            _clicked();
        }

        return true;
    }

    bool Button::advance(const double now) {
        _lit.advance(now);
        _give.advance(now);

        if (_busy && now - _ticked > TICK) {
            _ticked = now;
            _tick = (_tick + 1) % 3;
        }

        invalidate();

        if (_lit.live() || _give.live()) {
            return true;
        }

        return _busy && sleep_until(_ticked + TICK);
    }
}
