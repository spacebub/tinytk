// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "ttk/draw/Anim.h"

namespace ttk {
    namespace {

        // CSS's cubic-bezier(0.33, 1, 0.68, 1), to within the width of a pixel:
        // 1 - (1 - t)^3.
        float cubic_out(const float at) {
            const float left = 1.0F - at;

            return 1.0F - (left * left * left);
        }

        // The usual ease-out-back constants. The overshoot is about 10%.
        float back_out(const float at) {
            constexpr float over = 1.70158F;
            const float left = at - 1.0F;

            return 1.0F + ((over + 1.0F) * left * left * left) + (over * left * left);
        }

    }

    float Anim::shape(const Curve curve, const float at) {
        if (at <= 0.0F) {
            return 0.0F;
        }

        if (at >= 1.0F) {
            return 1.0F;
        }

        switch (curve) {
            case Curve::CubicOut: return cubic_out(at);
            case Curve::BackOut:  return back_out(at);
            default:              return at;
        }
    }

    void Anim::Tween::set(const float value) {
        _value = value;
        _from = value;
        _to = value;
        _running = false;
    }

    void Anim::Tween::run(const float value, const double now, const double seconds,
                          const Curve curve) {
        if (seconds <= 0.0) {
            set(value);

            return;
        }

        _from = _value;
        _to = value;
        _start = now;
        _span = seconds;
        _curve = curve;
        _running = true;
    }

    void Anim::Tween::toward(const float value, const double now, const double seconds,
                             const Curve curve) {
        if (_running ? _to == value : _value == value) {
            return;
        }

        run(value, now, seconds, curve);
    }

    void Anim::Tween::advance(const double now) {
        if (!_running) {
            return;
        }

        const double through = (now - _start) / _span;

        if (through >= 1.0) {
            _value = _to;
            _running = false;

            return;
        }

        _value = _from + ((_to - _from) * shape(_curve, static_cast<float>(through)));
    }
}
