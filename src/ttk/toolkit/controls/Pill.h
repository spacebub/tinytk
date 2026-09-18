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
        Pill *tones(BLRgba32 tone, BLRgba32 wash);

        double natural_width(Typeface &type) override;
        double natural_height(Typeface & /*type*/, double /*width*/) override { return 26.0; }

        void paint(const Painter &painter) override;

    private:
        [[nodiscard]] BLRgba32 tone() const;
        [[nodiscard]] BLRgba32 wash() const;

        std::string _text;
        Kind _kind{};
        Glyphs::Glyph _glyph{};

        BLRgba32 _tone{};
        BLRgba32 _wash{};
        bool _set = false;
        bool _toneDark = Theme::dark();

        bool _dot = true;
    };
}


#endif //TTK_CONTROLS_PILL_H
