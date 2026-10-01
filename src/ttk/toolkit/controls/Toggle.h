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
    //! On and off switch drawn as a track with a sliding knob, followed by an optional label.
    //!
    //! The switch does not change \ref checked by itself. A click or key reports the new state through the
    //! `toggled` callback, and the owner calls \ref set_checked() to accept it.
    class Toggle : public Widget {
    public:
        //! Creates an unchecked switch with the label `text`, none when empty, that reports clicks to `toggled`.
        //!
        //! `toggled` is called with the opposite of \ref checked when the switch is clicked or activated from the
        //! keyboard. It may be null.
        Toggle(std::string text, std::function<void(bool)> toggled);

        //! Whether the switch is on.
        //!
        //! Read it freely, but change it through \ref set_checked(). Writing it directly does not move the knob or
        //! repaint, and a later \ref set_checked() with the same value then does nothing.
        bool checked = false;

        //! Sets \ref checked to `value` without calling the `toggled` callback, and slides the knob to match.
        //!
        //! The knob jumps without animating while the switch is not attached to a root. Does nothing when
        //! \ref checked already equals `value`.
        void set_checked(bool value);

        //! Sets the label to `text` and repaints. The area that takes the pointer follows at the next layout.
        void set_text(std::string text);

        //! Returns \ref Widget::fixedWidth when it is set, otherwise the width of the track and the label.
        double natural_width(Typeface &type) override;

        //! Returns \ref Widget::fixedHeight when it is set, otherwise 24 pixels.
        double natural_height(Typeface &type, double width) override;

        //! Measures the track and the label, which bound the area \ref at() answers to.
        void arrange(Typeface &type) override;

        //! Paints the track, the knob and the label, dimmed when disabled.
        void paint(const Painter &painter) override;

        //! Takes the press when the switch is enabled.
        bool press(const Pointer &at) override;

        //! Calls the `toggled` callback with the opposite of \ref checked when `where` is still on the track or
        //! the label.
        void release(const Pointer &where) override;

        //! Called when the pointer moves onto the switch. Repaints it with a stronger track border.
        void enter() override;

        //! Called when the pointer moves off the switch. Repaints it with the normal track border.
        void leave() override;

        //! Tests whether the switch can receive keyboard focus, which it can while enabled.
        [[nodiscard]] bool takes_focus() const override { return enabled(); }

        //! Handles Return and Space, which call the `toggled` callback with the opposite of \ref checked when the
        //! switch is enabled. Returns false for other keys.
        bool key(const Key &pressed) override;

        //! Steps the knob animation and returns whether it is still running.
        bool advance(double now) override;

        //! Returns this switch when `[x, y]` is on the track or the label and the switch is visible and enabled,
        //! otherwise null.
        //!
        //! The rest of the box is ignored, so a switch given a whole row does not answer a press at the far end.
        Widget *at(double x, double y) override;

    private:
        std::string _text;
        std::function<void(bool)> _toggled;

        double reach(Typeface &type) const;

        double _reach = 0.0;

        Anim::Tween _on;
    };
}


#endif //TTK_CONTROLS_TOGGLE_H
