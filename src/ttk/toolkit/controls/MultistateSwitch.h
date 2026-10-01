// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_MULTISTATESWITCH_H
#define TTK_CONTROLS_MULTISTATESWITCH_H


#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "ttk/draw/Anim.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
    //! Segmented control that picks one of a short list of choices, drawn as a row of words in a rounded trough.
    //!
    //! The current choice is highlighted by a marker that slides between words. A click does not change the
    //! current choice by itself. It reports the clicked value to the callback, and the owner applies it with
    //! \ref set_current().
    class MultistateSwitch : public Widget {
    public:
        //! One word of the switch.
        struct Choice {
            //! Value reported when the choice is clicked, typically the caller's own enum cast to `int`.
            int value = 0;
            //! Word drawn for the choice.
            std::string label;
            //! Whether a small accent dot is drawn at the top right of the word.
            bool badge = false;

            //! Tests whether every field of `other` equals the one of this choice.
            bool operator==(const Choice &other) const = default;
        };

        //! Creates an empty switch that calls `selected` with the \ref Choice::value of each clicked choice.
        //! `selected` may be empty.
        explicit MultistateSwitch(std::function<void(int)> selected);

        //! Replaces the choices with `options`, repaints and asks for a new layout. Does nothing when `options`
        //! equals the current choices.
        void set_options(std::vector<Choice> options);

        //! Makes the choice whose value is `value` current and slides the marker to it. Does not call the
        //! selection callback.
        void set_current(int value);

        //! Returns the value last given to \ref set_current(), or nothing before the first call.
        [[nodiscard]] std::optional<int> current() const { return _current; }

        //! Returns \ref fixedWidth when it is set, otherwise the width of all words with their padding.
        double natural_width(Typeface &type) override;

        //! Returns \ref Theme::control. \ref fixedHeight is ignored.
        double natural_height(Typeface & /*type*/, double /*width*/) override { return Theme::control; }

        //! Moves the marker onto the current choice without animating, unless a slide is in progress. Removes
        //! the marker when no choice has the current value.
        void arrange(Typeface &type) override;

        //! Paints the trough, the marker and the words from the left edge of the box. The current word is drawn
        //! semibold in the accent colour, and the word under the pointer in the text colour.
        void paint(const Painter &painter) override;

        //! Takes the press when the switch is enabled.
        bool press(const Pointer &at) override;

        //! Calls the selection callback with the value of the choice under `at`, including the current one. Does
        //! nothing when `at` is outside the switch or between words.
        void release(const Pointer &at) override;

        //! Tracks the word under the pointer and repaints when it changes.
        void hover(const Pointer &at) override;

        //! Called when the pointer moves off the switch. Clears the hover state and the word under the pointer.
        void leave() override;

        //! Slides the marker towards the current choice over 0.18 seconds, repaints and returns whether the slide
        //! is still running.
        bool advance(double now) override;

    private:
        std::vector<BLRect> lanes(Typeface &type) const;

        std::vector<Choice> _options;
        std::optional<int> _current;

        std::function<void(int)> _selected;

        int _over = -1;

        Anim::Tween _markX;
        Anim::Tween _markWidth;
        bool _marked = false;
    };
}


#endif //TTK_CONTROLS_MULTISTATESWITCH_H
