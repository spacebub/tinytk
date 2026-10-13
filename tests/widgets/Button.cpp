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
#include "ttk/toolkit/controls/Button.h"

namespace {
using widgets::Painted;

constexpr BLRect BOX{40, 40, 180, 42};
constexpr double MIDDLE_X = 130;
constexpr double MIDDLE_Y = 61;
constexpr double SETTLED = 0.5;

// Left of the label and clear of the border and corners.
constexpr int FILL_X = 46;
constexpr int FILL_Y = 61;

constexpr BLRectI LABEL{100, 48, 60, 26};

double semibold(const float size, const std::string &text) {
    return tests::fonts().width(tests::fonts().at(ttk::Typeface::semibold, size), text);
}

struct Mounted {
    tests::Canvas canvas;
    ttk::Button *button = nullptr;
    int clicks = 0;

    explicit Mounted(const std::string &text = "Launch",
                     const ttk::Button::Kind kind = ttk::Button::Kind::Default,
                     const BLRect &box = BOX) {
        auto made = std::make_unique<ttk::Button>(text, [this] { ++clicks; });

        made->kind(kind);
        button = widgets::mount(canvas, std::move(made), box);
    }

    [[nodiscard]] std::uint32_t fill() const { return canvas.at(FILL_X, FILL_Y); }
    [[nodiscard]] std::uint32_t edge() const { return canvas.at(static_cast<int>(BOX.x), FILL_Y); }
    [[nodiscard]] std::uint32_t top() const { return canvas.at(static_cast<int>(MIDDLE_X), static_cast<int>(BOX.y)); }
};

TEST(Button, NaturalHeightIsTheControlHeight) {
    ttk::Button button("Launch", {});

    EXPECT_DOUBLE_EQ(button.natural_height(tests::fonts(), 500), ttk::Theme::control);
    EXPECT_DOUBLE_EQ(button.natural_height(tests::fonts(), 0), ttk::Theme::control);
}

TEST(Button, CompactNaturalHeightIsTheSmallControlHeight) {
    ttk::Button button("Launch", {});

    button.compact();

    EXPECT_DOUBLE_EQ(button.natural_height(tests::fonts(), 500), ttk::Theme::controlSmall);

    button.compact(false);

    EXPECT_DOUBLE_EQ(button.natural_height(tests::fonts(), 500), ttk::Theme::control);
}

TEST(Button, FixedSizesOverrideTheNaturalOnes) {
    ttk::Button button("Launch", {});

    button.fixedWidth = 77;
    button.fixedHeight = 19;

    EXPECT_DOUBLE_EQ(button.natural_width(tests::fonts()), 77);
    EXPECT_DOUBLE_EQ(button.natural_height(tests::fonts(), 500), 19);

    button.compact();

    EXPECT_DOUBLE_EQ(button.natural_width(tests::fonts()), 77);
    EXPECT_DOUBLE_EQ(button.natural_height(tests::fonts(), 500), 19);
}

TEST(Button, ChainedSettersReturnTheButton) {
    ttk::Button button("Launch", {});

    EXPECT_EQ(button.kind(ttk::Button::Kind::Primary), &button);
    EXPECT_EQ(button.glyph(ttk::Glyphs::Glyph::Plus), &button);
    EXPECT_EQ(button.compact(), &button);
    EXPECT_EQ(button.busy(true), &button);
    EXPECT_EQ(button.tooltip("Starts it"), &button);
}

TEST(Button, TooltipSetsTheHint) {
    ttk::Button button("Launch", {});

    button.tooltip("Starts the game");

    EXPECT_EQ(button.hint, "Starts the game");
}

TEST(Button, ShowsThePointerCursorAndTakesFocus) {
    const ttk::Button button("Launch", {});

    EXPECT_EQ(button.cursor, ttk::Cursor::Pointer);
    EXPECT_TRUE(button.takes_focus());
    EXPECT_FALSE(button.paints_focus()) << "a button relies on the root's focus ring";
}

TEST(Button, KindsCompareByTheirDeriveFunction) {
    constexpr auto derive = [](const ttk::Theme::Palette &palette) {
        return ttk::Button::Look{
                .ground = palette.danger,
                .lit = palette.danger,
                .down = palette.danger,
                .edge = palette.danger,
                .edgeLit = palette.danger,
                .ink = palette.text,
                .dots = palette.text,
        };
    };

    EXPECT_EQ(ttk::Button::Kind::Default, ttk::Button::Kind::Default);
    EXPECT_NE(ttk::Button::Kind::Default, ttk::Button::Kind::Primary);
    EXPECT_NE(ttk::Button::Kind::Success, ttk::Button::Kind::Danger);
    EXPECT_NE(ttk::Button::Kind::Danger, ttk::Button::Kind::Ghost);
    EXPECT_EQ(ttk::Button::Kind(+derive), ttk::Button::Kind(+derive));
    EXPECT_EQ(ttk::Button::Kind(+derive).look(ttk::Theme::palette()).ground.value, ttk::Theme::palette().danger.value);
}

TEST_F(Painted, ButtonShortLabelIsWidenedToTheButtonWidth) {
    ttk::Button button("OK", {});

    EXPECT_DOUBLE_EQ(button.natural_width(tests::fonts()), ttk::Theme::buttonWidth);
}

TEST_F(Painted, ButtonEmptyLabelIsTheButtonWidth) {
    ttk::Button button("", {});

    EXPECT_DOUBLE_EQ(button.natural_width(tests::fonts()), ttk::Theme::buttonWidth);

    button.compact();

    EXPECT_DOUBLE_EQ(button.natural_width(tests::fonts()), 26.0) << "an empty compact button is its padding";
}

TEST_F(Painted, ButtonLongLabelIsTextPlusPadding) {
    const std::string text = "Download every missing file from the mirror";
    ttk::Button button(text, {});

    EXPECT_DOUBLE_EQ(button.natural_width(tests::fonts()), semibold(ttk::Theme::fontBody, text) + 38.0);
}

TEST_F(Painted, ButtonGlyphAddsItsMarkAndGap) {
    const std::string text = "Download every missing file from the mirror";
    ttk::Button button(text, {});
    const double bare = button.natural_width(tests::fonts());

    button.glyph(ttk::Glyphs::Glyph::Download);

    EXPECT_NEAR(button.natural_width(tests::fonts()) - bare, (12.0 * 1.2) + 7.0, 1e-9);

    button.glyph(ttk::Glyphs::Glyph::Empty);

    EXPECT_DOUBLE_EQ(button.natural_width(tests::fonts()), bare) << "Empty must show no glyph";
}

TEST_F(Painted, ButtonCompactHasNoFloorAndASmallerFont) {
    ttk::Button button("OK", {});

    button.compact();

    EXPECT_DOUBLE_EQ(button.natural_width(tests::fonts()), semibold(ttk::Theme::fontSmall, "OK") + 26.0);
    EXPECT_LT(button.natural_width(tests::fonts()), ttk::Theme::buttonWidth);

    button.glyph(ttk::Glyphs::Glyph::Plus);

    EXPECT_NEAR(button.natural_width(tests::fonts()),
                semibold(ttk::Theme::fontSmall, "OK") + 26.0 + (12.0 * 1.1) + 7.0,
                1e-9);
}

TEST_F(Painted, ButtonUtf8LabelIsMeasured) {
    const std::string text = "Zeitgeist für Größe: ÉÈ й 日本 and more words";
    ttk::Button button(text, {});

    EXPECT_DOUBLE_EQ(button.natural_width(tests::fonts()), semibold(ttk::Theme::fontBody, text) + 38.0);
    EXPECT_GT(button.natural_width(tests::fonts()), ttk::Theme::buttonWidth);
}

struct Resting {
    const ttk::Button::Kind *kind;
    BLRgba32 ttk::Theme::Palette::*fill;
    const char *name;
};

class Kinds : public Painted, public ::testing::WithParamInterface<Resting> {};

TEST_P(Kinds, RestsOnItsFill) {
    const Mounted mounted("Launch", *GetParam().kind);

    EXPECT_EQ(mounted.fill(), (widgets::palette().*GetParam().fill).value) << GetParam().name;
}

TEST_P(Kinds, LabelInkStandsOffTheFill) {
    const Mounted mounted("Launch", *GetParam().kind);

    EXPECT_GT(widgets::contrast(mounted.canvas, LABEL, mounted.fill()), 1000U) << GetParam().name;
    EXPECT_EQ(widgets::outside(mounted.canvas, widgets::pixels_of(BOX)), 0U) << GetParam().name;
}

TEST_P(Kinds, FollowsAPaletteChange) {
    Mounted mounted("Launch", *GetParam().kind);

    ttk::Theme::set_mode(ttk::Theme::Mode::Light);
    mounted.canvas.frame();

    EXPECT_EQ(mounted.fill(), (ttk::Theme::palette(ttk::Theme::Mode::Light).*GetParam().fill).value) << GetParam().name;
    EXPECT_TRUE(mounted.canvas.matches_full());
}

INSTANTIATE_TEST_SUITE_P(
        Button,
        Kinds,
        ::testing::Values(Resting{&ttk::Button::Kind::Default, &ttk::Theme::Palette::raised, "Default"},
                          Resting{&ttk::Button::Kind::Primary, &ttk::Theme::Palette::accent, "Primary"},
                          Resting{&ttk::Button::Kind::Success, &ttk::Theme::Palette::success, "Success"},
                          Resting{&ttk::Button::Kind::Danger, &ttk::Theme::Palette::raised, "Danger"},
                          Resting{&ttk::Button::Kind::Ghost, &ttk::Theme::Palette::background, "Ghost"}),
        [](const ::testing::TestParamInfo<Resting> &each) { return std::string(each.param.name); });

TEST_F(Painted, ButtonDefaultBorderIsTheStrongBorder) {
    const Mounted mounted;

    EXPECT_EQ(mounted.edge(), widgets::palette().borderStrong.value);
    EXPECT_EQ(mounted.top(), widgets::palette().borderStrong.value);
}

TEST_F(Painted, ButtonDangerBorderIsHalfTheDangerColour) {
    const Mounted mounted("Delete", ttk::Button::Kind::Danger);

    EXPECT_TRUE(
            widgets::near(mounted.edge(),
                          widgets::over(widgets::palette().raised, ttk::Theme::alpha(widgets::palette().danger, 0.5))))
            << std::hex << mounted.edge();
}

TEST_F(Painted, ButtonFilledKindsHaveNoBorder) {
    for (const ttk::Button::Kind *kind : {&ttk::Button::Kind::Primary, &ttk::Button::Kind::Success}) {
        const Mounted mounted("Go", *kind);

        EXPECT_EQ(mounted.edge(), mounted.fill());
    }
}

TEST_F(Painted, ButtonCustomKindPaintsItsLook) {
    constexpr auto derive = [](const ttk::Theme::Palette & /*palette*/) {
        return ttk::Button::Look{
                .ground = BLRgba32(0xff123456),
                .lit = BLRgba32(0xff654321),
                .down = BLRgba32(0xff0000ff),
                .edge = BLRgba32(0xff00ff00),
                .edgeLit = BLRgba32(0xff00ff00),
                .ink = BLRgba32(0xffffffff),
                .dots = BLRgba32(0xffffffff),
        };
    };

    Mounted mounted("Custom", ttk::Button::Kind(+derive));

    EXPECT_EQ(mounted.fill(), 0xff123456U);
    EXPECT_EQ(mounted.edge(), 0xff00ff00U);

    widgets::hover(mounted.canvas, MIDDLE_X, MIDDLE_Y);
    mounted.canvas.frame_at(SETTLED);

    EXPECT_EQ(mounted.fill(), 0xff654321U);

    mounted.canvas.root().press(ttk::Pointer{.x = MIDDLE_X, .y = MIDDLE_Y});
    mounted.canvas.frame_at(SETTLED * 2);

    EXPECT_EQ(mounted.fill(), 0xff0000ffU);
}

TEST_F(Painted, ButtonNullKindPaintsAsDefault) {
    const Mounted plain;
    const Mounted null("Launch", ttk::Button::Kind(nullptr));

    EXPECT_EQ(null.canvas.pixels(), plain.canvas.pixels());
}

TEST_F(Painted, ButtonHoverLightsTheFillAndTheBorder) {
    Mounted mounted;

    widgets::hover(mounted.canvas, MIDDLE_X, MIDDLE_Y);
    mounted.canvas.frame_at(SETTLED);

    EXPECT_TRUE(mounted.button->hovered());
    EXPECT_EQ(mounted.fill(), ttk::Theme::mix(widgets::palette().raised, widgets::palette().hover, 1.0).value);
    EXPECT_EQ(mounted.edge(), widgets::palette().accent.value);
}

TEST_F(Painted, ButtonHoverFadesInOverTime) {
    Mounted mounted;
    const std::uint32_t rest = mounted.fill();
    const std::uint32_t lit = ttk::Theme::mix(widgets::palette().raised, widgets::palette().hover, 1.0).value;

    widgets::hover(mounted.canvas, MIDDLE_X, MIDDLE_Y);
    mounted.canvas.frame_at(0.02);

    EXPECT_NE(mounted.fill(), lit) << "the hover jumped instead of fading";
    EXPECT_TRUE(mounted.canvas.root().busy());

    mounted.canvas.frame_at(SETTLED);

    EXPECT_NE(rest, lit);
    EXPECT_EQ(mounted.fill(), lit);
}

TEST_F(Painted, ButtonHoverFadesOutOnLeave) {
    Mounted mounted;
    const std::uint32_t rest = mounted.fill();

    widgets::hover(mounted.canvas, MIDDLE_X, MIDDLE_Y);
    mounted.canvas.frame_at(SETTLED);
    widgets::hover(mounted.canvas, 5, 5);
    mounted.canvas.frame_at(SETTLED * 2);

    EXPECT_FALSE(mounted.button->hovered());
    EXPECT_EQ(mounted.fill(), rest);
    EXPECT_EQ(mounted.edge(), widgets::palette().borderStrong.value);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, ButtonWindowLeaveFadesTheHover) {
    Mounted mounted;
    const std::uint32_t rest = mounted.fill();

    widgets::hover(mounted.canvas, MIDDLE_X, MIDDLE_Y);
    mounted.canvas.frame_at(SETTLED);
    mounted.canvas.root().leave();
    mounted.canvas.frame_at(SETTLED * 2);

    EXPECT_EQ(mounted.fill(), rest);
}

TEST_F(Painted, ButtonHoverRepaintsOnlyTheButton) {
    Mounted mounted;

    widgets::hover(mounted.canvas, MIDDLE_X, MIDDLE_Y);

    for (const double now : {0.02, 0.05, 0.08, SETTLED}) {
        const std::size_t painted = mounted.canvas.frame_at(now);

        EXPECT_GT(painted, 0U) << "at " << now;
        EXPECT_LE(painted, widgets::area(BOX)) << "hover repainted outside the button at " << now;
    }

    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, ButtonLitBoxIsItsBox) {
    const Mounted mounted;

    EXPECT_DOUBLE_EQ(mounted.button->lit_box().x, BOX.x);
    EXPECT_DOUBLE_EQ(mounted.button->lit_box().y, BOX.y);
    EXPECT_DOUBLE_EQ(mounted.button->lit_box().w, BOX.w);
    EXPECT_DOUBLE_EQ(mounted.button->lit_box().h, BOX.h);
}

TEST_F(Painted, ButtonSettlesAfterTheHover) {
    Mounted mounted;

    widgets::hover(mounted.canvas, MIDDLE_X, MIDDLE_Y);
    mounted.canvas.frame_at(SETTLED);
    mounted.canvas.frame_at(SETTLED + 0.01);

    EXPECT_FALSE(mounted.canvas.root().busy()) << "a settled hover kept the frame loop running";
    EXPECT_EQ(mounted.canvas.frame_at(SETTLED + 0.02), 0U);
}

TEST_F(Painted, ButtonPressFillsWithTheDownColour) {
    Mounted mounted;

    widgets::hover(mounted.canvas, MIDDLE_X, MIDDLE_Y);
    mounted.canvas.root().press(ttk::Pointer{.x = MIDDLE_X, .y = MIDDLE_Y});
    mounted.canvas.frame_at(SETTLED);

    EXPECT_TRUE(mounted.button->pressed());
    EXPECT_EQ(mounted.canvas.root().grabbed(), mounted.button);
    EXPECT_EQ(mounted.fill(), widgets::palette().sunken.value);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, ButtonPressShrinksTheBody) {
    Mounted mounted;

    ASSERT_NE(mounted.edge(), widgets::background());

    mounted.canvas.root().press(ttk::Pointer{.x = MIDDLE_X, .y = MIDDLE_Y});
    mounted.canvas.frame_at(SETTLED);

    EXPECT_EQ(mounted.edge(), widgets::background()) << "the held body did not draw in from its edge";

    mounted.canvas.root().release(ttk::Pointer{.x = MIDDLE_X, .y = MIDDLE_Y});
    mounted.canvas.frame_at(SETTLED * 2);

    EXPECT_NE(mounted.edge(), widgets::background()) << "the body did not grow back after release";
}

TEST_F(Painted, ButtonPressRepaintsOnlyTheButton) {
    Mounted mounted;

    widgets::hover(mounted.canvas, MIDDLE_X, MIDDLE_Y);
    mounted.canvas.frame_at(SETTLED);
    mounted.canvas.root().press(ttk::Pointer{.x = MIDDLE_X, .y = MIDDLE_Y});

    for (const double now : {SETTLED + 0.03, SETTLED + 0.2}) {
        EXPECT_LE(mounted.canvas.frame_at(now), widgets::area(BOX));
    }

    mounted.canvas.root().release(ttk::Pointer{.x = MIDDLE_X, .y = MIDDLE_Y});

    for (const double now : {SETTLED + 0.25, SETTLED + 0.5}) {
        EXPECT_LE(mounted.canvas.frame_at(now), widgets::area(BOX));
    }

    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, ButtonReleaseInsideClicks) {
    Mounted mounted;

    widgets::click(mounted.canvas, MIDDLE_X, MIDDLE_Y);

    EXPECT_EQ(mounted.clicks, 1);
    EXPECT_FALSE(mounted.button->pressed());
    EXPECT_EQ(mounted.canvas.root().grabbed(), nullptr);

    widgets::click(mounted.canvas, BOX.x + 1, BOX.y + 1);

    EXPECT_EQ(mounted.clicks, 2) << "a click on the corner pixel missed";
}

TEST_F(Painted, ButtonPressAloneDoesNotClick) {
    Mounted mounted;

    mounted.canvas.root().press(ttk::Pointer{.x = MIDDLE_X, .y = MIDDLE_Y});

    EXPECT_EQ(mounted.clicks, 0);
}

TEST_F(Painted, ButtonReleaseOutsideDoesNotClick) {
    Mounted mounted;

    mounted.canvas.root().press(ttk::Pointer{.x = MIDDLE_X, .y = MIDDLE_Y});
    mounted.canvas.root().motion(300, 200);
    mounted.canvas.root().release(ttk::Pointer{.x = 300, .y = 200});

    EXPECT_EQ(mounted.clicks, 0);
    EXPECT_FALSE(mounted.button->pressed());
}

TEST_F(Painted, ButtonReleaseJustPastTheEdgeDoesNotClick) {
    Mounted mounted;

    mounted.canvas.root().press(ttk::Pointer{.x = MIDDLE_X, .y = MIDDLE_Y});
    mounted.canvas.root().release(ttk::Pointer{.x = BOX.x + BOX.w, .y = MIDDLE_Y});

    EXPECT_EQ(mounted.clicks, 0) << "the right edge is outside the half open box";
}

TEST_F(Painted, ButtonDragOutAndBackClicks) {
    Mounted mounted;

    mounted.canvas.root().press(ttk::Pointer{.x = MIDDLE_X, .y = MIDDLE_Y});
    mounted.canvas.root().motion(300, 200);
    mounted.canvas.root().motion(MIDDLE_X + 20, MIDDLE_Y);
    mounted.canvas.root().release(ttk::Pointer{.x = MIDDLE_X + 20, .y = MIDDLE_Y});

    EXPECT_EQ(mounted.clicks, 1);
}

TEST_F(Painted, ButtonPressOutsideDoesNotReachIt) {
    Mounted mounted;

    widgets::click(mounted.canvas, 300, 200);

    EXPECT_EQ(mounted.clicks, 0);
    EXPECT_FALSE(mounted.button->hovered());
}

TEST_F(Painted, ButtonEveryClickCalls) {
    Mounted mounted;

    for (int at = 0; at < 5; ++at) {
        widgets::click(mounted.canvas, MIDDLE_X, MIDDLE_Y);
    }

    EXPECT_EQ(mounted.clicks, 5);
}

TEST_F(Painted, ButtonSpaceClicksWhenFocused) {
    Mounted mounted;

    mounted.canvas.root().focus(mounted.button);

    EXPECT_TRUE(mounted.button->focused());
    EXPECT_TRUE(mounted.canvas.root().key(widgets::key(ttk::Code::Space)));
    EXPECT_EQ(mounted.clicks, 1);
}

TEST_F(Painted, ButtonReturnClicksWhenFocused) {
    Mounted mounted;

    mounted.canvas.root().focus(mounted.button);

    EXPECT_TRUE(mounted.canvas.root().key(widgets::key(ttk::Code::Return)));
    EXPECT_EQ(mounted.clicks, 1);
}

TEST_F(Painted, ButtonOtherKeysAreLeftAlone) {
    Mounted mounted;

    mounted.canvas.root().focus(mounted.button);

    for (const int code : {static_cast<int>('a'), ttk::Code::Escape, ttk::Code::Left, ttk::Code::Backspace}) {
        EXPECT_FALSE(mounted.button->key(widgets::key(code))) << code;
        EXPECT_FALSE(mounted.canvas.root().key(widgets::key(code))) << code;
    }

    EXPECT_EQ(mounted.clicks, 0);
}

TEST_F(Painted, ButtonKeysWithoutFocusDoNothing) {
    Mounted mounted;

    EXPECT_FALSE(mounted.canvas.root().key(widgets::key(ttk::Code::Space)));
    EXPECT_FALSE(mounted.canvas.root().key(widgets::key(ttk::Code::Return)));
    EXPECT_EQ(mounted.clicks, 0);
}

TEST_F(Painted, ButtonWithoutCallbackTakesClicksQuietly) {
    tests::Canvas canvas;
    ttk::Button *button = widgets::mount(canvas, std::make_unique<ttk::Button>("Quiet", nullptr), BOX);

    widgets::click(canvas, MIDDLE_X, MIDDLE_Y);
    canvas.root().focus(button);

    EXPECT_TRUE(canvas.root().key(widgets::key(ttk::Code::Space)));
    EXPECT_TRUE(button->key(widgets::key(ttk::Code::Return)));
}

TEST_F(Painted, ButtonDisabledIgnoresThePointer) {
    Mounted mounted;

    mounted.button->set_enabled(false);
    widgets::click(mounted.canvas, MIDDLE_X, MIDDLE_Y);

    EXPECT_EQ(mounted.clicks, 0);
    EXPECT_FALSE(mounted.button->hovered());
    EXPECT_FALSE(mounted.button->pressed());
    EXPECT_EQ(mounted.canvas.root().grabbed(), nullptr);
}

TEST_F(Painted, ButtonDisabledRefusesAPressHandedToIt) {
    const Mounted mounted;

    mounted.button->set_enabled(false);

    EXPECT_FALSE(mounted.button->press(ttk::Pointer{.x = MIDDLE_X, .y = MIDDLE_Y}));

    mounted.button->release(ttk::Pointer{.x = MIDDLE_X, .y = MIDDLE_Y});

    EXPECT_EQ(mounted.clicks, 0);
}

TEST_F(Painted, ButtonDisabledConsumesActivationKeysWithoutClicking) {
    Mounted mounted;

    mounted.button->set_enabled(false);
    mounted.canvas.root().focus(mounted.button);

    EXPECT_TRUE(mounted.button->key(widgets::key(ttk::Code::Space)));
    EXPECT_TRUE(mounted.canvas.root().key(widgets::key(ttk::Code::Return)));
    EXPECT_EQ(mounted.clicks, 0);
}

TEST_F(Painted, ButtonDisabledIsSkippedByTab) {
    Mounted mounted;

    mounted.button->set_enabled(false);

    EXPECT_FALSE(mounted.button->takes_focus());

    mounted.canvas.root().focus_next(false);

    EXPECT_EQ(mounted.canvas.root().focused(), nullptr);
}

TEST_F(Painted, ButtonDisabledFadesItsLabel) {
    Mounted mounted;
    const std::size_t bright = widgets::contrast(mounted.canvas, LABEL, mounted.fill());

    mounted.button->set_enabled(false);
    mounted.canvas.frame();

    const std::size_t faded = widgets::contrast(mounted.canvas, LABEL, mounted.fill());

    EXPECT_GT(faded, 0U) << "the disabled label vanished";
    EXPECT_LT(faded, bright) << "the disabled label was not faded";
    EXPECT_EQ(mounted.fill(), widgets::palette().raised.value) << "only the label fades";
}

TEST_F(Painted, ButtonEnablingRepaintsOnlyTheButton) {
    Mounted mounted;
    const std::vector<std::uint32_t> enabled = mounted.canvas.pixels();

    mounted.button->set_enabled(false);

    const std::size_t painted = mounted.canvas.frame();

    EXPECT_GT(painted, 0U);
    EXPECT_LE(painted, widgets::area(BOX));
    EXPECT_NE(mounted.canvas.pixels(), enabled);

    mounted.button->set_enabled(true);
    mounted.canvas.frame();

    EXPECT_EQ(mounted.canvas.pixels(), enabled);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, ButtonTabFocusDrawsTheRing) {
    Mounted mounted;

    ASSERT_EQ(mounted.top(), widgets::palette().borderStrong.value);

    mounted.canvas.root().focus_next(false);

    const std::size_t painted = mounted.canvas.frame();

    EXPECT_EQ(mounted.canvas.root().focused(), mounted.button);
    EXPECT_EQ(mounted.top(), widgets::palette().accent.value) << "no focus ring along the top edge";
    EXPECT_LE(painted, widgets::area(BOX));
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, ButtonProgrammaticFocusDrawsNoRing) {
    Mounted mounted;

    mounted.canvas.root().focus(mounted.button);
    mounted.canvas.frame();

    EXPECT_EQ(mounted.top(), widgets::palette().borderStrong.value);
}

TEST_F(Painted, ButtonPressHidesTheFocusRing) {
    Mounted mounted;

    mounted.canvas.root().focus_next(false);
    mounted.canvas.frame();
    widgets::click(mounted.canvas, 300, 200);

    const std::size_t painted = mounted.canvas.frame();

    EXPECT_EQ(mounted.top(), widgets::palette().borderStrong.value);
    EXPECT_LE(painted, widgets::area(BOX));
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, ButtonSetTextRepaintsOnlyTheButton) {
    Mounted mounted;
    const std::vector<std::uint32_t> before = mounted.canvas.pixels();

    mounted.button->set_text("Stop");

    EXPECT_TRUE(mounted.canvas.root().dirty());

    const std::size_t painted = mounted.canvas.frame();

    EXPECT_GT(painted, 0U);
    EXPECT_LE(painted, widgets::area(BOX));
    EXPECT_NE(mounted.canvas.pixels(), before);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, ButtonSetTextToTheSameTextDamagesNothing) {
    Mounted mounted;

    mounted.button->set_text("Launch");

    EXPECT_FALSE(mounted.canvas.root().dirty());
}

TEST_F(Painted, ButtonSetTextKeepsTheBox) {
    Mounted mounted;

    mounted.button->set_text("A label much longer than the button it sits on, far too long");

    EXPECT_FALSE(mounted.canvas.root().settle()) << "set_text asked for a new layout";

    EXPECT_DOUBLE_EQ(mounted.button->box().w, BOX.w);
}

TEST_F(Painted, ButtonLabelTooLongForTheBoxIsElidedInside) {
    Mounted mounted;

    mounted.button->set_text("A label much longer than the button it sits on, far too long");
    mounted.canvas.frame();

    constexpr BLRectI inside{44, 46, 172, 30};

    EXPECT_GT(widgets::ink(mounted.canvas, inside, mounted.fill()).pixels, 0U) << "the label vanished";
    EXPECT_EQ(widgets::outside(mounted.canvas, widgets::pixels_of(BOX)), 0U);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, ButtonInATinyBoxPaintsOnlyThere) {
    constexpr BLRect tiny{50, 50, 6, 4};
    Mounted mounted("Launch", ttk::Button::Kind::Default, tiny);

    EXPECT_EQ(widgets::outside(mounted.canvas, widgets::pixels_of(tiny)), 0U);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, ButtonConstructionSettersDoNotDamage) {
    Mounted mounted;

    mounted.button->kind(ttk::Button::Kind::Primary)->glyph(ttk::Glyphs::Glyph::Plus)->compact();

    EXPECT_FALSE(mounted.canvas.root().dirty()) << "a construction setter repainted or asked for a layout";
}

TEST_F(Painted, ButtonGlyphIsDrawnBeforeTheLabel) {
    const Mounted plain;
    tests::Canvas canvas;
    auto made = std::make_unique<ttk::Button>("Launch", nullptr);

    made->glyph(ttk::Glyphs::Glyph::Play);
    widgets::mount(canvas, std::move(made), BOX);

    constexpr BLRectI inside{44, 46, 172, 30};
    const widgets::Ink without = widgets::ink(plain.canvas, inside, plain.fill());
    const widgets::Ink with = widgets::ink(canvas, inside, plain.fill());

    EXPECT_LT(with.left, without.left) << "the glyph did not draw left of where the label was";
    EXPECT_EQ(widgets::outside(canvas, widgets::pixels_of(BOX)), 0U);
}

TEST_F(Painted, ButtonBusyReplacesTheLabelWithDots) {
    Mounted mounted;
    const std::vector<std::uint32_t> idle = mounted.canvas.pixels();

    mounted.button->busy(true);

    const std::size_t painted = mounted.canvas.frame();

    EXPECT_LE(painted, widgets::area(BOX));
    EXPECT_NE(mounted.canvas.pixels(), idle);
    EXPECT_EQ(mounted.canvas.at(130, 61), widgets::palette().accent.value) << "second dot not at full colour";
    EXPECT_EQ(mounted.canvas.at(141, 61), widgets::palette().accent.value) << "third dot not at full colour";
    EXPECT_TRUE(
            widgets::near(mounted.canvas.at(119, 61),
                          widgets::over(widgets::palette().raised, ttk::Theme::alpha(widgets::palette().accent, 0.25))))
            << "first dot not faint";
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, ButtonBusyDotsStepEveryTick) {
    Mounted mounted;

    mounted.button->busy(true);
    mounted.canvas.frame();

    const std::uint32_t full = widgets::palette().accent.value;

    mounted.canvas.frame_at(0.31);

    EXPECT_EQ(mounted.canvas.at(119, 61), full);
    EXPECT_NE(mounted.canvas.at(130, 61), full) << "the faint dot did not move on";
    EXPECT_EQ(mounted.canvas.at(141, 61), full);

    mounted.canvas.frame_at(0.62);

    EXPECT_EQ(mounted.canvas.at(130, 61), full);
    EXPECT_NE(mounted.canvas.at(141, 61), full);

    mounted.canvas.frame_at(0.93);

    EXPECT_NE(mounted.canvas.at(119, 61), full) << "the dot did not wrap round to the first";
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, ButtonBusySleepsBetweenSteps) {
    Mounted mounted;

    mounted.button->busy(true);
    mounted.canvas.frame_at(0.31);

    EXPECT_FALSE(mounted.canvas.root().busy()) << "busy dots kept the frame loop running between steps";
    EXPECT_NEAR(mounted.canvas.root().waking(), 0.61, 1e-9);
}

TEST_F(Painted, ButtonBusyStepRepaintsOnlyTheButton) {
    Mounted mounted;

    mounted.button->busy(true);
    mounted.canvas.frame();

    for (const double now : {0.31, 0.62, 0.93}) {
        const std::size_t painted = mounted.canvas.frame_at(now);

        EXPECT_GT(painted, 0U);
        EXPECT_LE(painted, widgets::area(BOX));
    }

    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, ButtonBusyIgnoresClicksFromPointerAndKeyboard) {
    Mounted mounted;

    mounted.button->busy(true);
    widgets::click(mounted.canvas, MIDDLE_X, MIDDLE_Y);

    EXPECT_EQ(mounted.canvas.root().grabbed(), nullptr);

    mounted.canvas.root().focus(mounted.button);

    EXPECT_TRUE(mounted.canvas.root().key(widgets::key(ttk::Code::Space)));
    EXPECT_TRUE(mounted.button->key(widgets::key(ttk::Code::Return)));
    EXPECT_FALSE(mounted.button->press(ttk::Pointer{.x = MIDDLE_X, .y = MIDDLE_Y}));
    EXPECT_EQ(mounted.clicks, 0);
}

TEST_F(Painted, ButtonNoLongerBusyShowsTheLabelAgain) {
    Mounted mounted;
    const std::vector<std::uint32_t> idle = mounted.canvas.pixels();

    mounted.button->busy(true);
    mounted.canvas.frame();
    mounted.button->busy(false);
    mounted.canvas.frame();

    EXPECT_EQ(mounted.canvas.pixels(), idle);

    widgets::click(mounted.canvas, MIDDLE_X, MIDDLE_Y);

    EXPECT_EQ(mounted.clicks, 1);
}

TEST_F(Painted, ButtonBusyToTheSameStateDamagesNothing) {
    Mounted mounted;

    mounted.button->busy(false);

    EXPECT_FALSE(mounted.canvas.root().dirty());
}

TEST_F(Painted, ButtonInAnEmptyBoxPaintsNothing) {
    Mounted mounted("Launch", ttk::Button::Kind::Default, BLRect{50, 50, 0, 0});

    EXPECT_EQ(widgets::outside(mounted.canvas, BLRectI{0, 0, 0, 0}), 0U);

    widgets::click(mounted.canvas, 50, 50);

    EXPECT_EQ(mounted.clicks, 0);
}

TEST_F(Painted, ButtonCompactPaintsASmallerLabel) {
    const Mounted plain("Launch");
    tests::Canvas canvas;
    auto made = std::make_unique<ttk::Button>("Launch", nullptr);

    made->compact();
    widgets::mount(canvas, std::move(made), BOX);

    constexpr BLRectI inside{44, 44, 172, 34};
    const widgets::Ink large = widgets::ink(plain.canvas, inside, plain.fill());
    const widgets::Ink small = widgets::ink(canvas, inside, plain.fill());

    ASSERT_GT(small.pixels, 0U);
    EXPECT_LT(small.right - small.left, large.right - large.left);
    EXPECT_LE(small.bottom - small.top, large.bottom - large.top);
}

TEST_F(Painted, ButtonAtItsNaturalWidthKeepsALongLabelInside) {
    const std::string text = "Download every missing file from the mirror";
    ttk::Button sizing(text, nullptr);
    const double wide = std::ceil(sizing.natural_width(tests::fonts()));
    const BLRect box{10, 40, wide, 42};
    Mounted mounted(text, ttk::Button::Kind::Default, box);

    EXPECT_EQ(widgets::outside(mounted.canvas, widgets::pixels_of(box)), 0U);
    EXPECT_TRUE(mounted.canvas.matches_full());
}
}
