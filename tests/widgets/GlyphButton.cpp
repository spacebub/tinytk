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
#include "ttk/draw/Glyphs.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/controls/GlyphButton.h"

namespace {
using ttk::Glyphs::Glyph;
using widgets::Painted;

constexpr BLRect BOX{40, 40, 32, 32};
constexpr BLRectI BODY{40, 40, 32, 32};
constexpr double MIDDLE = 56;
constexpr double SETTLED = 0.5;

// Inside the body, left of the glyph's square and clear of the border.
constexpr int WASH_X = 44;
constexpr int WASH_Y = 56;

struct Mounted {
    tests::Canvas canvas;
    ttk::GlyphButton *button = nullptr;
    int clicks = 0;

    explicit Mounted(const Glyph glyph = Glyph::Play, const BLRect &box = BOX, const bool outlined = false) {
        auto made = std::make_unique<ttk::GlyphButton>(glyph, [this] { ++clicks; });

        made->outlined(outlined);
        button = widgets::mount(canvas, std::move(made), box);
    }

    [[nodiscard]] std::size_t inked(const BLRgba32 colour, const int slack = 0) const {
        return widgets::count(canvas, BODY, colour, slack);
    }

    void lit() {
        widgets::hover(canvas, MIDDLE, MIDDLE);
        canvas.frame_at(SETTLED);
    }
};

TEST(GlyphButton, NaturalSizeIsTheSmallControl) {
    ttk::GlyphButton button(Glyph::Close, {});

    EXPECT_DOUBLE_EQ(button.natural_width(tests::fonts()), ttk::Theme::controlSmall);
    EXPECT_DOUBLE_EQ(button.natural_height(tests::fonts(), 500), ttk::Theme::controlSmall);
}

TEST(GlyphButton, SizeSetsBothNaturalSides) {
    ttk::GlyphButton button(Glyph::Close, {});

    button.size(20);

    EXPECT_DOUBLE_EQ(button.natural_width(tests::fonts()), 20);
    EXPECT_DOUBLE_EQ(button.natural_height(tests::fonts(), 500), 20);
}

TEST(GlyphButton, FixedSizesOverrideTheSide) {
    ttk::GlyphButton button(Glyph::Close, {});

    button.size(20);
    button.fixedWidth = 50;
    button.fixedHeight = 10;

    EXPECT_DOUBLE_EQ(button.natural_width(tests::fonts()), 50);
    EXPECT_DOUBLE_EQ(button.natural_height(tests::fonts(), 500), 10);
}

TEST(GlyphButton, ChainedSettersReturnTheButton) {
    ttk::GlyphButton button(Glyph::Close, {});

    EXPECT_EQ(button.glyph(Glyph::Plus), &button);
    EXPECT_EQ(button.size(24), &button);
    EXPECT_EQ(button.tone(&ttk::Theme::Palette::danger, &ttk::Theme::Palette::accent), &button);
    EXPECT_EQ(button.wash(&ttk::Theme::Palette::accentSoft), &button);
    EXPECT_EQ(button.outlined(), &button);
    EXPECT_EQ(button.turn(30), &button);
    EXPECT_EQ(button.spin(), &button);
    EXPECT_EQ(button.tooltip("Close"), &button);
}

TEST(GlyphButton, TooltipSetsTheHint) {
    ttk::GlyphButton button(Glyph::Close, {});

    button.tooltip("Close the dialog");

    EXPECT_EQ(button.hint, "Close the dialog");
    EXPECT_EQ(button.cursor, ttk::Cursor::Pointer);
}

TEST_F(Painted, GlyphButtonConstructionSettersDoNotDamage) {
    Mounted mounted;

    mounted.button->size(24)
            ->tone(&ttk::Theme::Palette::danger, &ttk::Theme::Palette::accent)
            ->wash(&ttk::Theme::Palette::accentSoft)
            ->outlined()
            ->turn(45)
            ->spin(90)
            ->tooltip("Tip");

    EXPECT_FALSE(mounted.canvas.root().dirty());
}

TEST_F(Painted, GlyphButtonRestsInTheMutedColourWithNoBackground) {
    const Mounted mounted;

    EXPECT_GT(mounted.inked(widgets::palette().muted), 0U);
    EXPECT_EQ(mounted.canvas.at(WASH_X, WASH_Y), widgets::background());
    EXPECT_EQ(mounted.canvas.at(static_cast<int>(BOX.x), WASH_Y), widgets::background());
}

TEST_F(Painted, GlyphButtonHoverTurnsTheGlyphToText) {
    Mounted mounted;

    mounted.lit();

    EXPECT_TRUE(mounted.button->hovered());
    EXPECT_GT(mounted.inked(widgets::palette().text), 0U);
    EXPECT_EQ(mounted.inked(widgets::palette().muted), 0U);
}

TEST_F(Painted, GlyphButtonHoverWashesTheBody) {
    Mounted mounted;

    mounted.lit();

    EXPECT_TRUE(widgets::near(mounted.canvas.at(WASH_X, WASH_Y),
                              widgets::over(widgets::palette().background, widgets::palette().hover)))
            << std::hex << mounted.canvas.at(WASH_X, WASH_Y);
}

TEST_F(Painted, GlyphButtonHoverFadesOutOnLeave) {
    Mounted mounted;
    const std::vector<std::uint32_t> rest = mounted.canvas.pixels();

    mounted.lit();
    widgets::hover(mounted.canvas, 300, 200);
    mounted.canvas.frame_at(SETTLED * 2);

    EXPECT_FALSE(mounted.button->hovered());
    EXPECT_EQ(mounted.canvas.pixels(), rest);
}

TEST_F(Painted, GlyphButtonHoverFadesOverATenth) {
    Mounted mounted;

    widgets::hover(mounted.canvas, MIDDLE, MIDDLE);
    mounted.canvas.frame_at(0.03);

    EXPECT_EQ(mounted.inked(widgets::palette().text), 0U) << "the hover jumped instead of fading";
    EXPECT_TRUE(mounted.canvas.root().busy());

    mounted.canvas.frame_at(0.11);
    mounted.canvas.frame_at(0.12);

    EXPECT_GT(mounted.inked(widgets::palette().text), 0U);
    EXPECT_FALSE(mounted.canvas.root().busy()) << "a settled hover kept the frame loop running";
}

TEST_F(Painted, GlyphButtonHoverRepaintsOnlyTheButton) {
    Mounted mounted;

    widgets::hover(mounted.canvas, MIDDLE, MIDDLE);

    for (const double now : {0.02, 0.05, SETTLED}) {
        const std::size_t painted = mounted.canvas.frame_at(now);

        EXPECT_GT(painted, 0U);
        EXPECT_LE(painted, widgets::area(BOX));
    }

    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, GlyphButtonCustomWashIsDrawnOnHover) {
    Mounted mounted;

    mounted.button->wash(ttk::Theme::Tone(BLRgba32(0xff204060)));
    mounted.lit();

    EXPECT_EQ(mounted.canvas.at(WASH_X, WASH_Y), 0xff204060U);
}

TEST_F(Painted, GlyphButtonTransparentWashFallsBackToHover) {
    Mounted mounted;

    mounted.button->wash(ttk::Theme::Tone(BLRgba32(0x00204060)));
    mounted.lit();

    EXPECT_TRUE(widgets::near(mounted.canvas.at(WASH_X, WASH_Y),
                              widgets::over(widgets::palette().background, widgets::palette().hover)))
            << std::hex << mounted.canvas.at(WASH_X, WASH_Y);
}

TEST_F(Painted, GlyphButtonToneSetsTheRestingAndLitColours) {
    Mounted mounted;

    mounted.button->tone(&ttk::Theme::Palette::danger, &ttk::Theme::Palette::success);
    mounted.canvas.full();

    EXPECT_GT(mounted.inked(widgets::palette().danger), 0U);
    EXPECT_EQ(mounted.inked(widgets::palette().muted), 0U);

    mounted.lit();

    EXPECT_GT(mounted.inked(widgets::palette().success), 0U);
    EXPECT_EQ(mounted.inked(widgets::palette().danger), 0U);
}

TEST_F(Painted, GlyphButtonFixedTonesAreDrawnAsGiven) {
    Mounted mounted;

    mounted.button->tone(ttk::Theme::Tone(BLRgba32(0xff10e0a0)), ttk::Theme::Tone(BLRgba32(0xffe010a0)));
    mounted.canvas.full();

    EXPECT_GT(mounted.inked(BLRgba32(0xff10e0a0)), 0U);
}

TEST_F(Painted, GlyphButtonOutlinedDrawsARaisedBodyAndBorder) {
    const Mounted mounted(Glyph::Play, BOX, true);

    EXPECT_EQ(mounted.canvas.at(WASH_X, WASH_Y), widgets::palette().raised.value);
    EXPECT_EQ(mounted.canvas.at(static_cast<int>(BOX.x), WASH_Y), widgets::palette().borderStrong.value);
}

TEST_F(Painted, GlyphButtonOutlinedBorderTakesTheLitColourOnHover) {
    Mounted mounted(Glyph::Play, BOX, true);

    mounted.lit();

    EXPECT_EQ(mounted.canvas.at(static_cast<int>(BOX.x), WASH_Y), widgets::palette().text.value);
    EXPECT_TRUE(widgets::near(mounted.canvas.at(WASH_X, WASH_Y),
                              widgets::over(widgets::palette().raised, widgets::palette().hover)));
}

TEST_F(Painted, GlyphButtonBodyIsCentredInALargerBox) {
    const Mounted mounted(Glyph::Play, BLRect{40, 40, 96, 48}, true);

    EXPECT_EQ(mounted.canvas.at(41, 64), widgets::background()) << "the body filled the whole box";
    EXPECT_EQ(mounted.canvas.at(72, 64), widgets::palette().borderStrong.value) << "left border not at 72";
    EXPECT_EQ(mounted.canvas.at(76, 64), widgets::palette().raised.value);
    EXPECT_EQ(mounted.canvas.at(76, 46), widgets::background()) << "the body is taller than its size";
}

TEST_F(Painted, GlyphButtonSizeSetsTheDrawnSquare) {
    tests::Canvas canvas;
    auto made = std::make_unique<ttk::GlyphButton>(Glyph::Play, nullptr);

    made->size(20)->outlined();
    widgets::mount(canvas, std::move(made), BOX);

    const widgets::Ink drawn = widgets::ink(canvas, BODY, widgets::background());

    EXPECT_EQ(drawn.left, 46);
    EXPECT_EQ(drawn.top, 46);
    EXPECT_EQ(drawn.right, 66);
    EXPECT_EQ(drawn.bottom, 66);
}

TEST_F(Painted, GlyphButtonDisabledFadesTheGlyph) {
    Mounted mounted;
    const std::size_t bright = widgets::contrast(mounted.canvas, BODY, widgets::background());

    mounted.button->set_enabled(false);

    const std::size_t painted = mounted.canvas.frame();
    const std::size_t faded = widgets::contrast(mounted.canvas, BODY, widgets::background());

    EXPECT_LE(painted, widgets::area(BOX));
    EXPECT_EQ(mounted.inked(widgets::palette().muted), 0U);
    EXPECT_GT(faded, 0U);
    EXPECT_LT(faded, bright);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, GlyphButtonDisabledIgnoresThePointer) {
    Mounted mounted;

    mounted.button->set_enabled(false);
    widgets::click(mounted.canvas, MIDDLE, MIDDLE);

    EXPECT_EQ(mounted.clicks, 0);
    EXPECT_FALSE(mounted.button->hovered());
    EXPECT_FALSE(mounted.button->press(ttk::Pointer{.x = MIDDLE, .y = MIDDLE}));

    mounted.button->release(ttk::Pointer{.x = MIDDLE, .y = MIDDLE});

    EXPECT_EQ(mounted.clicks, 0);
}

TEST_F(Painted, GlyphButtonReleaseInsideClicks) {
    Mounted mounted;

    widgets::click(mounted.canvas, MIDDLE, MIDDLE);

    EXPECT_EQ(mounted.clicks, 1);

    widgets::click(mounted.canvas, BOX.x, BOX.y);

    EXPECT_EQ(mounted.clicks, 2);
}

TEST_F(Painted, GlyphButtonIsATabStopThatReturnClicks) {
    Mounted mounted;

    mounted.canvas.root().focus_next(false);

    EXPECT_EQ(mounted.canvas.root().focused(), mounted.button);
    EXPECT_TRUE(mounted.canvas.root().key(widgets::key(ttk::Code::Return)));
    EXPECT_TRUE(mounted.canvas.root().key(widgets::key(ttk::Code::Space)));
    EXPECT_EQ(mounted.clicks, 2);

    mounted.button->set_enabled(false);

    EXPECT_FALSE(mounted.button->takes_focus());
}

TEST_F(Painted, GlyphButtonReleaseOutsideDoesNotClick) {
    Mounted mounted;

    mounted.canvas.root().press(ttk::Pointer{.x = MIDDLE, .y = MIDDLE});

    EXPECT_EQ(mounted.canvas.root().grabbed(), mounted.button);

    mounted.canvas.root().motion(200, 200);
    mounted.canvas.root().release(ttk::Pointer{.x = 200, .y = 200});

    EXPECT_EQ(mounted.clicks, 0);
}

TEST_F(Painted, GlyphButtonPressOutsideDoesNotReachIt) {
    Mounted mounted;

    widgets::click(mounted.canvas, BOX.x + BOX.w, MIDDLE);

    EXPECT_EQ(mounted.clicks, 0);
}

TEST_F(Painted, GlyphButtonWithoutCallbackTakesClicksQuietly) {
    tests::Canvas canvas;

    widgets::mount(canvas, std::make_unique<ttk::GlyphButton>(Glyph::Close, nullptr), BOX);
    widgets::click(canvas, MIDDLE, MIDDLE);

    EXPECT_EQ(canvas.root().grabbed(), nullptr);
}

TEST_F(Painted, GlyphButtonGlyphChangeRepaintsOnlyTheButton) {
    Mounted mounted(Glyph::Plus);
    const std::vector<std::uint32_t> before = mounted.canvas.pixels();

    mounted.button->glyph(Glyph::Close);

    const std::size_t painted = mounted.canvas.frame();

    EXPECT_GT(painted, 0U);
    EXPECT_LE(painted, widgets::area(BOX));
    EXPECT_NE(mounted.canvas.pixels(), before);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, GlyphButtonSameGlyphDamagesNothing) {
    Mounted mounted(Glyph::Plus);

    mounted.button->glyph(Glyph::Plus);

    EXPECT_FALSE(mounted.canvas.root().dirty());
}

TEST_F(Painted, GlyphButtonEmptyGlyphDrawsNothing) {
    const Mounted mounted(Glyph::Empty);

    EXPECT_EQ(widgets::ink(mounted.canvas, BODY, widgets::background()).pixels, 0U);
}

TEST_F(Painted, GlyphButtonTurnRotatesTheGlyph) {
    const Mounted plain;
    tests::Canvas canvas;
    auto made = std::make_unique<ttk::GlyphButton>(Glyph::Play, nullptr);

    made->turn(90);
    widgets::mount(canvas, std::move(made), BOX);

    EXPECT_NE(canvas.pixels(), plain.canvas.pixels());
    EXPECT_EQ(widgets::outside(canvas, BODY), 0U);
}

TEST_F(Painted, GlyphButtonSpunWithoutSpinChangesNothing) {
    Mounted mounted;
    const std::vector<std::uint32_t> before = mounted.canvas.pixels();

    mounted.button->spun(true);
    mounted.canvas.frame_at(0.05);
    mounted.canvas.frame_at(SETTLED);

    EXPECT_EQ(mounted.canvas.pixels(), before);
}

TEST_F(Painted, GlyphButtonSpunEndsAtTheSpinAngle) {
    tests::Canvas turned;
    auto still = std::make_unique<ttk::GlyphButton>(Glyph::Play, nullptr);

    still->turn(90);
    widgets::mount(turned, std::move(still), BOX);

    Mounted mounted;

    mounted.button->spin(90);
    mounted.button->spun(true);
    mounted.canvas.frame_at(SETTLED);

    EXPECT_EQ(mounted.canvas.pixels(), turned.pixels());
}

TEST_F(Painted, GlyphButtonSpunBackReturnsToRest) {
    Mounted mounted;
    const std::vector<std::uint32_t> rest = mounted.canvas.pixels();

    mounted.button->spin(90);
    mounted.button->spun(true);
    mounted.canvas.frame_at(SETTLED);
    mounted.button->spun(false);
    mounted.canvas.frame_at(SETTLED * 2);

    EXPECT_EQ(mounted.canvas.pixels(), rest);
}

TEST_F(Painted, GlyphButtonSpinRunsForSixteenHundredths) {
    Mounted mounted;
    const std::vector<std::uint32_t> rest = mounted.canvas.pixels();

    mounted.button->spin(90);
    mounted.button->spun(true);
    mounted.canvas.frame_at(0.04);

    const std::vector<std::uint32_t> partway = mounted.canvas.pixels();

    EXPECT_NE(partway, rest);
    EXPECT_TRUE(mounted.canvas.root().busy());

    mounted.canvas.frame_at(0.17);

    EXPECT_NE(mounted.canvas.pixels(), partway);
    EXPECT_FALSE(mounted.canvas.root().busy()) << "the spin ran past 0.16 seconds";
}

TEST_F(Painted, GlyphButtonSpinRepaintsOnlyTheButton) {
    Mounted mounted;

    mounted.button->spin(45);
    mounted.button->spun(true);

    for (const double now : {0.0, 0.04, 0.08, 0.2}) {
        EXPECT_LE(mounted.canvas.frame_at(now), widgets::area(BOX)) << "at " << now;
    }

    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, GlyphButtonTonesFollowAPaletteChange) {
    Mounted mounted;

    ttk::Theme::set_mode(ttk::Theme::Mode::Light);
    mounted.canvas.frame();

    EXPECT_GT(mounted.inked(widgets::palette().muted), 0U);
    EXPECT_EQ(mounted.inked(ttk::Theme::palette(ttk::Theme::Mode::Dark).muted), 0U);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, GlyphButtonHoverInTheLightPalette) {
    ttk::Theme::set_mode(ttk::Theme::Mode::Light);

    Mounted mounted;

    mounted.lit();

    EXPECT_GT(mounted.inked(widgets::palette().text), 0U);
    EXPECT_TRUE(widgets::near(mounted.canvas.at(WASH_X, WASH_Y),
                              widgets::over(widgets::palette().background, widgets::palette().hover)));
}

class Drawn : public Painted, public ::testing::WithParamInterface<int> {};

TEST_P(Drawn, InkStaysInsideTheBody) {
    const auto glyph = static_cast<Glyph>(GetParam());
    const Mounted mounted(glyph, BLRect{40, 40, 120, 80});

    EXPECT_GT(widgets::ink(mounted.canvas, BLRectI{84, 64, 32, 32}, widgets::background()).pixels, 0U)
            << "glyph " << GetParam() << " drew nothing";
    EXPECT_EQ(widgets::outside(mounted.canvas, BLRectI{84, 64, 32, 32}), 0U)
            << "glyph " << GetParam() << " drew outside its body";
}

INSTANTIATE_TEST_SUITE_P(GlyphButton,
                         Drawn,
                         ::testing::Range(static_cast<int>(Glyph::Close), static_cast<int>(Glyph::Count)));
}
