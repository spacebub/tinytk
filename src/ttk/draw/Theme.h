// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_DRAW_THEME_H
#define TTK_DRAW_THEME_H


#include <cstdint>

#include <blend2d/blend2d.h>

namespace ttk {
    namespace Theme {

        enum class Mode : std::uint8_t {
            Light,
            Dark,
            System
        };

        struct Palette {
            BLRgba32 background;
            BLRgba32 surface;
            BLRgba32 raised;
            BLRgba32 sunken;
            BLRgba32 field;

            BLRgba32 hover;
            BLRgba32 border;
            BLRgba32 borderStrong;

            BLRgba32 text;
            BLRgba32 muted;
            BLRgba32 faint;

            BLRgba32 accent;
            BLRgba32 accentHover;
            BLRgba32 accentText;
            BLRgba32 accentSoft;

            BLRgba32 mutedSoft;
            BLRgba32 success;
            BLRgba32 successSoft;
            BLRgba32 warning;
            BLRgba32 warningSoft;
            BLRgba32 danger;
            BLRgba32 dangerSoft;

            BLRgba32 shadow;
            BLRgba32 scrim;


            int headingWeight;
            bool dark;
        };

        const Palette &of();

        const Mode &mode();
        void set_mode(const Mode &mode);

        // What the desktop asks for, as SDL reports it.
        void set_system_dark(bool dark);

        // An application's own colours in place of the built in pair. Set before
        // anything is built, since a widget keeps the tone it was made under.
        void set_palettes(const Palette &dark, const Palette &light);

        bool dark();

        // The next shade in the cycle: system, light, dark.
        Mode next_mode();

        constexpr float fontTiny = 12.0F;
        constexpr float fontSmall = 13.0F;
        constexpr float fontBody = 14.0F;
        constexpr float fontMedium = 15.0F;
        constexpr float fontLarge = 17.0F;
        constexpr float fontTitle = 19.0F;
        constexpr float fontDisplay = 23.0F;
        constexpr float fontHero = 44.0F;

        constexpr double control = 42.0;
        constexpr double controlSmall = 32.0;
        constexpr double buttonWidth = 140.0;




        constexpr double radius = 12.0;
        constexpr double radiusSmall = 8.0;
        constexpr double radiusLarge = 18.0;
        constexpr double gap = 12.0;
        constexpr double pad = 20.0;

        // The title bar, and the square a window control occupies.
        constexpr double barHeight = 54.0;

        constexpr double bleed = 16.0;
        constexpr double gutter = 20.0;



        // A page is inset by the margin.
        constexpr double pageMargin = 22.0;
        constexpr double pageTop = 22.0;

        // An 8px lane with a 5px bar down the middle.
        constexpr double lane = 10.0;

        // The same slot in whichever shade is on now, for a tone taken from the palette
        // when a widget was built: a label toned `faint` under the dark shade keeps that
        // exact colour otherwise, and reads as near-white once the light one is on.
        BLRgba32 restated(BLRgba32 tone, bool wasDark);

        BLRgba32 alpha(BLRgba32 tone, double fraction);
        BLRgba32 darker(BLRgba32 tone, double factor);
        BLRgba32 lighter(BLRgba32 tone, double factor);

        // `over` composited onto `under`, which is what an opacity animation between two
        // opaque fills amounts to.
        BLRgba32 mix(BLRgba32 under, BLRgba32 over, double amount);

    }
}


#endif //TTK_DRAW_THEME_H
