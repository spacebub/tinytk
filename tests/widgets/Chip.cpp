// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Screen.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/controls/Chip.h"

namespace {
    using widgets::Painted;

    constexpr BLRect BOX{40, 40, 140, 26};
    constexpr BLRectI PIXELS{40, 40, 140, 26};
    constexpr BLRectI TEXT{47, 40, 126, 26};

    // Inside the body, left of the text padding and clear of the border.
    constexpr int GROUND_X = 43;
    constexpr int GROUND_Y = 53;

    ttk::Typeface &type() {
        return tests::fonts();
    }

    struct Mounted {
        tests::Canvas canvas;
        ttk::Chip *chip = nullptr;

        explicit Mounted(const std::string &text = "-complevel 9", const BLRect &box = BOX) {
            chip = widgets::mount(canvas, std::make_unique<ttk::Chip>(text, "Compatibility level"), box);
        }

        [[nodiscard]] std::uint32_t ground() const { return canvas.at(GROUND_X, GROUND_Y); }
        [[nodiscard]] std::uint32_t edge() const { return canvas.at(static_cast<int>(BOX.x), GROUND_Y); }

        [[nodiscard]] std::uint32_t ink() const { return widgets::peak(canvas, TEXT, ground()); }
    };

    TEST(Chip, AboutBecomesTheHint) {
        const ttk::Chip chip("-fast", "Faster monsters");

        EXPECT_EQ(chip.hint, "Faster monsters");
    }

    TEST(Chip, IsHitAnywhereInItsBox) {
        ttk::Chip chip("-fast", "Faster monsters");

        chip.place(BOX, type());

        EXPECT_EQ(chip.at(BOX.x, BOX.y), &chip);
        EXPECT_EQ(chip.at(BOX.x + BOX.w - 1, BOX.y + BOX.h - 1), &chip);
        EXPECT_EQ(chip.at(BOX.x + BOX.w, BOX.y), nullptr);
    }

    TEST(Chip, PlainReturnsTheChip) {
        ttk::Chip chip("-fast", "");

        EXPECT_EQ(chip.plain(), &chip);
    }

    TEST(Chip, FixedSizesOverrideTheNaturalOnes) {
        ttk::Chip chip("-fast", "");

        chip.fixedWidth = 90;
        chip.fixedHeight = 12;

        EXPECT_DOUBLE_EQ(chip.natural_width(type()), 90);
        EXPECT_DOUBLE_EQ(chip.natural_height(type(), 500), 12);
    }

    TEST_F(Painted, ChipNaturalWidthIsMonoTextPlusPadding) {
        ttk::Chip chip("-complevel 9", "");

        EXPECT_DOUBLE_EQ(chip.natural_width(type()),
                         type().width(type().at(ttk::Typeface::mono, ttk::Theme::fontSmall), "-complevel 9") + 14.0);
    }

    TEST_F(Painted, ChipMonoFaceGivesEveryCharacterTheSameWidth) {
        ttk::Chip narrow("iiii", "");
        ttk::Chip wide("MMMM", "");

        EXPECT_DOUBLE_EQ(narrow.natural_width(type()), wide.natural_width(type()));
    }

    TEST_F(Painted, ChipPlainUsesTheProportionalFace) {
        ttk::Chip chip("-complevel 9", "");

        chip.plain();

        EXPECT_DOUBLE_EQ(chip.natural_width(type()),
                         type().width(type().at(ttk::Typeface::regular, ttk::Theme::fontSmall), "-complevel 9") + 14.0);
        EXPECT_DOUBLE_EQ(chip.natural_height(type(), 100),
                         type().line_height(type().at(ttk::Typeface::regular, ttk::Theme::fontSmall)) + 6.0);
    }

    TEST_F(Painted, ChipNaturalHeightIsTheLinePlusPadding) {
        ttk::Chip chip("-fast", "");

        EXPECT_DOUBLE_EQ(chip.natural_height(type(), 100),
                         type().line_height(type().at(ttk::Typeface::mono, ttk::Theme::fontSmall)) + 6.0);
        EXPECT_DOUBLE_EQ(chip.natural_height(type(), 0), chip.natural_height(type(), 1000));
    }

    TEST_F(Painted, ChipEmptyTextIsOnlyPadding) {
        ttk::Chip chip("", "");

        EXPECT_DOUBLE_EQ(chip.natural_width(type()), 14.0);
    }

    TEST_F(Painted, ChipUtf8TextIsMeasured) {
        ttk::Chip chip("é й ✓", "");

        EXPECT_DOUBLE_EQ(chip.natural_width(type()),
                         type().width(type().at(ttk::Typeface::mono, ttk::Theme::fontSmall), "é й ✓") + 14.0);
    }

    TEST_F(Painted, ChipRestsAsFaintTextWithNoBackground) {
        const Mounted mounted;

        EXPECT_EQ(mounted.ground(), widgets::background());
        EXPECT_EQ(mounted.edge(), widgets::background());
        EXPECT_LT(widgets::distance(mounted.ink(), widgets::palette().faint.value),
                  widgets::distance(mounted.ink(), widgets::palette().text.value))
                << std::hex << mounted.ink();
        EXPECT_EQ(widgets::outside(mounted.canvas, PIXELS), 0U);
    }

    TEST_F(Painted, ChipHoverShowsARaisedBackgroundAndText) {
        Mounted mounted;

        widgets::hover(mounted.canvas, BOX.x + 20, BOX.y + 10);

        const std::size_t painted = mounted.canvas.frame();

        EXPECT_TRUE(mounted.chip->hovered());
        EXPECT_LE(painted, widgets::area(BOX));
        EXPECT_EQ(mounted.ground(), widgets::palette().raised.value);
        EXPECT_TRUE(widgets::near(mounted.edge(), widgets::over(widgets::palette().raised,
                                                                ttk::Theme::alpha(widgets::palette().text, 0.3))))
                << std::hex << mounted.edge();
        EXPECT_LT(widgets::distance(mounted.ink(), widgets::palette().text.value),
                  widgets::distance(mounted.ink(), widgets::palette().faint.value));
        EXPECT_TRUE(mounted.canvas.matches_full());
    }

    TEST_F(Painted, ChipLeaveDropsTheBackground) {
        Mounted mounted;
        const std::vector<std::uint32_t> rest = mounted.canvas.pixels();

        widgets::hover(mounted.canvas, BOX.x + 20, BOX.y + 10);
        mounted.canvas.frame();
        widgets::hover(mounted.canvas, 300, 200);

        const std::size_t painted = mounted.canvas.frame();

        EXPECT_FALSE(mounted.chip->hovered());
        EXPECT_LE(painted, widgets::area(BOX));
        EXPECT_EQ(mounted.canvas.pixels(), rest);
    }

    TEST_F(Painted, ChipTightShowsTheWarningColours) {
        Mounted mounted;

        mounted.chip->set_tight(true);

        const std::size_t painted = mounted.canvas.frame();

        EXPECT_GT(painted, 0U);
        EXPECT_LE(painted, widgets::area(BOX));
        EXPECT_EQ(mounted.ground(), widgets::palette().warningSoft.value);
        EXPECT_TRUE(widgets::near(mounted.edge(), widgets::over(widgets::palette().warningSoft,
                                                                ttk::Theme::alpha(widgets::palette().warning, 0.3))))
                << std::hex << mounted.edge();
        EXPECT_LT(widgets::distance(mounted.ink(), widgets::palette().warning.value),
                  widgets::distance(mounted.ink(), widgets::palette().faint.value));
        EXPECT_TRUE(mounted.canvas.matches_full());
    }

    TEST_F(Painted, ChipTightWinsOverHover) {
        Mounted mounted;

        mounted.chip->set_tight(true);
        widgets::hover(mounted.canvas, BOX.x + 20, BOX.y + 10);
        mounted.canvas.frame();

        EXPECT_EQ(mounted.ground(), widgets::palette().warningSoft.value);
    }

    TEST_F(Painted, ChipTightOffRestoresTheRestingLook) {
        Mounted mounted;
        const std::vector<std::uint32_t> rest = mounted.canvas.pixels();

        mounted.chip->set_tight(true);
        mounted.canvas.frame();
        mounted.chip->set_tight(false);
        mounted.canvas.frame();

        EXPECT_EQ(mounted.canvas.pixels(), rest);
    }

    TEST_F(Painted, ChipTightToTheSameStateDamagesNothing) {
        Mounted mounted;

        mounted.chip->set_tight(false);

        EXPECT_FALSE(mounted.canvas.root().dirty());

        mounted.chip->set_tight(true);
        mounted.canvas.frame();
        mounted.chip->set_tight(true);

        EXPECT_FALSE(mounted.canvas.root().dirty());
    }

    TEST_F(Painted, ChipEmptyTextPaintsNothingEvenTightOrHovered) {
        Mounted mounted("");

        mounted.chip->set_tight(true);
        widgets::hover(mounted.canvas, BOX.x + 20, BOX.y + 10);
        mounted.canvas.frame();

        EXPECT_EQ(widgets::ink(mounted.canvas, PIXELS, widgets::background()).pixels, 0U);
    }

    TEST_F(Painted, ChipSetTextRepaintsOnlyTheChip) {
        Mounted mounted;
        const std::vector<std::uint32_t> before = mounted.canvas.pixels();

        mounted.chip->set_text("-skill 4");

        const std::size_t painted = mounted.canvas.frame();

        EXPECT_GT(painted, 0U);
        EXPECT_LE(painted, widgets::area(BOX));
        EXPECT_NE(mounted.canvas.pixels(), before);
        EXPECT_TRUE(mounted.canvas.matches_full());
    }

    TEST_F(Painted, ChipSetTextToTheSameTextDamagesNothing) {
        Mounted mounted;

        mounted.chip->set_text("-complevel 9");

        EXPECT_FALSE(mounted.canvas.root().dirty());
    }

    TEST_F(Painted, ChipLongerTextIsElidedToTheBox) {
        Mounted mounted;

        mounted.chip->set_text("-file eviternity.wad -file eviternity_music.wad -skill 4 -warp 1");

        EXPECT_FALSE(mounted.canvas.root().settle()) << "set_text asked for a layout";

        mounted.canvas.frame();

        const widgets::Ink drawn = widgets::ink(mounted.canvas, PIXELS, widgets::background());

        EXPECT_LE(drawn.right, PIXELS.x + PIXELS.w - 5) << "text ran into the right padding";
        EXPECT_EQ(widgets::outside(mounted.canvas, PIXELS), 0U);
        EXPECT_TRUE(mounted.canvas.matches_full());
    }

    TEST_F(Painted, ChipTextStartsAfterThePadding) {
        const Mounted mounted;
        const widgets::Ink drawn = widgets::ink(mounted.canvas, PIXELS, widgets::background());

        EXPECT_GE(drawn.left, PIXELS.x + 6);
        EXPECT_LE(drawn.left, PIXELS.x + 10);
    }

    TEST_F(Painted, ChipPressIsNotTaken) {
        Mounted mounted;

        mounted.canvas.root().press(ttk::Pointer{.x = BOX.x + 20, .y = BOX.y + 10});

        EXPECT_EQ(mounted.canvas.root().grabbed(), nullptr);
        EXPECT_FALSE(mounted.chip->pressed());
        EXPECT_EQ(mounted.canvas.root().hovered(), mounted.chip) << "a chip is rested on for its tooltip";
    }

    TEST_F(Painted, ChipBackgroundFollowsAPaletteChange) {
        Mounted mounted;

        mounted.chip->set_tight(true);
        ttk::Theme::set_mode(ttk::Theme::Mode::Light);
        mounted.canvas.frame();

        EXPECT_EQ(mounted.ground(), widgets::palette().warningSoft.value);

        mounted.chip->set_tight(false);
        widgets::hover(mounted.canvas, BOX.x + 20, BOX.y + 10);
        mounted.canvas.frame();

        EXPECT_EQ(mounted.ground(), widgets::palette().raised.value);
        EXPECT_TRUE(mounted.canvas.matches_full());
    }

    TEST_F(Painted, ChipInATinyBoxPaintsOnlyThere) {
        constexpr BLRect tiny{50, 50, 10, 8};
        Mounted mounted("-complevel 9", tiny);

        widgets::hover(mounted.canvas, 52, 52);
        mounted.canvas.frame();

        EXPECT_EQ(widgets::outside(mounted.canvas, widgets::pixels_of(tiny)), 0U);
        EXPECT_TRUE(mounted.canvas.matches_full());
    }
}
