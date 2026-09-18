// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "ttk/draw/Glyphs.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/controls/Check.h"

namespace ttk {
    Check::Check(std::function<void(bool)> toggled) : _toggled(std::move(toggled)) {
        _takesPointer = true;
        cursor = Cursor::Pointer;
    }

    void Check::paint(const Painter &painter) {
        const Theme::Palette &palette = Theme::of();
        const double on = _on.value();

        const BLRect body{_box.x + ((_box.w - 18.0) / 2.0), _box.y + ((_box.h - 18.0) / 2.0), 18.0, 18.0};

        painter.round(body, 5.0, Theme::mix(palette.field, palette.accent, on));
        painter.outline(body, 5.0, 1.0,
                        Theme::mix(hovered() ? palette.borderStrong : palette.border, palette.accent,
                                   on));

        if (on <= 0.0) {
            return;
        }

        constexpr float weight = 0.85F;
        const double side = Glyphs::span(weight);

        Glyphs::draw(painter.context(), Glyphs::Glyph::Check,
                     BLPoint{body.x + ((body.w - side) / 2.0), body.y + ((body.h - side) / 2.0)}, weight,
                     Theme::alpha(palette.accentText, on));
    }

    bool Check::press(const Pointer & /*at*/) {
        return enabled();
    }

    void Check::release(const Pointer &at) {
        if (holds(at.x, at.y) && enabled() && _toggled) {
            _toggled(!_checked);
        }
    }

    void Check::set_checked(const bool value) {
        if (_checked == value) {
            return;
        }

        _checked = value;
        _on.toward(value ? 1.0F : 0.0F, now(), 0.14, Anim::Curve::CubicOut);
        wake();
    }

    void Check::enter() {
        Widget::enter();
    }

    void Check::leave() {
        Widget::leave();
    }

    bool Check::advance(const double now) {
        _on.advance(now);

        invalidate();

        return _on.live();
    }
}
