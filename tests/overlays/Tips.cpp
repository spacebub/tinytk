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
#include <vector>

#include <gtest/gtest.h>

#include "Overlays.h"
#include "support/Canvas.h"
#include "ttk/toolkit/overlays/Tips.h"

namespace {
using overlays::Taker;

constexpr double DELAY = 0.45;
constexpr double FADE = 0.11;
constexpr double GAP = 6.0;
constexpr double INSET = 4.0;
constexpr double ROOM = 320.0;

constexpr BLRect OVER{150.0, 150.0, 100.0, 30.0};
constexpr const char *SAID = "Saves the file";
constexpr const char *LONG = "A hint long enough that it cannot fit on one line of three hundred and twenty "
                             "pixels, so it has to wrap onto a second line and maybe a third";

class Tip : public overlays::Painted {
protected:
    tests::Canvas canvas;
    ttk::Tips *tips = nullptr;
    std::vector<std::uint32_t> blank;

    void SetUp() override {
        overlays::Painted::SetUp();

        if (IsSkipped()) {
            return;
        }

        canvas.frame();
        tips = canvas.root().layer(ttk::Root::TIPS)->append(std::make_unique<ttk::Tips>());
        canvas.root().relayout();
        canvas.frame();
        blank = canvas.pixels();
    }

    void at(const double now) { canvas.frame_at(now); }

    void point(const std::string &text,
               const double now,
               const double x = 200.0,
               const BLRect &over = OVER,
               const double y = 160.0) {
        canvas.root().set_now(now);
        tips->point(text, over, x, y, now);
        at(now);
    }

    void raise(const std::string &text, const double start = 0.0, const double x = 200.0, const BLRect &over = OVER) {
        point(text, start, x, over);
        at(start + DELAY);
        at(start + DELAY + FADE + 0.05);
    }

    [[nodiscard]] BLRectI ink() const { return overlays::changed(blank, canvas.pixels(), canvas.width()); }

    [[nodiscard]] bool clear() const { return canvas.pixels() == blank; }
};

TEST_F(Tip, NothingIsDrawnBeforeTheDelay) {
    point(SAID, 0.0);
    at(0.2);
    at(DELAY - 0.01);

    EXPECT_TRUE(clear()) << "the tip showed before its delay";
}

TEST_F(Tip, TipAppearsOnceTheDelayHasPassed) {
    raise(SAID);

    EXPECT_FALSE(clear());
}

TEST_F(Tip, TipFadesInRatherThanAppearingAtOnce) {
    point(SAID, 0.0);
    at(DELAY);

    EXPECT_TRUE(clear()) << "the first frame of the fade is already opaque";

    at(DELAY + (FADE / 3.0));

    const std::vector<std::uint32_t> part = canvas.pixels();

    at(DELAY + FADE + 0.05);

    EXPECT_NE(part, blank);
    EXPECT_NE(part, canvas.pixels()) << "the tip did not fade in";
}

TEST_F(Tip, WaitingTipSleepsUntilTheDelayEnds) {
    point(SAID, 1.0);
    at(1.1);

    EXPECT_FALSE(canvas.root().busy()) << "the window ran frames while only waiting";
    EXPECT_NEAR(canvas.root().waking(), 1.0 + DELAY, 1e-9);
}

TEST_F(Tip, FadeKeepsTheTipLiveThenItSleeps) {
    point(SAID, 0.0);
    at(DELAY);

    EXPECT_TRUE(canvas.root().busy());

    at(DELAY + FADE + 0.05);

    EXPECT_FALSE(canvas.root().busy());
    EXPECT_LT(canvas.root().waking(), 0.0) << "a tip that is up has nothing left to wait for";
}

TEST_F(Tip, TipSitsAboveTheWidgetCentredOnThePointer) {
    raise(SAID);

    const BLRectI box = ink();

    EXPECT_NEAR(box.x + (box.w / 2.0), 200.0, 1.5);
    EXPECT_NEAR(box.y + box.h, OVER.y - GAP, 1.5);
}

TEST_F(Tip, TipGoesBelowTheWidgetWithoutRoomAbove) {
    constexpr BLRect high{150.0, 10.0, 100.0, 30.0};

    raise(SAID, 0.0, 200.0, high);

    EXPECT_NEAR(ink().y, high.y + high.h + GAP, 1.5);
}

TEST_F(Tip, TipIsKeptInsideTheLeftEdge) {
    raise(SAID, 0.0, 0.0);

    EXPECT_NEAR(ink().x, INSET, 1.5);
}

TEST_F(Tip, TipIsKeptInsideTheRightEdge) {
    raise(SAID, 0.0, canvas.width());

    const BLRectI box = ink();

    EXPECT_NEAR(box.x + box.w, canvas.width() - INSET, 1.5);
}

TEST_F(Tip, PointerHeightDoesNotMoveTheTip) {
    point(SAID, 0.0, 200.0, OVER, 0.0);
    at(DELAY);
    at(1.0);

    const std::vector<std::uint32_t> top = canvas.pixels();

    point("", 1.0);
    at(2.0);
    point(SAID, 3.0, 200.0, OVER, 239.0);
    at(3.0 + DELAY);
    at(4.0);

    EXPECT_EQ(canvas.pixels(), top);
}

TEST_F(Tip, ShortTextIsNotStretchedToTheWrapWidth) {
    raise("Hi");

    EXPECT_LT(ink().w, 100);
}

TEST_F(Tip, LongTextWrapsAtThreeHundredTwentyPixels) {
    raise(SAID);

    const int line = ink().h;

    point("", 1.0);
    at(2.0);
    raise(LONG, 3.0);

    const BLRectI box = ink();

    EXPECT_LE(box.w, static_cast<int>(ROOM + 20.0 + 2.0));
    EXPECT_GE(box.w, static_cast<int>(ROOM + 20.0 - 2.0));
    EXPECT_GT(box.h, line + 10) << "the long tip did not wrap onto more lines";
}

TEST_F(Tip, RepeatingTheTextKeepsTheFirstDeadline) {
    point(SAID, 0.0);
    point(SAID, 0.3);
    at(DELAY);
    at(DELAY + FADE + 0.05);

    EXPECT_FALSE(clear()) << "reporting the same text again restarted the wait";
}

TEST_F(Tip, WaitingTipFollowsThePointer) {
    point(SAID, 0.0, 100.0);
    point(SAID, 0.3, 300.0);
    at(DELAY);
    at(1.0);

    const BLRectI box = ink();

    EXPECT_NEAR(box.x + (box.w / 2.0), 300.0, 1.5);
}

TEST_F(Tip, TipStaysWhereItAppeared) {
    raise(SAID, 0.0, 200.0);

    const std::vector<std::uint32_t> shown = canvas.pixels();

    point(SAID, 1.0, 300.0);
    at(1.5);

    EXPECT_EQ(canvas.pixels(), shown);
}

TEST_F(Tip, DifferentTextRestartsTheWait) {
    point("first", 0.0);
    point("second", 0.3);
    at(DELAY + 0.05);

    EXPECT_TRUE(clear()) << "the wait was not counted from the new text";

    at(0.3 + DELAY);
    at(1.0);

    EXPECT_FALSE(clear());
}

TEST_F(Tip, NextTextReplacesAShownTipAtOnce) {
    raise(SAID);

    const std::vector<std::uint32_t> first = canvas.pixels();
    constexpr BLRect other{20.0, 120.0, 60.0, 30.0};

    point("Another control entirely", 1.0, 50.0, other);

    EXPECT_NE(canvas.pixels(), first) << "the next tip waited for the delay again";

    const BLRectI box = ink();

    EXPECT_NEAR(box.y + box.h, other.y - GAP, 1.5);
    EXPECT_LT(box.x, 100) << "the old tip was left on screen";
}

TEST_F(Tip, EmptyTextFadesTheTipOut) {
    raise(SAID);
    point("", 1.0);
    at(1.0 + (FADE / 3.0));

    EXPECT_FALSE(clear()) << "the tip vanished instead of fading";

    at(1.0 + FADE + 0.05);
    at(1.5);

    EXPECT_TRUE(clear());
    EXPECT_FALSE(canvas.root().busy());
}

TEST_F(Tip, EmptyTextBeforeTheDelayCancelsTheTip) {
    point(SAID, 0.0);
    point("", 0.2);
    at(DELAY);
    at(1.0);

    EXPECT_TRUE(clear());
}

TEST_F(Tip, AfterFadingOutTheNextTipWaitsAgain) {
    raise(SAID);
    point("", 1.0);
    at(1.5);
    point(SAID, 2.0);
    at(2.2);

    EXPECT_TRUE(clear()) << "a tip after the last one faded skipped the wait";

    at(2.0 + DELAY);
    at(3.0);

    EXPECT_FALSE(clear());
}

TEST_F(Tip, PointerPassesThroughTheTip) {
    auto *page = static_cast<Taker *>(canvas.root().content()->add(std::make_unique<Taker>()));

    canvas.root().relayout();
    canvas.frame();
    blank = canvas.pixels();
    raise(SAID);

    const BLRectI box = ink();
    const double x = box.x + (box.w / 2.0);
    const double y = box.y + (box.h / 2.0);

    EXPECT_EQ(tips->at(x, y), nullptr);

    overlays::click(canvas.root(), x, y);

    EXPECT_EQ(canvas.root().hovered(), page);
    EXPECT_EQ(page->presses, 1);
}

TEST_F(Tip, PaintMatchesAFullRepaintThroughTheFades) {
    point(SAID, 0.0);

    for (const double now : {DELAY, DELAY + 0.03, DELAY + 0.07, 1.0}) {
        at(now);

        ASSERT_TRUE(canvas.matches_full()) << "fade in partial/full mismatch at " << now;
    }

    point("Another", 1.2, 80.0);

    EXPECT_TRUE(canvas.matches_full()) << "replaced tip partial/full mismatch";

    point("", 1.3);

    for (const double now : {1.33, 1.37, 1.5}) {
        at(now);

        ASSERT_TRUE(canvas.matches_full()) << "fade out partial/full mismatch at " << now;
    }
}
}
