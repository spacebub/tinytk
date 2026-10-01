// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_OVERLAYS_TIPS_H
#define TTK_OVERLAYS_TIPS_H


#include <string>

#include "ttk/draw/Anim.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
    //! Tooltip overlay that shows one tip at a time for the whole window.
    //!
    //! Meant for the \ref Root::TIPS layer, fed each frame through \ref point() with the \ref Widget::hint of the
    //! hovered widget. The tip wraps at 320 pixels and fades in and out.
    class Tips : public Widget {
    public:
        //! Reports the tip wanted now: `text` for the widget whose box is `over`, with the pointer at `[x, y]`, at the
        //! frame time `now`. An empty `text` fades the tip out.
        //!
        //! The first tip appears once the same `text` has been reported for 0.45 seconds. While a tip is up, a
        //! different `text` replaces it at once. The tip is centred across on `x` and placed above `over`, or below
        //! it when there is no room, kept 4 pixels inside the widget's own box. Until it appears it follows the
        //! pointer, and once up it stays where it appeared. `y` does not affect placement.
        void point(const std::string &text, const BLRect &over, double x, double y, double now);

        //! Paints the tip at its current fade.
        void paint(const Painter &painter) override;

        //! Raises the waiting tip once its delay has passed and steps the fade. Returns true while fading, and sleeps
        //! until the delay ends while a tip is waiting.
        bool advance(double now) override;

        //! Returns null, so the pointer always passes through the tip.
        Widget *at(double /*x*/, double /*y*/) override { return nullptr; }

    private:
        [[nodiscard]] BLRect measure(const std::string &said) const;

        void raise();

        static constexpr double DELAY = 0.45;
        static constexpr double ROOM = 320.0;

        std::string _pending;
        std::string _shown;

        BLRect _over{};
        double _x = 0.0;
        double _y = 0.0;

        double _armed = 0.0;
        bool _up = false;

        BLRect _frame{};

        Anim::Tween _fade;
    };
}


#endif //TTK_OVERLAYS_TIPS_H
