// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Mount.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Event.h"
#include "ttk/toolkit/controls/Toggle.h"

namespace {
using controls::key;
namespace Code = ttk::Code;

constexpr BLRect BOX{20, 20, 360, 24};
constexpr double TRACK = 38.0;

double label_width(const std::string &text) {
    ttk::Typeface &type = tests::fonts();

    return type.width(type.at(400, ttk::Theme::fontBody), text);
}

class ToggleTest : public controls::Painted {
protected:
    tests::Canvas canvas;
    std::vector<bool> toggled;
    ttk::Toggle *toggle = nullptr;

    std::unique_ptr<ttk::Toggle> build(std::string text = "Fullscreen") {
        return std::make_unique<ttk::Toggle>(std::move(text), [this](const bool value) { toggled.push_back(value); });
    }

    ttk::Toggle *place(std::unique_ptr<ttk::Toggle> made) {
        toggle = controls::mount(canvas, std::move(made), BOX);

        return toggle;
    }

    ttk::Toggle *make(std::string text = "Fullscreen") { return place(build(std::move(text))); }

    void click_track() { controls::click(canvas.root(), BOX.x + (TRACK / 2.0), BOX.y + (BOX.h / 2.0)); }

    void settle() {
        for (int step = 0; step < 10; ++step) {
            canvas.frame_at(canvas.root().now() + 0.05);
        }
    }

    // A point of the track clear of the knob at either end.
    [[nodiscard]] std::uint32_t track() const {
        return canvas.at(static_cast<int>(BOX.x + (TRACK / 2.0)), static_cast<int>(BOX.y + 2.0 + 1.0));
    }
};

TEST_F(ToggleTest, StartsUnchecked) {
    make();

    EXPECT_FALSE(toggle->checked);
}

TEST_F(ToggleTest, AClickReportsTheOppositeStateWithoutTakingIt) {
    make();
    click_track();

    EXPECT_EQ(toggled, (std::vector<bool>{true}));
    EXPECT_FALSE(toggle->checked);

    toggle->set_checked(true);
    click_track();

    EXPECT_EQ(toggled, (std::vector<bool>{true, false}));
}

TEST_F(ToggleTest, AClickOnTheLabelToggles) {
    make();
    controls::click(canvas.root(), BOX.x + TRACK + 10.0 + (label_width("Fullscreen") / 2.0), BOX.y + 12.0);

    EXPECT_EQ(toggled, (std::vector<bool>{true}));
}

TEST_F(ToggleTest, AClickPastTheLabelDoesNothing) {
    make();
    controls::click(canvas.root(), BOX.x + TRACK + 10.0 + label_width("Fullscreen") + 20.0, BOX.y + 12.0);

    EXPECT_TRUE(toggled.empty());
}

TEST_F(ToggleTest, ReleasingOffTheSwitchDoesNothing) {
    make();
    controls::sweep(canvas.root(), BOX.x + 10.0, BOX.y + 12.0, BOX.x + 300.0, BOX.y + 12.0);

    EXPECT_TRUE(toggled.empty());
}

TEST_F(ToggleTest, SetCheckedDoesNotReport) {
    make();
    toggle->set_checked(true);

    EXPECT_TRUE(toggle->checked);
    EXPECT_TRUE(toggled.empty());
}

TEST_F(ToggleTest, SetCheckedToTheSameStateDoesNothing) {
    make();
    canvas.root().advance(0.0);
    toggle->set_checked(false);

    EXPECT_FALSE(canvas.root().busy());
}

TEST_F(ToggleTest, WritingCheckedDirectlyMakesTheNextSetCheckedANoOp) {
    make();
    canvas.root().advance(0.0);
    toggle->checked = true;
    toggle->set_checked(true);

    EXPECT_FALSE(canvas.root().busy()) << "set_checked started a slide for a state already written";

    canvas.full();

    EXPECT_EQ(track(), ttk::Theme::palette().sunken.value) << "the knob moved on a direct write";
}

TEST_F(ToggleTest, ReturnAndSpaceReportTheOppositeState) {
    make();

    EXPECT_TRUE(toggle->key(key(Code::Return)));
    EXPECT_TRUE(toggle->key(key(Code::Space)));
    EXPECT_EQ(toggled, (std::vector<bool>{true, true}));
}

TEST_F(ToggleTest, KeysThroughTheRootToggleOnce) {
    make();
    canvas.root().focus(toggle);

    EXPECT_TRUE(canvas.root().key(key(Code::Space)));
    EXPECT_EQ(toggled, (std::vector<bool>{true})) << "the root's activation fallback toggled as well";
}

TEST_F(ToggleTest, OtherKeysAreNotConsumed) {
    make();

    EXPECT_FALSE(toggle->key(key(Code::Left)));
    EXPECT_FALSE(toggle->key(key(Code::Tab)));
    EXPECT_FALSE(toggle->key(key('t')));
    EXPECT_TRUE(toggled.empty());
}

TEST_F(ToggleTest, ADisabledSwitchConsumesKeysButDoesNotReport) {
    make();
    toggle->set_enabled(false);

    EXPECT_TRUE(toggle->key(key(Code::Return)));
    EXPECT_TRUE(toggle->key(key(Code::Space)));
    EXPECT_TRUE(toggled.empty());
}

TEST_F(ToggleTest, ADisabledSwitchIgnoresClicks) {
    make();
    toggle->set_enabled(false);

    EXPECT_FALSE(toggle->press(ttk::Pointer{.x = BOX.x + 5.0, .y = BOX.y + 5.0}));

    click_track();

    EXPECT_TRUE(toggled.empty());
}

TEST_F(ToggleTest, ANullCallbackIsAllowed) {
    place(std::make_unique<ttk::Toggle>("Quiet", nullptr));
    click_track();

    EXPECT_TRUE(toggle->key(key(Code::Return)));
}

TEST_F(ToggleTest, AtAnswersOnTheTrackAndTheLabelOnly) {
    make();

    const double reach = TRACK + 10.0 + label_width("Fullscreen");

    EXPECT_EQ(toggle->at(BOX.x + 1.0, BOX.y + 1.0), toggle);
    EXPECT_EQ(toggle->at(BOX.x + reach - 1.0, BOX.y + 12.0), toggle);
    EXPECT_EQ(toggle->at(BOX.x + reach + 1.0, BOX.y + 12.0), nullptr);
    EXPECT_EQ(toggle->at(BOX.x + 1.0, BOX.y + BOX.h + 1.0), nullptr);
}

TEST_F(ToggleTest, AtAnswersNothingWhenDisabledOrHidden) {
    make();
    toggle->set_enabled(false);

    EXPECT_EQ(toggle->at(BOX.x + 1.0, BOX.y + 1.0), nullptr);

    toggle->set_enabled(true);
    toggle->set_visible(false);

    EXPECT_EQ(toggle->at(BOX.x + 1.0, BOX.y + 1.0), nullptr);
}

TEST_F(ToggleTest, WithoutALabelOnlyTheTrackAnswers) {
    make("");

    EXPECT_EQ(toggle->at(BOX.x + TRACK - 1.0, BOX.y + 5.0), toggle);
    EXPECT_EQ(toggle->at(BOX.x + TRACK + 1.0, BOX.y + 5.0), nullptr);
}

TEST_F(ToggleTest, SetTextMovesTheReachAtTheNextLayout) {
    make("A");

    const double longer = TRACK + 10.0 + label_width("A much longer label");

    toggle->set_text("A much longer label");

    EXPECT_EQ(toggle->at(BOX.x + longer - 2.0, BOX.y + 5.0), nullptr) << "the reach moved before a layout";

    canvas.root().relayout();
    canvas.frame();

    EXPECT_EQ(toggle->at(BOX.x + longer - 2.0, BOX.y + 5.0), toggle);
}

TEST_F(ToggleTest, SetTextRepaintsOnlyOnAChange) {
    make("Same");
    canvas.frame();
    toggle->set_text("Same");

    EXPECT_FALSE(canvas.root().dirty());

    toggle->set_text("Other");

    EXPECT_TRUE(canvas.root().dirty());
}

TEST_F(ToggleTest, NaturalWidthIsTheTrackAndTheLabel) {
    auto made = build("Label");

    EXPECT_NEAR(made->natural_width(tests::fonts()), TRACK + 10.0 + label_width("Label"), 1e-3);

    made->fixedWidth = 200.0;

    EXPECT_DOUBLE_EQ(made->natural_width(tests::fonts()), 200.0);
}

TEST_F(ToggleTest, NaturalWidthWithoutALabelIsTheTrack) {
    auto made = build("");

    EXPECT_DOUBLE_EQ(made->natural_width(tests::fonts()), TRACK);
}

TEST_F(ToggleTest, NaturalHeightIs24UnlessFixed) {
    auto made = build();

    EXPECT_DOUBLE_EQ(made->natural_height(tests::fonts(), 100.0), 24.0);

    made->fixedHeight = 30.0;

    EXPECT_DOUBLE_EQ(made->natural_height(tests::fonts(), 100.0), 30.0);
}

TEST_F(ToggleTest, TheTrackIsSunkenOffAndAccentOn) {
    make();
    canvas.full();

    EXPECT_EQ(track(), ttk::Theme::palette().sunken.value);

    toggle->set_checked(true);
    settle();
    canvas.full();

    EXPECT_EQ(track(), ttk::Theme::palette().accent.value);
}

TEST_F(ToggleTest, TheKnobSlidesToTheRight) {
    make();

    const auto knob = [&] {
        return canvas.at(static_cast<int>(BOX.x + TRACK - 11.0), static_cast<int>(BOX.y + (BOX.h / 2.0)));
    };

    canvas.full();

    const std::uint32_t off = knob();

    toggle->set_checked(true);
    settle();
    canvas.full();

    EXPECT_EQ(off, ttk::Theme::palette().sunken.value);
    EXPECT_EQ(knob(), ttk::Theme::palette().accentText.value);
}

TEST_F(ToggleTest, SetCheckedSlidesWhileAttached) {
    make();
    canvas.root().advance(0.0);
    toggle->set_checked(true);

    EXPECT_TRUE(canvas.root().busy());

    canvas.full();

    EXPECT_EQ(track(), ttk::Theme::palette().sunken.value) << "the knob jumped while attached";

    settle();

    EXPECT_FALSE(canvas.root().busy());
}

TEST_F(ToggleTest, SetCheckedJumpsWhileUnattached) {
    auto made = build();

    made->set_checked(true);
    place(std::move(made));
    canvas.full();

    EXPECT_EQ(track(), ttk::Theme::palette().accent.value);
}

TEST_F(ToggleTest, ADisabledSwitchIsDimmed) {
    make();
    canvas.full();

    const std::uint32_t bright = track();

    toggle->set_enabled(false);
    canvas.full();

    EXPECT_NE(track(), bright);
}

TEST_F(ToggleTest, HoverStrengthensTheBorder) {
    make();
    canvas.full();

    const BLRectI rim{static_cast<int>(BOX.x + 10.0), static_cast<int>(BOX.y + 1.0), 20, 1};
    const std::vector<std::uint32_t> resting = controls::pixels_in(canvas, rim);

    canvas.root().motion(BOX.x + 5.0, BOX.y + 12.0);
    canvas.frame();

    EXPECT_TRUE(toggle->hovered());
    EXPECT_NE(controls::pixels_in(canvas, rim), resting);

    canvas.root().motion(390.0, 230.0);
    canvas.frame();

    EXPECT_FALSE(toggle->hovered());
    EXPECT_EQ(controls::pixels_in(canvas, rim), resting);
}

TEST_F(ToggleTest, PartialRepaintsMatchAFullOne) {
    make();
    canvas.root().motion(BOX.x + 5.0, BOX.y + 12.0);
    canvas.frame();
    toggle->set_checked(true);
    canvas.frame_at(0.05);
    canvas.frame_at(0.3);

    EXPECT_TRUE(canvas.matches_full());
}
}
