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
#include <string>
#include <string_view>

#include "ttk/draw/Anim.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
//! Check box drawn as an 18 pixel square, centred in its box, or at its left edge with a text beside it.
//!
//! A click does not change the state by itself. It reports the requested state to the callback, and the owner
//! applies it with \ref set_checked(). With a text, a click on the text counts as a click on the square.
class Check : public Widget {
public:
    //! Creates an unchecked box that calls `toggled` with the opposite of \ref checked() on each click.
    //! `toggled` may be empty.
    explicit Check(std::function<void(bool)> toggled);

    //! Creates an unchecked box with `text` beside it that calls `toggled` with the opposite of \ref checked()
    //! on each click. `toggled` may be empty. An empty `text` draws the bare square.
    Check(std::string text, std::function<void(bool)> toggled);

    //! Tests whether the box is checked.
    [[nodiscard]] bool checked() const { return _checked; }

    //! Sets the checked state to `value` and animates the box towards it, or jumps to it while the box is in no
    //! root. Does not call the toggle callback.
    void set_checked(bool value);

    //! Replaces the text beside the square and repaints. The box keeps its size until the next layout.
    void set_text(std::string_view text);

    //! Returns the text beside the square.
    [[nodiscard]] const std::string &text() const { return _text; }

    //! Returns 18, the side of the square, when there is no text, ignoring \ref fixedWidth. With a text, returns
    //! \ref fixedWidth when it is set, otherwise the square, a 10 pixel gap and the text.
    double natural_width(Typeface &type) override;

    //! Returns 18, the side of the square, when there is no text, ignoring \ref fixedHeight. With a text,
    //! returns \ref fixedHeight when it is set, otherwise 24.
    double natural_height(Typeface &type, double width) override;

    //! Measures the text, so a click past its end misses.
    void arrange(Typeface &type) override;

    //! Paints the square, filled with the accent colour and a check mark as it turns on, and the text elided
    //! to the box.
    void paint(const Painter &painter) override;

    //! Returns the box at `[x, y]` when it is visible and enabled. With a text, only the square, the gap and
    //! the text count, so the space past the text is not part of the box.
    Widget *at(double x, double y) override;

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
    [[nodiscard]] double reach(Typeface &type) const;

    std::string _text;
    std::function<void(bool)> _toggled;
    bool _checked = false;

    // How far the square, the gap and the text reach from the left edge, measured at layout.
    double _reach = 0.0;

    Anim::Tween _on;
};
}


#endif //TTK_CONTROLS_CHECK_H
