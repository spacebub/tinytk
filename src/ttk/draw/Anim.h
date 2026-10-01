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

//! Easing curves and a single value animated over time.
namespace ttk::Anim {

    //! Easing curve that maps linear progress to eased progress.
    enum class Curve : std::uint8_t {
        //! Progress is passed through unchanged.
        Linear,
        //! Fast start and gentle settle, matching CSS `cubic-bezier(0.33, 1, 0.68, 1)`.
        CubicOut,
        //! Ease-out that overshoots the target by about 10% and settles back onto it.
        BackOut,
    };

    //! Returns `at` eased by `curve`.
    //!
    //! `at` is clamped to `[0, 1]` first, so the result is exactly 0 at or below 0 and exactly 1 at or above 1.
    //! \ref Curve::BackOut can return values above 1 in between.
    float shape(Curve curve, float at);

    //! A float that moves from one value to another over a fixed duration.
    //!
    //! A tween holds no clock of its own. Every call that needs time takes `now` in seconds, normally the frame
    //! clock from \ref Widget::now(), and \ref advance() must be called each frame for \ref value() to move.
    class Tween {
    public:
        //! Sets the value to `value` immediately and stops any run in progress.
        void set(float value);

        //! Starts a run from the current value to `value`, beginning at `now` and lasting `seconds`.
        //!
        //! A run in progress is replaced and the new one starts from wherever the old one had got to.
        //! A non-positive `seconds` behaves like \ref set().
        void run(float value, double now, double seconds, Curve curve);

        //! Starts a run to `value` unless the tween is already at or headed to `value`.
        //!
        //! Safe to call every frame with the same target, which \ref run() is not, as it would restart the run each
        //! time. Use it for state that is reported repeatedly, such as hover.
        void toward(float value, double now, double seconds, Curve curve);

        //! Moves the value to where the run in progress should be at `now`, and ends the run once its duration
        //! has elapsed. Does nothing when no run is in progress.
        void advance(double now);

        //! Returns the current value, as of the last \ref advance(), \ref set() or \ref run().
        [[nodiscard]] float value() const { return _value; }

        //! Tests whether a run is in progress.
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


#endif //TTK_DRAW_ANIM_H
