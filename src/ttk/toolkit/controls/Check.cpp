// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>

#include "ttk/toolkit/controls/Check.h"
#include "ttk/draw/Glyphs.h"
#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"

namespace ttk {
namespace {
constexpr double SIDE = 18.0;
constexpr double GAP = 10.0;
}

Check::Check(std::function<void(bool)> toggled) : Check(std::string(), std::move(toggled)) {}

Check::Check(std::string text, std::function<void(bool)> toggled)
    : _text(std::move(text)), _toggled(std::move(toggled)) {
    _takesPointer = true;
    cursor = Cursor::Pointer;
}

void Check::set_text(const std::string_view text) {
    if (_text == text) {
        return;
    }

    _text = text;

    invalidate();
}

double Check::reach(Typeface &type) const {
    return _text.empty() ? SIDE : SIDE + GAP + type.width(type.at(400, Theme::fontBody), _text);
}

double Check::natural_width(Typeface &type) {
    _reach = reach(type);

    return _text.empty() || fixedWidth < 0.0 ? _reach : fixedWidth;
}

double Check::natural_height(Typeface & /*type*/, double /*width*/) {
    if (_text.empty()) {
        return SIDE;
    }

    return fixedHeight >= 0.0 ? fixedHeight : 24.0;
}

void Check::arrange(Typeface &type) {
    _reach = reach(type);
}

Widget *Check::at(const double x, const double y) {
    if (!visible() || !enabled() || !holds(x, y)) {
        return nullptr;
    }

    return _text.empty() || x < _box.x + std::min(_reach, _box.w) ? this : nullptr;
}

void Check::paint(const Painter &painter) {
    const Theme::Palette &palette = Theme::palette();
    const double on = _on.value();
    const double left = _text.empty() ? _box.x + ((_box.w - SIDE) / 2.0) : _box.x;

    const BLRect body{left, _box.y + ((_box.h - SIDE) / 2.0), SIDE, SIDE};

    painter.round(body, 5.0, Theme::mix(palette.field, palette.accent, on));
    painter.outline(body, 5.0, 1.0, Theme::mix(hovered() ? palette.borderStrong : palette.border, palette.accent, on));

    if (!_text.empty()) {
        painter.label(painter.font(400, Theme::fontBody),
                      BLRect{body.x + SIDE + GAP, _box.y, std::max(0.0, _box.w - SIDE - GAP), _box.h},
                      Align::Start,
                      _text,
                      enabled() ? palette.text : Theme::alpha(palette.text, 0.45));
    }

    if (on <= 0.0) {
        return;
    }

    constexpr float weight = 0.85F;
    const double side = Glyphs::span(weight);

    Glyphs::draw(painter.context(),
                 Glyphs::Glyph::Check,
                 BLPoint{body.x + ((body.w - side) / 2.0), body.y + ((body.h - side) / 2.0)},
                 weight,
                 Theme::alpha(palette.accentText, on));
}

bool Check::press(const Pointer & /*at*/) {
    return enabled();
}

// The whole box, past the text too, since Return and Space press its centre.
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

    // Nothing advances a tween started before the box is in a root, so a state set while it is built has to jump.
    if (root() == nullptr) {
        _on.set(value ? 1.0F : 0.0F);
        invalidate();

        return;
    }

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
