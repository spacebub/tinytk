// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_DRAW_ANIM_H
#define TTK_DRAW_ANIM_H


#include <cstdint>

namespace ttk {
    namespace Anim {

        enum class Curve : std::uint8_t {
            Linear,
            // cubic-bezier(0.33, 1, 0.68, 1).
            CubicOut,
            // ease-out-back: overshoots and comes back.
            BackOut,
        };

        float shape(Curve curve, float at);

        class Tween {
        public:
            // Jumps straight there, with nothing in flight.
            void set(float value);

            // Starts a run to `value`, from wherever the tween has got to.
            void run(float value, double now, double seconds, Curve curve);

            // Same, except a run already headed there is left alone. Otherwise a hover
            // that arrives every frame would restart the animation every frame.
            void toward(float value, double now, double seconds, Curve curve);

            void advance(double now);

            [[nodiscard]] float value() const { return _value; }

            [[nodiscard]] bool live() const { return _running; }

        private:
            float _value = 0.0F;
            float _from = 0.0F;
            float _to = 0.0F;

            double _start = 0.0;
            double _span = 0.0;

            Curve _curve = Curve::Linear;
            bool _running = false;
        };

    }
}


#endif //TTK_DRAW_ANIM_H
