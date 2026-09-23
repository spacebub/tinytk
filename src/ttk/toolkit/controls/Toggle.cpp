// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>

#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/controls/Toggle.h"

namespace ttk {
    Toggle::Toggle(std::string text, std::function<void(bool)> toggled)
        : _text(std::move(text)), _toggled(std::move(toggled)) {
        _takesPointer = true;
        cursor = Cursor::Pointer;
    }

    void Toggle::set_checked(const bool value) {
        if (checked == value) {
            return;
        }

        checked = value;

        // Nothing advances a tween started before the widget is in a root, so a state
        // set while the dialog is still being built has to jump.
        if (root() == nullptr) {
            _on.set(value ? 1.0F : 0.0F);
            invalidate();

            return;
        }

        _on.run(value ? 1.0F : 0.0F, now(), 0.14, Anim::Curve::CubicOut);
        wake();
    }

    void Toggle::set_text(std::string text) {
        if (_text == text) {
            return;
        }

        _text = std::move(text);

        invalidate();
    }

    double Toggle::reach(Typeface &type) const {
        return _text.empty() ? 38.0
                             : 38.0 + 10.0 + type.width(type.at(400, Theme::fontBody), _text);
    }

    double Toggle::natural_width(Typeface &type) {
        _reach = reach(type);

        return fixedWidth >= 0.0 ? fixedWidth : _reach;
    }

    void Toggle::arrange(Typeface &type) {
        _reach = reach(type);
    }

    Widget *Toggle::at(const double x, const double y) {
        const double reach = _reach > 0.0 ? _reach : _box.w;

        return visible() && enabled() && x >= _box.x && x < _box.x + std::min(reach, _box.w)
                && y >= _box.y && y < _box.y + _box.h
            ? this
            : nullptr;
    }

    double Toggle::natural_height(Typeface & /*type*/, double /*width*/) {
        return fixedHeight >= 0.0 ? fixedHeight : 24.0;
    }

    void Toggle::paint(const Painter &painter) {
        const Theme::Palette &palette = Theme::palette();
        const double on = _on.value();
        const double dim = enabled() ? 1.0 : 0.45;

        const BLRect track{_box.x, _box.y + ((_box.h - 22.0) / 2.0), 38.0, 22.0};

        painter.round(track, 11.0,
                      Theme::alpha(Theme::mix(palette.sunken, palette.accent, on), dim));
        painter.outline(track, 11.0, 1.0,
                        Theme::alpha(Theme::mix(hovered() ? palette.borderStrong : palette.border,
                                                palette.accent, on),
                                     dim));

        constexpr double knob = 16.0;
        const double left = track.x + 3.0 + (on * (track.w - knob - 6.0));

        painter.circle(BLPoint{left + (knob / 2.0), track.y + (track.h / 2.0)}, knob / 2.0,
                       Theme::alpha(Theme::mix(palette.faint, palette.accentText, on), dim));

        if (_text.empty()) {
            return;
        }

        painter.label(painter.font(400, Theme::fontBody),
                      BLRect{track.x + track.w + 10.0, _box.y,
                             _box.w - track.w - 10.0, _box.h},
                      Align::Start, _text, Theme::alpha(palette.text, dim));
    }

    bool Toggle::press(const Pointer & /*at*/) {
        return enabled();
    }

    void Toggle::release(const Pointer &where) {
        if (at(where.x, where.y) == this && _toggled) {
            _toggled(!checked);
        }
    }

    void Toggle::enter() {
        Widget::enter();
    }

    void Toggle::leave() {
        Widget::leave();
    }

    bool Toggle::key(const Key &pressed) {
        if (pressed.code != Code::Return && pressed.code != Code::Space) {
            return false;
        }

        if (enabled() && _toggled) {
            _toggled(!checked);
        }

        return true;
    }

    bool Toggle::advance(const double now) {
        _on.advance(now);

        invalidate();

        return _on.live();
    }
}
