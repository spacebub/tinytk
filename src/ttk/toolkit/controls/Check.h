// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_CHECK_H
#define TTK_CONTROLS_CHECK_H


#include <functional>

#include "ttk/draw/Anim.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
    class Check : public Widget {
    public:
        explicit Check(std::function<void(bool)> toggled);

        [[nodiscard]] bool checked() const { return _checked; }
        void set_checked(bool value);

        double natural_width(Typeface & /*type*/) override { return 18.0; }
        double natural_height(Typeface & /*type*/, double /*width*/) override { return 18.0; }

        void paint(const Painter &painter) override;

        bool press(const Pointer &at) override;
        void release(const Pointer &at) override;
        void enter() override;
        void leave() override;

        bool advance(double now) override;

    private:
        std::function<void(bool)> _toggled;
        bool _checked = false;

        Anim::Tween _on;
    };
}


#endif //TTK_CONTROLS_CHECK_H
