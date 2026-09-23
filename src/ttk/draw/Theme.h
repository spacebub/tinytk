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

namespace ttk::Theme {
    enum class Mode : std::uint8_t {
        Light,
        Dark,
        System,
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

        BLRgba32 statusLaunching;
        BLRgba32 statusRunning;
        BLRgba32 statusFailing;
        BLRgba32 statusIdle;

        int headingWeight;
        bool dark;
    };

    const Palette &palette();
    const Palette &palette(Mode mode);

    Mode mode();
    void set_mode(Mode mode);

    Mode cycle_mode();

    struct Setup {
        Palette dark{};
        Palette light{};
        Mode mode = Mode::System;
    };

    void configure(const Setup &setup);

    // What the desktop asks for, as SDL reports it.
    void set_system_dark(bool dark);

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

    constexpr double barHeight = 54.0;

    constexpr double bleed = 16.0;
    constexpr double gutter = 20.0;

    constexpr double pageMargin = 22.0;
    constexpr double pageTop = 22.0;

    constexpr double lane = 10.0;

    // Moves on whenever the palette on screen changes.
    std::uint32_t revision();

    // A palette slot that follows the shade, or a fixed colour that does not.
    class Tone {
    public:
        Tone(BLRgba32 Palette::*slot);
        explicit Tone(const BLRgba32 colour) : _colour(colour) {}

        void restyle();

        [[nodiscard]] BLRgba32 colour() const { return _colour; }

    private:
        BLRgba32 Palette::*_slot = nullptr;
        BLRgba32 _colour{};
    };

    BLRgba32 alpha(BLRgba32 tone, double fraction);
    BLRgba32 darker(BLRgba32 tone, double factor);
    BLRgba32 lighter(BLRgba32 tone, double factor);

    BLRgba32 mix(BLRgba32 under, BLRgba32 over, double amount);
}


#endif //TTK_DRAW_THEME_H
