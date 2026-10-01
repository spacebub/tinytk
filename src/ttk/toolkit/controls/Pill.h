// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_PILL_H
#define TTK_CONTROLS_PILL_H


#include <string>

#include "ttk/draw/Theme.h"
#include "ttk/draw/Glyphs.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
    //! Rounded badge with a short text or a glyph, and a dot before it by default.
    //!
    //! The background is a soft wash and the text, dot and border take a stronger tone, darkened in a light
    //! palette. The chained setters are meant for construction and do not repaint.
    class Pill : public Widget {

    public:
        //! Colour scheme of a pill.
        enum class Kind : std::uint8_t {
            //! The palette's accent colours.
            None,
            //! The palette's muted colours.
            Muted,
            //! The palette's success colours.
            Success,
            //! The palette's warning colours.
            Warning,
            //! The palette's danger colours.
            Danger,
        };

        //! Creates an accent coloured pill showing `text` with a dot.
        explicit Pill(std::string text = {});

        //! Sets the text to `text` and repaints. The layout is not redone, so the width stays as it was.
        void set_text(std::string text);

        //! Returns the strong tone of `kind` in the current palette, before the darkening a pill applies to it in
        //! a light palette.
        [[nodiscard]] static BLRgba32 tone_of(Kind kind);

        //! Returns the soft background colour of `kind` in the current palette.
        [[nodiscard]] static BLRgba32 wash_of(Kind kind);

        //! Sets the colours to those of `value` and returns this pill.
        Pill *kind(Kind value);

        //! Sets whether a dot is drawn before the text or glyph and returns this pill.
        Pill *dot(bool value);

        //! Sets a glyph that is drawn in place of the text and returns this pill. \ref Glyphs::Glyph::Empty shows
        //! the text.
        Pill *glyph(Glyphs::Glyph glyph);

        //! Sets the strong tone to `tone` and the background to `wash`, overriding \ref kind(), and returns this
        //! pill.
        Pill *tones(Theme::Tone tone, Theme::Tone wash);

        //! Returns \ref fixedWidth when it is set, otherwise the width of the text or glyph, the dot and padding.
        double natural_width(Typeface &type) override;

        //! Returns 26 pixels. \ref fixedHeight is ignored.
        double natural_height(Typeface & /*type*/, double /*width*/) override { return 26.0; }

        //! Paints the rounded background and border, then the dot and the text or glyph centred in the box.
        void paint(const Painter &painter) override;

    protected:
        //! Called when the \ref Theme has changed. Rereads the colours of palette slots and redoes the darkening
        //! for the new palette.
        void restyle() override;

    private:
        [[nodiscard]] static BLRgba32 Theme::Palette::*tone_slot(Kind kind);
        [[nodiscard]] static BLRgba32 Theme::Palette::*wash_slot(Kind kind);

        void resolve();

        std::string _text;
        Glyphs::Glyph _glyph{};

        Theme::Tone _tone{&Theme::Palette::accent};
        Theme::Tone _wash{&Theme::Palette::accentSoft};
        BLRgba32 _ink{};

        bool _dot = true;
    };
}


#endif //TTK_CONTROLS_PILL_H
