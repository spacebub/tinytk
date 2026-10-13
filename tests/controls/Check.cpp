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
#include <vector>

#include <gtest/gtest.h>

#include "Mount.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Event.h"
#include "ttk/toolkit/controls/Check.h"

namespace {
using controls::key;
namespace Code = ttk::Code;

constexpr BLRect BOX{20, 20, 30, 30};
constexpr double SIDE = 18.0;

class CheckTest : public controls::Painted {
protected:
    tests::Canvas canvas;
    std::vector<bool> toggled;
    ttk::Check *check = nullptr;

    ttk::Check *make() {
        check = controls::mount(
                canvas, std::make_unique<ttk::Check>([this](const bool value) { toggled.push_back(value); }), BOX);

        return check;
    }

    void click_box() { controls::click(canvas.root(), BOX.x + (BOX.w / 2.0), BOX.y + (BOX.h / 2.0)); }

    void settle() {
        for (int step = 0; step < 10; ++step) {
            canvas.frame_at(canvas.root().now() + 0.05);
        }
    }

    // Inside the square, left of the check mark and clear of the rim.
    [[nodiscard]] std::uint32_t body() const {
        const double left = BOX.x + ((BOX.w - SIDE) / 2.0);

        return canvas.at(static_cast<int>(left + 3.0), static_cast<int>(BOX.y + (BOX.h / 2.0)));
    }
};

TEST_F(CheckTest, StartsUnchecked) {
    make();

    EXPECT_FALSE(check->checked());
}

TEST_F(CheckTest, NaturalSizeIs18WhateverIsFixed) {
    ttk::Check made(nullptr);

    made.fixedWidth = 50.0;
    made.fixedHeight = 50.0;

    EXPECT_DOUBLE_EQ(made.natural_width(tests::fonts()), SIDE);
    EXPECT_DOUBLE_EQ(made.natural_height(tests::fonts(), 100.0), SIDE);
}

TEST_F(CheckTest, AClickReportsTheOppositeStateWithoutTakingIt) {
    make();
    click_box();

    EXPECT_EQ(toggled, (std::vector<bool>{true}));
    EXPECT_FALSE(check->checked());

    check->set_checked(true);
    click_box();

    EXPECT_EQ(toggled, (std::vector<bool>{true, false}));
}

TEST_F(CheckTest, SetCheckedDoesNotReport) {
    make();
    check->set_checked(true);

    EXPECT_TRUE(check->checked());
    EXPECT_TRUE(toggled.empty());

    check->set_checked(false);

    EXPECT_FALSE(check->checked());
}

TEST_F(CheckTest, SetCheckedToTheSameStateDoesNothing) {
    make();
    canvas.root().advance(0.0);
    check->set_checked(false);

    EXPECT_FALSE(canvas.root().busy());
}

TEST_F(CheckTest, ReleasingOutsideTheBoxDoesNothing) {
    make();
    controls::sweep(canvas.root(), BOX.x + 15.0, BOX.y + 15.0, BOX.x + 200.0, BOX.y + 15.0);

    EXPECT_TRUE(toggled.empty());
}

TEST_F(CheckTest, ADisabledBoxIgnoresClicks) {
    make();
    check->set_enabled(false);

    EXPECT_FALSE(check->press(ttk::Pointer{.x = BOX.x + 15.0, .y = BOX.y + 15.0}));

    click_box();
    check->release(ttk::Pointer{.x = BOX.x + 15.0, .y = BOX.y + 15.0});

    EXPECT_TRUE(toggled.empty());
}

TEST_F(CheckTest, ANullCallbackIsAllowed) {
    controls::mount(canvas, std::make_unique<ttk::Check>(nullptr), BOX);
    click_box();

    SUCCEED();
}

TEST_F(CheckTest, SpaceAndReturnThroughTheRootToggle) {
    make();
    canvas.root().focus(check);

    EXPECT_TRUE(canvas.root().key(key(Code::Space)));
    EXPECT_TRUE(canvas.root().key(key(Code::Return)));
    EXPECT_EQ(toggled, (std::vector<bool>{true, true}));
}

TEST_F(CheckTest, AHeldKeyTogglesOnce) {
    make();
    canvas.root().focus(check);
    canvas.root().key(key(Code::Space));
    canvas.root().key(ttk::Key{.code = Code::Space, .repeat = true});

    EXPECT_EQ(toggled, (std::vector<bool>{true}));
}

TEST_F(CheckTest, TheBoxHandlesNoKeysItself) {
    make();

    EXPECT_FALSE(check->key(key(Code::Space)));
    EXPECT_FALSE(check->key(key(Code::Return)));
    EXPECT_TRUE(toggled.empty());
}

TEST_F(CheckTest, ADisabledBoxIsNoTabStop) {
    make();
    check->set_enabled(false);

    EXPECT_FALSE(check->takes_focus());
}

TEST_F(CheckTest, TheBoxIsFieldColouredOffAndAccentOn) {
    make();
    canvas.full();

    EXPECT_EQ(body(), ttk::Theme::palette().field.value);

    check->set_checked(true);
    settle();
    canvas.full();

    EXPECT_EQ(body(), ttk::Theme::palette().accent.value);
}

TEST_F(CheckTest, TheCheckMarkIsDrawnOnlyWhenOn) {
    make();
    canvas.full();

    const BLRectI middle{static_cast<int>(BOX.x + 10.0), static_cast<int>(BOX.y + 10.0), 10, 10};

    EXPECT_EQ(controls::count_near(canvas, middle, ttk::Theme::palette().field, 0), 100) << "a mark while off";

    check->set_checked(true);
    settle();
    canvas.full();

    EXPECT_LT(controls::count_near(canvas, middle, ttk::Theme::palette().accent, 0), 100) << "no mark while on";
    EXPECT_GT(controls::count_near(canvas, middle, ttk::Theme::palette().accentText, 40), 0);
}

TEST_F(CheckTest, TheBoxIsCentredInItsBox) {
    make();
    check->set_checked(true);
    settle();
    canvas.full();

    const int left = static_cast<int>(BOX.x + ((BOX.w - SIDE) / 2.0));
    const int middle = static_cast<int>(BOX.y + (BOX.h / 2.0));

    EXPECT_EQ(canvas.at(left - 1, middle), ttk::Theme::palette().background.value);
    EXPECT_EQ(canvas.at(left + static_cast<int>(SIDE), middle), ttk::Theme::palette().background.value);
    EXPECT_NE(canvas.at(left + 1, middle), ttk::Theme::palette().background.value);
}

TEST_F(CheckTest, TurningOnAnimatesAndEnds) {
    make();
    canvas.root().advance(0.0);
    check->set_checked(true);

    EXPECT_TRUE(canvas.root().busy());

    canvas.full();

    EXPECT_EQ(body(), ttk::Theme::palette().field.value) << "the box jumped instead of animating";

    settle();

    EXPECT_FALSE(canvas.root().busy());
}

TEST_F(CheckTest, HoverStrengthensTheRim) {
    make();
    canvas.full();

    const int left = static_cast<int>(BOX.x + ((BOX.w - SIDE) / 2.0));
    const int middle = static_cast<int>(BOX.y + (BOX.h / 2.0));
    const std::uint32_t resting = canvas.at(left, middle);

    canvas.root().motion(BOX.x + 15.0, BOX.y + 15.0);
    canvas.frame();

    EXPECT_TRUE(check->hovered());
    EXPECT_NE(canvas.at(left, middle), resting);

    canvas.root().leave();
    canvas.frame();

    EXPECT_FALSE(check->hovered());
    EXPECT_EQ(canvas.at(left, middle), resting);
}

TEST_F(CheckTest, LeaveClearsThePress) {
    make();
    canvas.root().motion(BOX.x + 15.0, BOX.y + 15.0);
    canvas.root().press(ttk::Pointer{.x = BOX.x + 15.0, .y = BOX.y + 15.0});

    EXPECT_TRUE(check->pressed());

    check->leave();

    EXPECT_FALSE(check->pressed());
    EXPECT_FALSE(check->hovered());
}

TEST_F(CheckTest, PartialRepaintsMatchAFullOne) {
    make();
    canvas.root().motion(BOX.x + 15.0, BOX.y + 15.0);
    canvas.frame();
    check->set_checked(true);
    canvas.frame_at(0.05);
    canvas.frame_at(0.3);

    EXPECT_TRUE(canvas.matches_full());
}
}
