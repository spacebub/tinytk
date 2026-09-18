// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_TOGGLE_H
#define TTK_CONTROLS_TOGGLE_H


#include <functional>
#include <string>

#include "ttk/draw/Anim.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
    // A switch: the track, the knob, and an optional label beside them.
    class Toggle : public Widget {
    public:
        Toggle(std::string text, std::function<void(bool)> toggled);

        bool checked = false;

        void set_checked(bool value);

        void set_text(std::string text);

        double natural_width(Typeface &type) override;
        double natural_height(Typeface &type, double width) override;

        void arrange(Typeface &type) override;

        void paint(const Painter &painter) override;

        bool press(const Pointer &at) override;
        void release(const Pointer &where) override;
        void enter() override;
        void leave() override;

        [[nodiscard]] bool takes_focus() const override { return enabled(); }
        bool key(const Key &pressed) override;

        bool advance(double now) override;

        // The track and its label, not whatever width the row handed over: a switch
        // alone on a row must not answer a press at the far end of it.
        Widget *at(double x, double y) override;

    private:
        std::string _text;
        std::function<void(bool)> _toggled;

        // The track and the label together, which is all the hit test answers to.
        double reach(Typeface &type) const;

        double _reach = 0.0;

        Anim::Tween _on;
    };
}


#endif //TTK_CONTROLS_TOGGLE_H
