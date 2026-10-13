// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cmath>
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
#include "ttk/toolkit/controls/Pill.h"

namespace {
using widgets::Painted;
using Kind = ttk::Pill::Kind;

constexpr BLRect BOX{40, 40, 120, 26};
constexpr BLRectI PIXELS{40, 40, 120, 26};

// Inside the left cap, above the dot and the text.
constexpr int WASH_X = 53;
constexpr int WASH_Y = 44;

ttk::Typeface &type() {
    return tests::fonts();
}

double semibold(const std::string &text) {
    return type().width(type().at(600, ttk::Theme::fontSmall), text);
}

std::uint32_t ink_of(const BLRgba32 tone) {
    return widgets::palette().dark ? tone.value : ttk::Theme::darker(tone, 0.35).value;
}

struct Mounted {
    tests::Canvas canvas;
    ttk::Pill *pill = nullptr;

    explicit Mounted(std::unique_ptr<ttk::Pill> made, const BLRect &box = BOX) {
        pill = widgets::mount(canvas, std::move(made), box);
    }

    explicit Mounted(const std::string &text = "", const Kind kind = Kind::None, const BLRect &box = BOX)
        : Mounted(make(text, kind), box) {}

    static std::unique_ptr<ttk::Pill> make(const std::string &text, const Kind kind) {
        auto made = std::make_unique<ttk::Pill>(text);

        made->kind(kind);

        return made;
    }

    [[nodiscard]] std::uint32_t wash() const { return canvas.at(WASH_X, WASH_Y); }

    // Centre of the dot of a pill whose text is `text`.
    [[nodiscard]] std::uint32_t dot(const std::string &text = "") const {
        const double content = semibold(text) + 14.0;
        const double x = BOX.x + ((BOX.w - content) / 2.0) + 4.0;

        return canvas.at(static_cast<int>(std::floor(x)), static_cast<int>(BOX.y + 13));
    }
};

struct Scheme {
    Kind kind;
    BLRgba32 ttk::Theme::Palette::*tone;
    BLRgba32 ttk::Theme::Palette::*wash;
    const char *name;
};

class Schemes : public Painted, public ::testing::WithParamInterface<Scheme> {};

TEST_P(Schemes, ToneAndWashAreThePaletteSlots) {
    EXPECT_EQ(ttk::Pill::tone_of(GetParam().kind).value, (widgets::palette().*GetParam().tone).value);
    EXPECT_EQ(ttk::Pill::wash_of(GetParam().kind).value, (widgets::palette().*GetParam().wash).value);

    ttk::Theme::set_mode(ttk::Theme::Mode::Light);

    EXPECT_EQ(ttk::Pill::tone_of(GetParam().kind).value, (widgets::palette().*GetParam().tone).value);
    EXPECT_EQ(ttk::Pill::wash_of(GetParam().kind).value, (widgets::palette().*GetParam().wash).value);
}

TEST_P(Schemes, PaintsTheWashAndTheDotInTheTone) {
    const Mounted mounted("", GetParam().kind);

    EXPECT_EQ(mounted.wash(), (widgets::palette().*GetParam().wash).value);
    EXPECT_EQ(mounted.dot(), ink_of(widgets::palette().*GetParam().tone));
}

TEST_P(Schemes, DarkensTheToneInTheLightPalette) {
    ttk::Theme::set_mode(ttk::Theme::Mode::Light);

    const Mounted mounted("", GetParam().kind);
    const BLRgba32 tone = widgets::palette().*GetParam().tone;

    EXPECT_EQ(mounted.wash(), (widgets::palette().*GetParam().wash).value);
    EXPECT_EQ(mounted.dot(), ttk::Theme::darker(tone, 0.35).value);
    EXPECT_NE(mounted.dot(), tone.value);
}

TEST_P(Schemes, RestylesOnAPaletteChange) {
    Mounted mounted("", GetParam().kind);

    ttk::Theme::set_mode(ttk::Theme::Mode::Light);
    mounted.canvas.frame();

    EXPECT_EQ(mounted.wash(), (widgets::palette().*GetParam().wash).value);
    EXPECT_EQ(mounted.dot(), ttk::Theme::darker(widgets::palette().*GetParam().tone, 0.35).value);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

INSTANTIATE_TEST_SUITE_P(
        Pill,
        Schemes,
        ::testing::Values(
                Scheme{Kind::None, &ttk::Theme::Palette::accent, &ttk::Theme::Palette::accentSoft, "None"},
                Scheme{Kind::Muted, &ttk::Theme::Palette::muted, &ttk::Theme::Palette::mutedSoft, "Muted"},
                Scheme{Kind::Success, &ttk::Theme::Palette::success, &ttk::Theme::Palette::successSoft, "Success"},
                Scheme{Kind::Warning, &ttk::Theme::Palette::warning, &ttk::Theme::Palette::warningSoft, "Warning"},
                Scheme{Kind::Danger, &ttk::Theme::Palette::danger, &ttk::Theme::Palette::dangerSoft, "Danger"}),
        [](const ::testing::TestParamInfo<Scheme> &each) { return std::string(each.param.name); });

TEST(Pill, ChainedSettersReturnThePill) {
    ttk::Pill pill("Ready");

    EXPECT_EQ(pill.kind(Kind::Success), &pill);
    EXPECT_EQ(pill.dot(false), &pill);
    EXPECT_EQ(pill.glyph(ttk::Glyphs::Glyph::Check), &pill);
    EXPECT_EQ(pill.tones(&ttk::Theme::Palette::danger, &ttk::Theme::Palette::dangerSoft), &pill);
}

TEST(Pill, NaturalHeightIsAlways26) {
    ttk::Pill pill("Ready");

    EXPECT_DOUBLE_EQ(pill.natural_height(tests::fonts(), 500), 26.0);

    pill.fixedHeight = 50;

    EXPECT_DOUBLE_EQ(pill.natural_height(tests::fonts(), 500), 26.0) << "fixedHeight is ignored";
}

TEST(Pill, FixedWidthOverridesTheNaturalWidth) {
    ttk::Pill pill("Ready");

    pill.fixedWidth = 71;

    EXPECT_DOUBLE_EQ(pill.natural_width(tests::fonts()), 71);
}

TEST(Pill, DoesNotTakeThePointer) {
    ttk::Pill pill("Ready");

    pill.place(BOX, tests::fonts());

    EXPECT_EQ(pill.at(BOX.x + 10, BOX.y + 10), nullptr);
}

TEST_F(Painted, PillNaturalWidthIsTextDotAndPadding) {
    ttk::Pill pill("Ready");

    EXPECT_DOUBLE_EQ(pill.natural_width(type()), semibold("Ready") + 14.0 + 22.0);
}

TEST_F(Painted, PillWithoutADotDropsItsRoom) {
    ttk::Pill pill("Ready");

    pill.dot(false);

    EXPECT_DOUBLE_EQ(pill.natural_width(type()), semibold("Ready") + 22.0);
}

TEST_F(Painted, PillEmptyTextIsDotAndPadding) {
    ttk::Pill pill;

    EXPECT_DOUBLE_EQ(pill.natural_width(type()), 36.0);

    pill.dot(false);

    EXPECT_DOUBLE_EQ(pill.natural_width(type()), 22.0);
}

TEST_F(Painted, PillGlyphReplacesTheTextInTheWidth) {
    ttk::Pill pill("A long text that the glyph replaces");

    pill.glyph(ttk::Glyphs::Glyph::Check);

    EXPECT_NEAR(pill.natural_width(type()), (12.0 * 1.4) + 14.0 + 22.0, 1e-9);

    pill.glyph(ttk::Glyphs::Glyph::Empty);

    EXPECT_DOUBLE_EQ(pill.natural_width(type()), semibold("A long text that the glyph replaces") + 36.0);
}

TEST_F(Painted, PillUtf8TextIsMeasured) {
    ttk::Pill pill("Größe й ✓");

    EXPECT_DOUBLE_EQ(pill.natural_width(type()), semibold("Größe й ✓") + 36.0);
}

TEST_F(Painted, PillDefaultsToAccentWithADot) {
    const Mounted mounted(std::make_unique<ttk::Pill>());

    EXPECT_EQ(mounted.wash(), widgets::palette().accentSoft.value);
    EXPECT_EQ(mounted.dot(), widgets::palette().accent.value);
}

TEST_F(Painted, PillBorderIsTheInkAtAThird) {
    const Mounted mounted("", Kind::Success);
    const std::uint32_t top = mounted.canvas.at(static_cast<int>(BOX.x + (BOX.w / 2)), static_cast<int>(BOX.y));

    EXPECT_TRUE(widgets::near(
            top, widgets::over(widgets::palette().successSoft, ttk::Theme::alpha(widgets::palette().success, 0.3))))
            << std::hex << top;
}

TEST_F(Painted, PillIsRoundedToHalfItsHeight) {
    const Mounted mounted;

    EXPECT_EQ(mounted.canvas.at(PIXELS.x, PIXELS.y), widgets::background());
    EXPECT_EQ(mounted.canvas.at(PIXELS.x + 1, PIXELS.y + 1), widgets::background());
    EXPECT_EQ(mounted.canvas.at(PIXELS.x + PIXELS.w - 1, PIXELS.y + PIXELS.h - 1), widgets::background());
    EXPECT_EQ(widgets::outside(mounted.canvas, PIXELS), 0U);
}

TEST_F(Painted, PillWithoutADotDrawsOnlyTheWash) {
    auto made = std::make_unique<ttk::Pill>();

    made->dot(false);

    const Mounted mounted(std::move(made));

    EXPECT_EQ(widgets::count(mounted.canvas, BLRectI{56, 44, 88, 18}, widgets::palette().accentSoft), 88U * 18U);
}

TEST_F(Painted, PillTextIsCentredAfterTheDot) {
    const Mounted mounted("Ready");
    const double content = semibold("Ready") + 14.0;
    const double start = BOX.x + ((BOX.w - content) / 2.0);
    const widgets::Ink text = widgets::ink(mounted.canvas,
                                           BLRectI{static_cast<int>(start) + 12, 42, 136 - static_cast<int>(start), 22},
                                           widgets::palette().accentSoft.value);

    EXPECT_EQ(mounted.dot("Ready"), widgets::palette().accent.value);
    EXPECT_GT(text.pixels, 0U);
    EXPECT_NEAR(text.left, start + 14.0, 3.0);
    EXPECT_NEAR(text.right, start + content, 3.0);
}

TEST_F(Painted, PillGlyphIsDrawnInPlaceOfTheText) {
    auto withText = std::make_unique<ttk::Pill>("Some text");
    auto without = std::make_unique<ttk::Pill>();

    withText->glyph(ttk::Glyphs::Glyph::Check);
    without->glyph(ttk::Glyphs::Glyph::Check);

    const Mounted one(std::move(withText));
    const Mounted two(std::move(without));
    const Mounted plain;

    EXPECT_EQ(one.canvas.pixels(), two.canvas.pixels()) << "the text was drawn beside the glyph";
    EXPECT_NE(two.canvas.pixels(), plain.canvas.pixels());
}

TEST_F(Painted, PillTonesOverrideTheKind) {
    auto made = std::make_unique<ttk::Pill>();

    made->kind(Kind::Danger)->tones(ttk::Theme::Tone(BLRgba32(0xff40c0f0)), ttk::Theme::Tone(BLRgba32(0xff102030)));

    const Mounted mounted(std::move(made));

    EXPECT_EQ(mounted.wash(), 0xff102030U);
    EXPECT_EQ(mounted.dot(), 0xff40c0f0U);
}

TEST_F(Painted, PillSlotTonesFollowAPaletteChange) {
    auto made = std::make_unique<ttk::Pill>();

    made->tones(&ttk::Theme::Palette::warning, &ttk::Theme::Palette::mutedSoft);

    Mounted mounted(std::move(made));

    EXPECT_EQ(mounted.wash(), widgets::palette().mutedSoft.value);

    ttk::Theme::set_mode(ttk::Theme::Mode::Light);
    mounted.canvas.frame();

    EXPECT_EQ(mounted.wash(), widgets::palette().mutedSoft.value);
    EXPECT_EQ(mounted.dot(), ttk::Theme::darker(widgets::palette().warning, 0.35).value);
}

TEST_F(Painted, PillConstructionSettersDoNotDamage) {
    Mounted mounted;

    mounted.pill->kind(Kind::Danger)->dot(false)->glyph(ttk::Glyphs::Glyph::Check);

    EXPECT_FALSE(mounted.canvas.root().dirty());
}

TEST_F(Painted, PillSetTextRepaintsOnlyThePill) {
    Mounted mounted("Ready");
    const std::vector<std::uint32_t> before = mounted.canvas.pixels();

    mounted.pill->set_text("Busy");

    const std::size_t painted = mounted.canvas.frame();

    EXPECT_GT(painted, 0U);
    EXPECT_LE(painted, widgets::area(BOX));
    EXPECT_NE(mounted.canvas.pixels(), before);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, PillSetTextKeepsTheWidth) {
    Mounted mounted("Ready");

    mounted.pill->set_text("Busy");

    EXPECT_FALSE(mounted.canvas.root().settle()) << "set_text asked for a layout";
    EXPECT_DOUBLE_EQ(mounted.pill->box().w, BOX.w);
}

TEST_F(Painted, PillTextTooLongForTheBoxIsElidedInside) {
    Mounted mounted("Ready");

    mounted.pill->set_text("A much longer text than the pill holds");
    mounted.canvas.frame();

    EXPECT_EQ(widgets::outside(mounted.canvas, PIXELS), 0U);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, PillSetTextToTheSameTextDamagesNothing) {
    Mounted mounted("Ready");

    mounted.pill->set_text("Ready");

    EXPECT_FALSE(mounted.canvas.root().dirty());
}

TEST_F(Painted, PillInATinyBoxPaintsOnlyThere) {
    constexpr BLRect tiny{50, 50, 8, 6};
    auto made = std::make_unique<ttk::Pill>();

    made->dot(false);

    Mounted mounted(std::move(made), tiny);

    EXPECT_EQ(widgets::outside(mounted.canvas, widgets::pixels_of(tiny)), 0U);
    EXPECT_TRUE(mounted.canvas.matches_full());
}
}
