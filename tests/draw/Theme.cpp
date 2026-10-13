// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cstdint>

#include <gtest/gtest.h>

#include "ttk/draw/Theme.h"

namespace {
    namespace Theme = ttk::Theme;

    bool same(const Theme::Palette &one, const Theme::Palette &two) {
        return one.background.value == two.background.value && one.text.value == two.text.value
               && one.accent.value == two.accent.value && one.scrim.value == two.scrim.value
               && one.headingWeight == two.headingWeight && one.dark == two.dark;
    }

    // Theme state is process wide. Every case puts back what it found.
    class Themed : public ::testing::Test {
    protected:
        void SetUp() override {
            _dark = Theme::palette(Theme::Mode::Dark);
            _light = Theme::palette(Theme::Mode::Light);
            _mode = Theme::mode();
            _systemDark = &Theme::palette(Theme::Mode::System) == &Theme::palette(Theme::Mode::Dark);
        }

        void TearDown() override {
            Theme::configure({.dark = _dark, .light = _light, .mode = _mode});
            Theme::set_system_dark(_systemDark);
        }

    private:
        Theme::Palette _dark{};
        Theme::Palette _light{};
        Theme::Mode _mode = Theme::Mode::System;
        bool _systemDark = true;
    };

    TEST_F(Themed, DefaultsToSystemShowingDark) {
        EXPECT_EQ(Theme::mode(), Theme::Mode::System);
        EXPECT_TRUE(Theme::palette().dark);
        EXPECT_EQ(&Theme::palette(), &Theme::palette(Theme::Mode::Dark));
    }

    TEST_F(Themed, BuiltInPalettesAreLightAndDark) {
        const Theme::Palette &dark = Theme::palette(Theme::Mode::Dark);
        const Theme::Palette &light = Theme::palette(Theme::Mode::Light);

        EXPECT_TRUE(dark.dark);
        EXPECT_FALSE(light.dark);
        EXPECT_NE(dark.background.value, light.background.value);
        EXPECT_NE(dark.text.value, light.text.value);
        EXPECT_GE(dark.headingWeight, 600);
        EXPECT_GE(light.headingWeight, 600);
        EXPECT_LT(dark.background.r() + dark.background.g() + dark.background.b(),
                  light.background.r() + light.background.g() + light.background.b());
        EXPECT_EQ(dark.background.a(), 0xffU) << "the window background is translucent";
        EXPECT_EQ(light.background.a(), 0xffU);
    }

    TEST_F(Themed, PaletteOnScreenIsTheOneTheModeSelects) {
        for (const Theme::Mode mode : {Theme::Mode::Light, Theme::Mode::Dark, Theme::Mode::System}) {
            Theme::set_mode(mode);

            EXPECT_EQ(Theme::mode(), mode);
            EXPECT_EQ(&Theme::palette(), &Theme::palette(mode));
        }
    }

    TEST_F(Themed, SystemFollowsTheDesktop) {
        Theme::set_system_dark(false);

        EXPECT_EQ(&Theme::palette(Theme::Mode::System), &Theme::palette(Theme::Mode::Light));

        Theme::set_system_dark(true);

        EXPECT_EQ(&Theme::palette(Theme::Mode::System), &Theme::palette(Theme::Mode::Dark));
    }

    TEST_F(Themed, SetModeAdvancesTheRevisionOnlyWhenThePaletteChanges) {
        Theme::set_system_dark(true);
        Theme::set_mode(Theme::Mode::System);

        const std::uint32_t before = Theme::revision();

        Theme::set_mode(Theme::Mode::Dark);

        EXPECT_EQ(Theme::revision(), before) << "System and Dark show the same palette";

        Theme::set_mode(Theme::Mode::Light);

        EXPECT_NE(Theme::revision(), before);

        const std::uint32_t light = Theme::revision();

        Theme::set_mode(Theme::Mode::Light);

        EXPECT_EQ(Theme::revision(), light);
    }

    TEST_F(Themed, DesktopChangeAdvancesTheRevisionOnlyWhenShown) {
        Theme::set_mode(Theme::Mode::Light);

        const std::uint32_t before = Theme::revision();

        Theme::set_system_dark(false);
        Theme::set_system_dark(true);

        EXPECT_EQ(Theme::revision(), before) << "a fixed mode reacted to the desktop";

        Theme::set_mode(Theme::Mode::System);

        const std::uint32_t system = Theme::revision();

        Theme::set_system_dark(false);

        EXPECT_NE(Theme::revision(), system);
        EXPECT_FALSE(Theme::palette().dark);

        const std::uint32_t light = Theme::revision();

        Theme::set_system_dark(false);

        EXPECT_EQ(Theme::revision(), light) << "telling the same preference again advanced the revision";
    }

    TEST_F(Themed, CycleGoesSystemLightDarkSystem) {
        Theme::set_mode(Theme::Mode::System);

        EXPECT_EQ(Theme::cycle_mode(), Theme::Mode::Light);
        EXPECT_EQ(Theme::mode(), Theme::Mode::Light);
        EXPECT_EQ(Theme::cycle_mode(), Theme::Mode::Dark);
        EXPECT_EQ(Theme::cycle_mode(), Theme::Mode::System);
        EXPECT_EQ(Theme::mode(), Theme::Mode::System);
    }

    TEST_F(Themed, CycleShowsEachPalette) {
        Theme::set_mode(Theme::Mode::System);
        Theme::cycle_mode();

        EXPECT_FALSE(Theme::palette().dark);

        Theme::cycle_mode();

        EXPECT_TRUE(Theme::palette().dark);
    }

    TEST_F(Themed, ConfigureReplacesPalettesAndMode) {
        Theme::Setup setup{
                .dark = Theme::palette(Theme::Mode::Dark),
                .light = Theme::palette(Theme::Mode::Light),
                .mode = Theme::Mode::Light,
        };

        setup.light.accent = BLRgba32(0xff123456);
        setup.dark.accent = BLRgba32(0xff654321);

        const std::uint32_t before = Theme::revision();

        Theme::configure(setup);

        EXPECT_NE(Theme::revision(), before);
        EXPECT_EQ(Theme::mode(), Theme::Mode::Light);
        EXPECT_EQ(Theme::palette().accent.value, 0xff123456U);
        EXPECT_EQ(Theme::palette(Theme::Mode::Dark).accent.value, 0xff654321U);
    }

    TEST_F(Themed, ConfigureAlwaysAdvancesTheRevision) {
        const Theme::Setup setup{
                .dark = Theme::palette(Theme::Mode::Dark),
                .light = Theme::palette(Theme::Mode::Light),
                .mode = Theme::mode(),
        };
        const std::uint32_t before = Theme::revision();

        Theme::configure(setup);

        EXPECT_NE(Theme::revision(), before);
    }

    TEST_F(Themed, PaletteReferencesSeeConfigure) {
        const Theme::Palette &light = Theme::palette(Theme::Mode::Light);
        Theme::Setup setup{.dark = Theme::palette(Theme::Mode::Dark), .light = light, .mode = Theme::Mode::Dark};

        setup.light.text = BLRgba32(0xff010203);
        Theme::configure(setup);

        EXPECT_EQ(light.text.value, 0xff010203U) << "the reference kept the old palette";
        EXPECT_EQ(&light, &Theme::palette(Theme::Mode::Light));
    }

    TEST_F(Themed, DefaultSetupPalettesAreZero) {
        const Theme::Setup setup{};

        EXPECT_EQ(setup.dark.background.value, 0U);
        EXPECT_EQ(setup.light.text.value, 0U);
        EXPECT_EQ(setup.dark.headingWeight, 0);
        EXPECT_FALSE(setup.light.dark);
        EXPECT_EQ(setup.mode, Theme::Mode::System);
    }

    TEST_F(Themed, ConfigureWithSystemShowsTheDesktopChoice) {
        Theme::set_system_dark(false);

        const Theme::Palette dark = Theme::palette(Theme::Mode::Dark);
        const Theme::Palette light = Theme::palette(Theme::Mode::Light);

        Theme::configure({.dark = dark, .light = light, .mode = Theme::Mode::System});

        EXPECT_TRUE(same(Theme::palette(), light));
    }

    TEST_F(Themed, ToneFollowsItsSlotOnRestyle) {
        Theme::set_mode(Theme::Mode::Dark);

        Theme::Tone tone(&Theme::Palette::accent);

        EXPECT_EQ(tone.colour().value, Theme::palette(Theme::Mode::Dark).accent.value);

        Theme::set_mode(Theme::Mode::Light);

        EXPECT_EQ(tone.colour().value, Theme::palette(Theme::Mode::Dark).accent.value) << "changed before restyle";

        tone.restyle();

        EXPECT_EQ(tone.colour().value, Theme::palette(Theme::Mode::Light).accent.value);
    }

    TEST_F(Themed, DerivedToneIsComputedAgainOnRestyle) {
        Theme::set_mode(Theme::Mode::Dark);

        Theme::Tone tone([](const Theme::Palette &palette) { return Theme::alpha(palette.text, 0.5); });

        EXPECT_EQ(tone.colour().value, Theme::alpha(Theme::palette(Theme::Mode::Dark).text, 0.5).value);

        Theme::set_mode(Theme::Mode::Light);
        tone.restyle();

        EXPECT_EQ(tone.colour().value, Theme::alpha(Theme::palette(Theme::Mode::Light).text, 0.5).value);
    }

    TEST_F(Themed, FixedToneIgnoresRestyle) {
        Theme::Tone tone(BLRgba32(0x80abcdef));

        Theme::set_mode(Theme::Mode::Light);
        tone.restyle();

        EXPECT_EQ(tone.colour().value, 0x80abcdefU);
    }

    struct Brand {
        BLRgba32 tint;
    };

    constexpr Theme::Extension<Brand> BRAND{
            .dark = {.tint = BLRgba32{0xff80c0ff}},
            .light = {.tint = BLRgba32{0xff1a5fa0}},
    };

    TEST_F(Themed, ExtensionPicksTheSetOfThePalette) {
        EXPECT_EQ(BRAND.of(Theme::palette(Theme::Mode::Dark)).tint.value, 0xff80c0ffU);
        EXPECT_EQ(BRAND.of(Theme::palette(Theme::Mode::Light)).tint.value, 0xff1a5fa0U);

        Theme::Tone tint([](const Theme::Palette &palette) { return BRAND.of(palette).tint; });

        Theme::set_mode(Theme::Mode::Light);
        tint.restyle();

        EXPECT_EQ(tint.colour().value, 0xff1a5fa0U);
    }

    TEST(ThemeColour, AlphaScalesOnlyTheAlpha) {
        EXPECT_EQ(Theme::alpha(BLRgba32(0x80102030), 0.5).value, 0x40102030U);
        EXPECT_EQ(Theme::alpha(BLRgba32(0xff102030), 1.0).value, 0xff102030U);
        EXPECT_EQ(Theme::alpha(BLRgba32(0xff102030), 0.0).value, 0x00102030U);
    }

    TEST(ThemeColour, AlphaClampsTheFraction) {
        EXPECT_EQ(Theme::alpha(BLRgba32(0x80102030), 2.0).value, 0x80102030U);
        EXPECT_EQ(Theme::alpha(BLRgba32(0x80102030), -1.0).value, 0x00102030U);
    }

    TEST(ThemeColour, DarkerDividesTheChannels) {
        EXPECT_EQ(Theme::darker(BLRgba32(0x80c86432), 1.0).value, 0x80643219U);
        EXPECT_EQ(Theme::darker(BLRgba32(0x80c86432), 0.0).value, 0x80c86432U);
        EXPECT_EQ(Theme::darker(BLRgba32(0x80c86432), -3.0).value, 0x80c86432U) << "a negative factor brightened";
    }

    TEST(ThemeColour, LighterMultipliesAndCaps) {
        EXPECT_EQ(Theme::lighter(BLRgba32(0x40644020), 1.0).value, 0x40c88040U);
        EXPECT_EQ(Theme::lighter(BLRgba32(0x40c88040), 1.0).value, 0x40ffff80U) << "channels were not capped";
        EXPECT_EQ(Theme::lighter(BLRgba32(0x40644020), -1.0).value, 0x40644020U);
        EXPECT_EQ(Theme::lighter(BLRgba32(0xff000000), 5.0).value, 0xff000000U);
    }

    TEST(ThemeColour, MixInterpolatesAndKeepsTheUnderAlpha) {
        const BLRgba32 under(0x80000000);
        const BLRgba32 over(0xffc86432);

        EXPECT_EQ(Theme::mix(under, over, 0.0).value, 0x80000000U);
        EXPECT_EQ(Theme::mix(under, over, 1.0).value, 0x80c86432U);
        EXPECT_EQ(Theme::mix(under, over, 0.5).value, 0x80643219U);
    }

    TEST(ThemeColour, MixClampsTheAmount) {
        const BLRgba32 under(0xff102030);
        const BLRgba32 over(0xffffffff);

        EXPECT_EQ(Theme::mix(under, over, 3.0).value, 0xffffffffU);
        EXPECT_EQ(Theme::mix(under, over, -3.0).value, 0xff102030U);
    }

    TEST(ThemeColour, MixIsScaledByTheOverAlpha) {
        EXPECT_EQ(Theme::mix(BLRgba32(0xff000000), BLRgba32(0x00ffffff), 1.0).value, 0xff000000U);
        EXPECT_EQ(Theme::mix(BLRgba32(0xff000000), BLRgba32(0x80c8c8c8), 1.0).value, 0xff646464U);
    }

    TEST(ThemeColour, MixTowardDarkerDoesNotWrap) {
        EXPECT_EQ(Theme::mix(BLRgba32(0xffffffff), BLRgba32(0xff000000), 0.25).value, 0xffbfbfbfU);
    }

    TEST(ThemeSizes, FontSizesGrowInOrder) {
        EXPECT_LT(Theme::fontTiny, Theme::fontSmall);
        EXPECT_LT(Theme::fontSmall, Theme::fontBody);
        EXPECT_LT(Theme::fontBody, Theme::fontMedium);
        EXPECT_LT(Theme::fontMedium, Theme::fontLarge);
        EXPECT_LT(Theme::fontLarge, Theme::fontTitle);
        EXPECT_LT(Theme::fontTitle, Theme::fontDisplay);
        EXPECT_LT(Theme::fontDisplay, Theme::fontHero);
    }

    TEST(ThemeSizes, ControlsAndCornersAreOrdered) {
        EXPECT_LT(Theme::controlSmall, Theme::control);
        EXPECT_LT(Theme::radiusSmall, Theme::radius);
        EXPECT_LT(Theme::radius, Theme::radiusLarge);
        EXPECT_GT(Theme::barHeight, Theme::control);
        EXPECT_GT(Theme::buttonWidth, Theme::control);
        EXPECT_GT(Theme::lane, 0.0);
    }
}
