// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_GLYPHBUTTON_H
#define TTK_CONTROLS_GLYPHBUTTON_H


#include <functional>
#include <string>

#include "ttk/draw/Anim.h"
#include "ttk/draw/Theme.h"
#include "ttk/draw/Glyphs.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
    class GlyphButton : public Widget {
    public:
        GlyphButton(Glyphs::Glyph glyph, std::function<void()> clicked);

        GlyphButton *glyph(Glyphs::Glyph glyph);
        GlyphButton *size(double value);
        GlyphButton *tone(Theme::Tone rest, Theme::Tone lit);

        // A transparent wash falls back to the palette's hover.
        GlyphButton *wash(Theme::Tone tone);
        GlyphButton *outlined(bool value = true);
        GlyphButton *turn(double degrees);
        GlyphButton *spin(double degrees = 45.0);
        GlyphButton *tooltip(std::string text);

        double natural_width(Typeface &type) override;
        double natural_height(Typeface &type, double width) override;

        void paint(const Painter &painter) override;

        bool press(const Pointer &at) override;
        void release(const Pointer &at) override;
        void enter() override;
        void leave() override;

        // Turns the glyph by the `spin` amount and back, for a button that holds something
        // open.
        void spun(bool on);

        bool advance(double now) override;

    protected:
        void restyle() override;

    private:
        Glyphs::Glyph _glyph{};
        std::function<void()> _clicked;

        double _size = Theme::controlSmall;
        double _turn = 0.0;
        double _spinBy = 0.0;

        Theme::Tone _rest{&Theme::Palette::muted};
        Theme::Tone _hot{&Theme::Palette::text};
        Theme::Tone _wash{&Theme::Palette::hover};

        bool _outlined = false;

        Anim::Tween _lit;
        Anim::Tween _spun;
    };
}


#endif //TTK_CONTROLS_GLYPHBUTTON_H
