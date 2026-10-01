// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_LABEL_H
#define TTK_CONTROLS_LABEL_H


#include <functional>
#include <string>

#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
    //! Text drawn on one line, elided to fit, or wrapped over several lines when \ref wrap() is set.
    //!
    //! A label does not take the pointer unless \ref on_click() makes it a link. The chained setters are meant for
    //! construction and neither repaint nor ask for a new layout.
    class Label : public Widget {
    public:
        //! Creates a label showing `text` in regular body type and the text colour.
        explicit Label(std::string text = {}) : _text(std::move(text)) {}

        //! Returns the text as given, before any upper casing or path shortening.
        [[nodiscard]] const std::string &text() const { return _text; }

        //! Sets the text to `text` and repaints.
        //!
        //! A wrapped label asks for a new layout when its height at the current width changes. Any other label
        //! keeps its box, so a longer text is elided until the next layout.
        void set_text(std::string text);

        //! Sets the font weight, such as 400 or 600, and the size in pixels, and returns this label.
        Label *font(int weight, float size);

        //! Sets the text colour to `tone` and returns this label.
        Label *tone(Theme::Tone tone);

        //! Sets where a single line is placed across the box and returns this label. Wrapped text is always
        //! placed at the start.
        Label *align(Align where);

        //! Sets whether the text wraps to the width it is given, growing as tall as the lines need, and returns
        //! this label.
        Label *wrap(bool value = true);

        //! Styles the label as a section caption, small semibold faint text upper cased with wide letter spacing,
        //! and returns it. Only ASCII letters are upper cased.
        Label *section();

        //! Sets whether the label uses the fixed width face and returns this label.
        Label *mono(bool value = true);

        //! Makes the label a link that calls `clicked` when clicked, and returns this label.
        //!
        //! A link shows the pointer cursor, turns the accent colour on hover and answers the pointer only over its
        //! drawn text. Its tooltip is \ref Widget::hint.
        Label *on_click(std::function<void()> clicked);

        //! Sets whether the text is a file path and returns this label.
        //!
        //! A path is drawn in the fixed width face with the home directory written as `~`, and leading directories
        //! are replaced by an ellipsis until it fits the box.
        Label *path(bool value = true);

        //! Returns \ref fixedWidth when it is set, otherwise the width of the whole text on one line, upper cased
        //! for a section caption and unshortened for a path.
        double natural_width(Typeface &type) override;

        //! Returns \ref fixedHeight when it is set, otherwise one line height, or for a wrapped label the height
        //! of the text wrapped at `width`.
        double natural_height(Typeface &type, double width) override;

        //! Paints the text. Paints nothing when the text is empty.
        void paint(const Painter &painter) override;

        //! Measures the drawn text of a link, so \ref at() can hit test it.
        void arrange(Typeface &type) override;

        //! Takes the press when the label is a link.
        bool press(const Pointer &at) override;

        //! Calls the click callback of a link when `where` is still over its drawn text.
        void release(const Pointer &where) override;

        //! Called when the pointer moves onto the label. Marks it hovered and repaints it, as \ref Widget::enter()
        //! does.
        void enter() override;

        //! Called when the pointer moves off the label. Clears the hover and press state and repaints it, as
        //! \ref Widget::leave() does.
        void leave() override;

        //! Returns this label when it is a visible and enabled link and `[x, y]` lies over its drawn text, rather
        //! than anywhere in its box. Behaves as \ref Widget::at() for a label that is not a link.
        Widget *at(double x, double y) override;

    protected:
        //! Called when the \ref Theme has changed. Rereads the text colour when it follows a palette slot.
        void restyle() override { _tone.restyle(); }

    private:
        // What paint() puts on the screen, which for a path is the shortened form.
        double reach(Typeface &type) const;

        std::string _text;

        std::function<void()> _clicked;

        int _weight = 400;
        float _size = Theme::fontBody;

        Theme::Tone _tone{&Theme::Palette::text};

        Align _place = Align::Start;

        // What a tracked label draws, upper-cased once rather than per measure.
        std::string _upper;

        bool _wrap = false;
        bool _tracked = false;
        bool _mono = false;
        bool _path = false;

        // A path is fitted by dividing its box by the width of one character, so it is
        // always drawn in the fixed width face.
        [[nodiscard]] int face_weight() const { return Typeface::pick(_weight, _mono || _path); }

        double _reach = 0.0;

        // The wrapped height last measured, so a text that folds to a different one
        // asks for the layout it needs.
        double _tall = -1.0;
    };
}


#endif //TTK_CONTROLS_LABEL_H
