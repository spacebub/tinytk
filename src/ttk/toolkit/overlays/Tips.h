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
    // One tooltip for the window, armed by whatever the pointer rests on.
    class Tips : public Widget {
    public:
        // Called every frame with what the pointer is over and where the pointer is.
        // An empty text takes the tip down.
        void point(const std::string &text, const BLRect &over, double x, double y, double now);

        void paint(const Painter &painter) override;

        bool advance(double now) override;

        // Nothing under a tooltip is ever pressed through it.
        Widget *at(double /*x*/, double /*y*/) override { return nullptr; }

    private:
        // Where the words would go, given what they are and where the pointer is.
        [[nodiscard]] BLRect measure(const std::string &said) const;

        // Puts `_pending` up, damaging what it leaves and what it takes.
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
