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
    // Plain type, laid out as one line unless `wrap` is set.
    class Label : public Widget {
    public:
        explicit Label(std::string text = {}) : _text(std::move(text)) {}

        [[nodiscard]] const std::string &text() const { return _text; }

        void set_text(std::string text);

        Label *font(int weight, float size);
        Label *tone(Theme::Tone tone);
        Label *align(Align where);

        // Wrapped to the width it is given, and as tall as that takes.
        Label *wrap(bool value = true);

        // Small, tracked and upper case: the caption over a group of controls.
        Label *section();

        // Fixed width face, for paths and command lines.
        Label *mono(bool value = true);

        // Takes the pointer and tints on hover. The tooltip is `hint` as usual.
        Label *on_click(std::function<void()> clicked);

        // Written as ~, and shortened by whole directories to whatever room it gets.
        Label *path(bool value = true);

        double natural_width(Typeface &type) override;
        double natural_height(Typeface &type, double width) override;

        void paint(const Painter &painter) override;

        void arrange(Typeface &type) override;

        bool press(const Pointer &at) override;
        void release(const Pointer &where) override;
        void enter() override;
        void leave() override;

        // A link answers where its text is, not across the row it was given.
        Widget *at(double x, double y) override;

    protected:
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
