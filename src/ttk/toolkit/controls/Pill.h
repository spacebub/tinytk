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
    class Pill : public Widget {

    public:
        enum class Kind : std::uint8_t {
            None, // defaults to Theme::Palette.accent
            Muted,
            Success,
            Warning,
            Danger,
        };

        explicit Pill(std::string text = {});

        void set_text(std::string text);

        // The five tones a pill is drawn in. Shared with whatever else carries a kind.
        [[nodiscard]] static BLRgba32 tone_of(Kind kind);
        [[nodiscard]] static BLRgba32 wash_of(Kind kind);

        // muted | warning | danger. Anything else is the accent.
        Pill *kind(Kind value);
        Pill *dot(bool value);
        Pill *glyph(Glyphs::Glyph glyph);
        Pill *tones(Theme::Tone tone, Theme::Tone wash);

        double natural_width(Typeface &type) override;
        double natural_height(Typeface & /*type*/, double /*width*/) override { return 26.0; }

        void paint(const Painter &painter) override;

    protected:
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
