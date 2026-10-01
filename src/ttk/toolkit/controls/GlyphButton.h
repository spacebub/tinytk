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
    //! Square button that shows a single glyph and no text.
    //!
    //! The button is drawn as a square of \ref size() centred in its box. It does not take keyboard focus. Only
    //! \ref glyph() repaints. The other chained setters are meant for construction.
    class GlyphButton : public Widget {
    public:
        //! Creates a button showing `glyph` that calls `clicked` on each click. `clicked` may be empty.
        GlyphButton(Glyphs::Glyph glyph, std::function<void()> clicked);

        //! Sets the glyph to `glyph`, repaints when it changed, and returns this button.
        GlyphButton *glyph(Glyphs::Glyph glyph);

        //! Sets the side of the drawn square and of the natural size to `value` pixels and returns this button.
        //! Defaults to \ref Theme::controlSmall.
        GlyphButton *size(double value);

        //! Sets the glyph colour to `rest` normally and `lit` on hover, blended while the hover fades, and returns
        //! this button.
        GlyphButton *tone(Theme::Tone rest, Theme::Tone lit);

        //! Sets the background drawn on hover to `tone` and returns this button. A fully transparent `tone` falls
        //! back to the palette's hover colour.
        GlyphButton *wash(Theme::Tone tone);

        //! Sets whether the button has a raised background and a border, and returns this button.
        GlyphButton *outlined(bool value = true);

        //! Sets a fixed rotation of the glyph to `degrees` about its centre and returns this button.
        GlyphButton *turn(double degrees);

        //! Sets the extra rotation in degrees that \ref spun() animates to and returns this button. Until it is
        //! called the extra rotation is 0, so \ref spun() has no visible effect.
        GlyphButton *spin(double degrees = 45.0);

        //! Sets the tooltip, \ref Widget::hint, to `text` and returns this button.
        GlyphButton *tooltip(std::string text);

        //! Returns \ref fixedWidth when it is set, otherwise the side set by \ref size().
        double natural_width(Typeface &type) override;

        //! Returns \ref fixedHeight when it is set, otherwise the side set by \ref size().
        double natural_height(Typeface &type, double width) override;

        //! Paints the optional raised background and border, the hover wash and the glyph. A disabled button
        //! draws its glyph faded.
        void paint(const Painter &painter) override;

        //! Takes the press when the button is enabled.
        bool press(const Pointer &at) override;

        //! Calls the click callback when `at` is inside the button and it is enabled.
        void release(const Pointer &at) override;

        //! Called when the pointer moves onto the button. Starts the hover highlight.
        void enter() override;

        //! Called when the pointer moves off the button. Fades out the hover highlight.
        void leave() override;

        //! Animates the glyph to the rotation set by \ref spin() when `on` is true, or back to none when false.
        //!
        //! Suits a button that holds something open, such as a menu. Each call starts a new run of 0.16 seconds.
        void spun(bool on);

        //! Steps the hover and spin animations, repaints and returns whether either is still running.
        bool advance(double now) override;

    protected:
        //! Called when the \ref Theme has changed. Rereads the colours of palette slots given to \ref tone() and
        //! \ref wash().
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
