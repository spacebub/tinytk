// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cmath>
#include <cstdint>
#include <memory>
#include <numbers>
#include <vector>

#include <gtest/gtest.h>

#include "Screen.h"
#include "support/Canvas.h"
#include "ttk/draw/Glyphs.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Painter.h"
#include "ttk/toolkit/controls/Spinner.h"

namespace {
constexpr BLRect BOX{40, 40, 24, 24};
constexpr BLRgba32 RED{0xffff0000};

// A point on the circle the arc runs along, `turn` of the way round clockwise from the top.
BLPoint on_ring(const BLRect &box, const double turn) {
    const double side = std::min(box.w, box.h);
    const double width = std::max(1.5, side * 0.14);
    const double radius = (side - width) / 2.0;
    const double angle = (turn * 2.0 * std::numbers::pi) - (std::numbers::pi / 2.0);

    return BLPoint{box.x + (box.w / 2.0) + (radius * std::cos(angle)),
                   box.y + (box.h / 2.0) + (radius * std::sin(angle))};
}

std::uint32_t at(const tests::Canvas &canvas, const BLPoint point) {
    return canvas.at(static_cast<int>(std::floor(point.x)), static_cast<int>(std::floor(point.y)));
}

std::vector<std::uint32_t> square(const tests::Canvas &canvas) {
    std::vector<std::uint32_t> out;

    for (int y = static_cast<int>(BOX.y); y < static_cast<int>(BOX.y + BOX.h); ++y) {
        for (int x = static_cast<int>(BOX.x); x < static_cast<int>(BOX.x + BOX.w); ++x) {
            out.push_back(canvas.at(x, y));
        }
    }

    return out;
}

class SpinnerTest : public widgets::Painted {
protected:
    tests::Canvas canvas;

    // Live for the next frame or scheduled for a later one.
    [[nodiscard]] bool asking() { return canvas.root().busy() || canvas.root().waking() >= 0.0; }

    ttk::Spinner *make() {
        canvas.root().set_now(0.0);

        return widgets::mount(canvas, std::make_unique<ttk::Spinner>(), BOX);
    }
};

TEST(Spinner, NaturalSizeIsTheGlyphSquare) {
    ttk::Spinner spinner(2.0F);

    EXPECT_DOUBLE_EQ(spinner.natural_width(tests::fonts()), ttk::Glyphs::span(2.0F));
    EXPECT_DOUBLE_EQ(spinner.natural_height(tests::fonts(), 100.0), ttk::Glyphs::span(2.0F));

    spinner.fixedWidth = 30.0;

    EXPECT_DOUBLE_EQ(spinner.natural_width(tests::fonts()), 30.0);
}

TEST(Spinner, PhaseGoesRoundOncePerPeriodInSteps) {
    EXPECT_DOUBLE_EQ(ttk::Spinner::phase_at(0.0), 0.0);
    EXPECT_NEAR(ttk::Spinner::phase_at(ttk::Spinner::PERIOD / 2.0), 0.5, 1.0 / ttk::Spinner::STEPS);
    EXPECT_NEAR(ttk::Spinner::phase_at(ttk::Spinner::PERIOD * 3.0), 0.0, 1e-9);
    EXPECT_DOUBLE_EQ(ttk::Spinner::phase_at(0.004), ttk::Spinner::phase_at(0.0)) << "a phase moved inside a step";
}

TEST(Spinner, StepAfterIsWhereThePhaseNextChanges) {
    for (double now = 0.0; now < 3.0; now += 0.0137) {
        const double next = ttk::Spinner::step_after(now);

        EXPECT_GT(next, now);
        EXPECT_LE(next - now, (1.0 / ttk::Spinner::STEPS) + 1e-9);
        EXPECT_NE(ttk::Spinner::phase_at(next), ttk::Spinner::phase_at(now)) << "at " << now;
    }
}

TEST_F(SpinnerTest, RenderDrawsTheArcInTheToneOverAFaintRing) {
    tests::Canvas plain;
    BLContext &context = plain.surface().context();
    const ttk::Painter painter(context, tests::fonts(), BLRectI{0, 0, plain.width(), plain.height()});

    context.fill_all(BLRgba32(0xff000000));
    ttk::Spinner::render(painter, BOX, 0.0, RED);

    const std::uint32_t arc = at(plain, on_ring(BOX, 0.12));
    const std::uint32_t ring = at(plain, on_ring(BOX, 0.62));

    EXPECT_GT(widgets::channel(arc, 16), 200) << "no arc a little way round from the top";
    EXPECT_GT(widgets::channel(ring, 16), 10) << "no ring opposite the arc";
    EXPECT_LT(widgets::channel(ring, 16), 120) << "the ring is not faint";
}

TEST_F(SpinnerTest, RenderTurnsWithThePhase) {
    tests::Canvas plain;
    BLContext &context = plain.surface().context();
    const ttk::Painter painter(context, tests::fonts(), BLRectI{0, 0, plain.width(), plain.height()});

    context.fill_all(BLRgba32(0xff000000));
    ttk::Spinner::render(painter, BOX, 0.5, RED);

    EXPECT_GT(widgets::channel(at(plain, on_ring(BOX, 0.62)), 16), 200) << "the arc did not turn half way";
    EXPECT_LT(widgets::channel(at(plain, on_ring(BOX, 0.12)), 16), 120);
}

TEST_F(SpinnerTest, PaintsInTheAccentByDefault) {
    make();

    EXPECT_TRUE(widgets::near(at(canvas, on_ring(BOX, 0.12)), widgets::palette().accent.value, 24));
}

TEST_F(SpinnerTest, ToneOverridesTheAccent) {
    ttk::Spinner *spinner = make();

    spinner->tone(ttk::Theme::Tone(RED));
    canvas.frame();

    EXPECT_TRUE(widgets::near(at(canvas, on_ring(BOX, 0.12)), RED.value, 24));
}

TEST_F(SpinnerTest, TurnsAStepAtATimeWhileShown) {
    make();

    const std::vector<std::uint32_t> first = square(canvas);

    ASSERT_TRUE(canvas.root().busy()) << "a painted spinner did not start";

    canvas.frame_at(0.0);

    EXPECT_FALSE(canvas.root().busy()) << "a spinner kept the frame loop running between steps";
    EXPECT_DOUBLE_EQ(canvas.root().waking(), ttk::Spinner::step_after(0.0));

    canvas.frame_at(ttk::Spinner::PERIOD / 4.0);

    EXPECT_NE(square(canvas), first) << "the spinner did not turn";
    EXPECT_GT(canvas.root().waking(), ttk::Spinner::PERIOD / 4.0);
}

TEST_F(SpinnerTest, StopsOnceHiddenAndStartsWhenShownAgain) {
    ttk::Spinner *spinner = make();

    canvas.frame_at(0.1);
    spinner->set_visible(false);
    canvas.frame_at(0.2);
    canvas.frame_at(0.3);

    EXPECT_FALSE(asking()) << "a hidden spinner still asks for frames";

    spinner->set_visible(true);
    canvas.frame_at(0.4);

    EXPECT_TRUE(asking()) << "a spinner shown again did not start";
}

TEST_F(SpinnerTest, StopsWhileDisabledAndPaintsFaded) {
    ttk::Spinner *spinner = make();

    canvas.frame_at(0.1);
    spinner->set_enabled(false);
    canvas.frame_at(0.2);
    canvas.frame_at(0.3);

    EXPECT_FALSE(asking());

    const std::uint32_t still = at(canvas, on_ring(BOX, 0.12 + ttk::Spinner::phase_at(0.1)));

    EXPECT_FALSE(widgets::near(still, widgets::palette().accent.value, 24)) << "a disabled spinner is not faded";
    EXPECT_FALSE(widgets::near(still, widgets::background(), 4)) << "a disabled spinner is not drawn";

    spinner->set_enabled(true);
    canvas.frame_at(0.4);

    EXPECT_TRUE(asking());
}

TEST_F(SpinnerTest, StopsWhenItsRepaintsAreClippedAway) {
    make();

    canvas.frame_at(0.1);
    canvas.root().layer(ttk::Root::POPUPS)->children().front()->place(BLRect{-100, -100, 24, 24}, tests::fonts());
    canvas.frame_at(0.2);
    canvas.frame_at(0.3);
    canvas.frame_at(0.4);

    EXPECT_FALSE(asking()) << "a spinner off the window still asks for frames";
}
}
