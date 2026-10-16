// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "ttk/toolkit/controls/Spinner.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "ttk/draw/Glyphs.h"

namespace ttk {
namespace {

constexpr double SWEEP = 0.28;
constexpr double RING = 0.2;
constexpr double DISABLED = 0.4;

// Nudged so the step a wake-up lands on is never the one it slept from, which a time that is a whole number
// of steps short by a rounding error would be.
double step_of(const double now) {
    return std::floor((now * Spinner::STEPS) + 1e-6);
}

double phase_of(const double step) {
    const double turns = std::max(0.0, step) / Spinner::STEPS / Spinner::PERIOD;

    return turns - std::floor(turns);
}

BLPath &shape() {
    static BLPath kept;

    return kept;
}

}

double Spinner::phase_at(const double now) {
    return phase_of(step_of(now));
}

double Spinner::step_after(const double now) {
    return (step_of(now) + 1.0) / STEPS;
}

void Spinner::render(const Painter &painter, const BLRect &box, const double phase, const BLRgba32 tone) {
    const double side = std::min(box.w, box.h);

    if (side <= 0.0 || tone.a() == 0) {
        return;
    }

    const BLRect square{box.x + ((box.w - side) / 2.0), box.y + ((box.h - side) / 2.0), side, side};

    if (!painter.needed(square)) {
        return;
    }

    const double width = std::max(1.5, side * 0.14);
    const double radius = (side - width) / 2.0;
    const double x = square.x + (side / 2.0);
    const double y = square.y + (side / 2.0);
    BLPath &path = shape();

    path.clear();
    path.add_circle(BLCircle(x, y, radius));
    painter.stroke(path, width, Theme::alpha(tone, RING));

    const double turn = 2.0 * std::numbers::pi;

    path.clear();
    path.add_arc(BLArc(x, y, radius, radius, (phase * turn) - (turn / 4.0), SWEEP * turn));
    painter.stroke(path, width, tone);
}

Spinner::Spinner(const float weight) : _side(Glyphs::span(weight)) {
}

Spinner *Spinner::tone(Theme::Tone tone) {
    _tone = tone;

    invalidate();

    return this;
}

void Spinner::restyle() {
    _tone.restyle();
}

double Spinner::natural_width(Typeface & /*type*/) {
    return fixedWidth >= 0.0 ? fixedWidth : _side;
}

double Spinner::natural_height(Typeface & /*type*/, double /*width*/) {
    return fixedHeight >= 0.0 ? fixedHeight : _side;
}

void Spinner::paint(const Painter &painter) {
    const bool live = enabled();

    if (live && !_turning && root() != nullptr) {
        _turning = true;
        _step = step_of(now());

        wake();
    }

    _shown = true;

    render(painter, _box, phase_of(_step), live ? _tone.colour() : Theme::alpha(_tone.colour(), DISABLED));
}

bool Spinner::advance(const double now) {
    if (!enabled() || !_shown) {
        _turning = false;

        return false;
    }

    if (const double step = step_of(now); step != _step) {
        _step = step;
        _shown = false;

        invalidate();
    }

    return sleep_until((_step + 1.0) / STEPS);
}
}
