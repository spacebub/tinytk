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
    //! Check box drawn as an 18 pixel square centred in its box.
    //!
    //! A click does not change the state by itself. It reports the requested state to the callback, and the owner
    //! applies it with \ref set_checked().
    class Check : public Widget {
    public:
        //! Creates an unchecked box that calls `toggled` with the opposite of \ref checked() on each click.
        //! `toggled` may be empty.
        explicit Check(std::function<void(bool)> toggled);

        //! Tests whether the box is checked.
        [[nodiscard]] bool checked() const { return _checked; }

        //! Sets the checked state to `value` and animates the box towards it. Does not call the toggle callback.
        void set_checked(bool value);

        //! Returns 18, the side of the box in pixels. \ref fixedWidth is ignored.
        double natural_width(Typeface & /*type*/) override { return 18.0; }

        //! Returns 18, the side of the box in pixels. \ref fixedHeight is ignored.
        double natural_height(Typeface & /*type*/, double /*width*/) override { return 18.0; }

        //! Paints the box, filled with the accent colour and a check mark as it turns on.
        void paint(const Painter &painter) override;

        //! Takes the press when the box is enabled.
        bool press(const Pointer &at) override;

        //! Calls the toggle callback with the opposite of \ref checked() when `at` is inside the box and it is
        //! enabled.
        void release(const Pointer &at) override;

        //! Called when the pointer moves onto the box. Marks it hovered and repaints it, as \ref Widget::enter()
        //! does.
        void enter() override;

        //! Called when the pointer moves off the box. Clears the hover and press state and repaints it, as
        //! \ref Widget::leave() does.
        void leave() override;

        //! Steps the check animation, repaints and returns whether it is still running.
        bool advance(double now) override;

    private:
        std::function<void(bool)> _toggled;
        bool _checked = false;

        Anim::Tween _on;
    };
}


#endif //TTK_CONTROLS_CHECK_H
