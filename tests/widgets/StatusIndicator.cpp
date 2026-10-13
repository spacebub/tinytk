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
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Screen.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Painter.h"
#include "ttk/toolkit/controls/StatusIndicator.h"

namespace {
    using Status = ttk::StatusIndicator::Status;

    constexpr BLRect BOX{40, 40, 200, 24};

    // The dot is 3.5 across around [13.5, 12] from the top-left corner.
    constexpr int DOT_X = 53;
    constexpr int DOT_Y = 52;

    constexpr std::uint32_t TILE = 0x9e000000;

    // A frame lands a little after each beat, as a real one does.
    constexpr double LATE = 0.001;

    ttk::Typeface &type() {
        return tests::fonts();
    }

    ttk::StatusIndicator::Tones palette_tones(const ttk::Theme::Palette &palette) {
        return {
                .launching = palette.accentHover,
                .running = palette.success,
                .failing = palette.danger,
                .idle = palette.muted,
        };
    }

    ttk::StatusIndicator::Tones fixed_tones(const ttk::Theme::Palette & /*palette*/) {
        return {
                .launching = BLRgba32(0xff0000ff),
                .running = BLRgba32(0xff00ff00),
                .failing = BLRgba32(0xffff0000),
                .idle = BLRgba32(0xff808080),
        };
    }

    class Painted : public widgets::Painted {
    protected:
        void TearDown() override {
            ttk::StatusIndicator::set_tones(&palette_tones);
            widgets::Painted::TearDown();
        }
    };

    struct Mounted {
        tests::Canvas canvas;
        ttk::StatusIndicator *indicator = nullptr;
        int clicks = 0;

        explicit Mounted(const Status status = Status::Empty, const BLRect &box = BOX) {
            indicator = widgets::mount(canvas, std::make_unique<ttk::StatusIndicator>(), box);
            indicator->set(status);
            canvas.frame();
        }

        [[nodiscard]] std::uint32_t dot() const { return canvas.at(DOT_X, DOT_Y); }

        [[nodiscard]] double wide() const { return ttk::StatusIndicator::width_of(type(), indicator->status()); }
    };

    std::uint32_t tone_of(const Status status) {
        const ttk::Theme::Palette &palette = widgets::palette();

        switch (status) {
            case Status::Launching:
                return palette.accentHover.value;
            case Status::Running:
                return palette.success.value;
            case Status::Stopping:
            case Status::Failed:
                return palette.danger.value;
            case Status::Empty:
            case Status::Closed:
                break;
        }

        return palette.muted.value;
    }

    std::uint32_t faint_dot(const Status status) {
        return widgets::over(widgets::over(widgets::background(), TILE),
                             ttk::Theme::alpha(BLRgba32(tone_of(status)), 0.2).value);
    }

    TEST(StatusIndicator, OnlyLaunchingAndStoppingBeat) {
        EXPECT_TRUE(ttk::StatusIndicator::beats(Status::Launching));
        EXPECT_TRUE(ttk::StatusIndicator::beats(Status::Stopping));
        EXPECT_FALSE(ttk::StatusIndicator::beats(Status::Empty));
        EXPECT_FALSE(ttk::StatusIndicator::beats(Status::Running));
        EXPECT_FALSE(ttk::StatusIndicator::beats(Status::Closed));
        EXPECT_FALSE(ttk::StatusIndicator::beats(Status::Failed));
    }

    TEST(StatusIndicator, SayOfExplainsEveryStatus) {
        EXPECT_EQ(ttk::StatusIndicator::say_of(Status::Launching, ""), "It has been started and is loading.");
        EXPECT_EQ(ttk::StatusIndicator::say_of(Status::Running, ""), "It is up.");
        EXPECT_EQ(ttk::StatusIndicator::say_of(Status::Stopping, ""),
                  "It has been asked to quit. One still loading does not hear until it is up.");
        EXPECT_EQ(ttk::StatusIndicator::say_of(Status::Closed, ""), "It has been closed.");
        EXPECT_EQ(ttk::StatusIndicator::say_of(Status::Failed, ""), "It did not start.");
    }

    TEST(StatusIndicator, SayOfEmptyIsTheClosedSentence) {
        EXPECT_EQ(ttk::StatusIndicator::say_of(Status::Empty, ""), ttk::StatusIndicator::say_of(Status::Closed, ""));
        EXPECT_EQ(ttk::StatusIndicator::say_of(Status::Empty, "why"), ttk::StatusIndicator::say_of(Status::Closed, ""));
    }

    TEST(StatusIndicator, SayOfFailedGivesTheReason) {
        EXPECT_EQ(ttk::StatusIndicator::say_of(Status::Failed, "No IWAD found."), "No IWAD found.");
    }

    TEST(StatusIndicator, SayOfIgnoresTheReasonForOtherStatuses) {
        for (const Status status : {Status::Launching, Status::Running, Status::Stopping, Status::Closed}) {
            EXPECT_EQ(ttk::StatusIndicator::say_of(status, "A reason"), ttk::StatusIndicator::say_of(status, ""))
                    << static_cast<int>(status);
        }
    }

    TEST(StatusIndicator, StartsEmptyWithNoTooltip) {
        const ttk::StatusIndicator indicator;

        EXPECT_EQ(indicator.status(), Status::Empty);
        EXPECT_TRUE(indicator.hint.empty());
        EXPECT_EQ(indicator.cursor, ttk::Cursor::Default);
    }

    TEST(StatusIndicator, NaturalHeightIsTheConstant) {
        ttk::StatusIndicator indicator;

        EXPECT_DOUBLE_EQ(indicator.natural_height(tests::fonts(), 500), ttk::StatusIndicator::HEIGHT);
        EXPECT_DOUBLE_EQ(ttk::StatusIndicator::HEIGHT, 24.0);
    }

    TEST(StatusIndicator, EmptyHasNoWidth) {
        ttk::StatusIndicator indicator;

        EXPECT_DOUBLE_EQ(ttk::StatusIndicator::width_of(tests::fonts(), Status::Empty), 0.0);
        EXPECT_DOUBLE_EQ(indicator.natural_width(tests::fonts()), 0.0);
    }

    TEST(StatusIndicator, FixedWidthOverridesTheNaturalWidth) {
        ttk::StatusIndicator indicator;

        indicator.fixedWidth = 64;

        EXPECT_DOUBLE_EQ(indicator.natural_width(tests::fonts()), 64);
    }

    TEST(StatusIndicator, SetUpdatesTheStatusAndTooltip) {
        ttk::StatusIndicator indicator;

        indicator.set(Status::Running);

        EXPECT_EQ(indicator.status(), Status::Running);
        EXPECT_EQ(indicator.hint, "It is up.");

        indicator.set(Status::Failed, "Missing file.");

        EXPECT_EQ(indicator.hint, "Missing file.");

        indicator.set(Status::Failed, "Another reason.");

        EXPECT_EQ(indicator.hint, "Another reason.") << "a new reason alone did not update the tooltip";

        indicator.set(Status::Empty);

        EXPECT_TRUE(indicator.hint.empty());
    }

    TEST(StatusIndicator, OnClickAppendsWhatAClickDoes) {
        ttk::StatusIndicator indicator;

        EXPECT_EQ(indicator.on_click([] {}, "Click to stop it."), &indicator);
        EXPECT_TRUE(indicator.hint.empty()) << "an empty indicator has no tooltip";

        indicator.set(Status::Running);

        EXPECT_EQ(indicator.hint, "It is up. Click to stop it.");
        EXPECT_EQ(indicator.cursor, ttk::Cursor::Pointer);
    }

    TEST(StatusIndicator, NullClickIgnoresThePointerAgain) {
        ttk::StatusIndicator indicator;

        indicator.on_click([] {}, "Click.");

        EXPECT_TRUE(indicator.press(ttk::Pointer{}));

        indicator.on_click(nullptr, "");

        EXPECT_FALSE(indicator.press(ttk::Pointer{}));
        EXPECT_EQ(indicator.cursor, ttk::Cursor::Default);
        EXPECT_FALSE(indicator.takes_focus());
    }

    TEST(StatusIndicator, PressIsNotTakenWithoutAHandler) {
        ttk::StatusIndicator indicator;

        indicator.set(Status::Running);

        EXPECT_FALSE(indicator.press(ttk::Pointer{}));
        EXPECT_FALSE(indicator.takes_focus());
    }

    TEST_F(Painted, StatusIndicatorWidthIsTheWordPlusPadding) {
        const BLFont &face = type().at(600, ttk::Theme::fontTiny);

        for (const auto &[status, word] : {
                     std::pair{Status::Launching, "Launching"},
                     std::pair{Status::Running, "Running"},
                     std::pair{Status::Stopping, "Stopping"},
                     std::pair{Status::Closed, "Closed"},
                     std::pair{Status::Failed, "Failed"},
             }) {
            EXPECT_DOUBLE_EQ(ttk::StatusIndicator::width_of(type(), status), type().width(face, word) + 33.0) << word;
        }
    }

    TEST_F(Painted, StatusIndicatorNaturalWidthFollowsTheStatus) {
        ttk::StatusIndicator indicator;

        indicator.set(Status::Launching);

        EXPECT_DOUBLE_EQ(indicator.natural_width(type()), ttk::StatusIndicator::width_of(type(), Status::Launching));
    }

    TEST_F(Painted, StatusIndicatorRenderEmptyPaintsNothing) {
        tests::Canvas canvas;

        canvas.frame();

        const std::vector<std::uint32_t> before = canvas.pixels();
        const ttk::Painter painter(canvas.surface().context(), type(), BLRectI{0, 0, 400, 240});
        const BLRect box = ttk::StatusIndicator::render(painter, BLPoint{10, 10}, Status::Empty, false);

        canvas.surface().present();

        EXPECT_DOUBLE_EQ(box.w, 0.0);
        EXPECT_DOUBLE_EQ(box.h, 0.0);
        EXPECT_EQ(canvas.pixels(), before);
    }

    TEST_F(Painted, StatusIndicatorRenderReturnsTheBoxItCovers) {
        tests::Canvas canvas;

        canvas.frame();

        const ttk::Painter painter(canvas.surface().context(), type(), BLRectI{0, 0, 400, 240});
        const BLRect box = ttk::StatusIndicator::render(painter, BLPoint{30, 50}, Status::Running, false);

        canvas.surface().present();

        EXPECT_DOUBLE_EQ(box.x, 30);
        EXPECT_DOUBLE_EQ(box.y, 50);
        EXPECT_DOUBLE_EQ(box.w, ttk::StatusIndicator::width_of(type(), Status::Running));
        EXPECT_DOUBLE_EQ(box.h, ttk::StatusIndicator::HEIGHT);
        EXPECT_EQ(widgets::outside(canvas, BLRectI{30, 50, static_cast<int>(std::ceil(box.w)), 24}), 0U);
        EXPECT_EQ(canvas.at(43, 62), widgets::palette().success.value);
    }

    TEST_F(Painted, StatusIndicatorRenderDimOnlyFadesABeatingDot) {
        tests::Canvas canvas;

        canvas.frame();

        const ttk::Painter painter(canvas.surface().context(), type(), BLRectI{0, 0, 400, 240});

        ttk::StatusIndicator::render(painter, BLPoint{10, 10}, Status::Running, true);
        ttk::StatusIndicator::render(painter, BLPoint{10, 50}, Status::Launching, true);
        ttk::StatusIndicator::render(painter, BLPoint{10, 90}, Status::Launching, false);
        canvas.surface().present();

        EXPECT_EQ(canvas.at(23, 22), widgets::palette().success.value) << "dim faded a dot that does not beat";
        EXPECT_TRUE(widgets::near(canvas.at(23, 62), faint_dot(Status::Launching))) << std::hex << canvas.at(23, 62);
        EXPECT_EQ(canvas.at(23, 102), widgets::palette().accentHover.value);
    }

    TEST_F(Painted, StatusIndicatorEmptyPaintsNothing) {
        const Mounted mounted;

        EXPECT_EQ(widgets::outside(mounted.canvas, BLRectI{0, 0, 0, 0}), 0U);
    }

    class Tones : public Painted, public ::testing::WithParamInterface<Status> {};

    TEST_P(Tones, DotAndBorderTakeTheStatusTone) {
        const Mounted mounted(GetParam());
        const std::uint32_t tone = tone_of(GetParam());
        const std::uint32_t top = mounted.canvas.at(static_cast<int>(BOX.x + 20), static_cast<int>(BOX.y));

        EXPECT_EQ(mounted.dot(), tone);
        EXPECT_TRUE(widgets::near(top, widgets::over(widgets::over(widgets::background(), TILE),
                                                     ttk::Theme::alpha(BLRgba32(tone), 0.5).value)))
                << std::hex << top;
    }

    TEST_P(Tones, PaintsADarkTileAtTheTopLeftOfTheBox) {
        const Mounted mounted(GetParam());
        const int right = static_cast<int>(std::ceil(BOX.x + mounted.wide()));

        EXPECT_TRUE(widgets::near(mounted.canvas.at(right - 4, DOT_Y), widgets::over(widgets::background(), TILE)))
                << std::hex << mounted.canvas.at(right - 4, DOT_Y);
        EXPECT_EQ(mounted.canvas.at(right + 1, DOT_Y), widgets::background()) << "the pill is wider than width_of";
        EXPECT_EQ(widgets::outside(mounted.canvas, BLRectI{40, 40, right - 40, 24}), 0U);
    }

    TEST_P(Tones, WordIsDrawnInTheTone) {
        const Mounted mounted(GetParam());
        const BLRectI word{static_cast<int>(BOX.x) + 24, static_cast<int>(BOX.y) + 2,
                           static_cast<int>(mounted.wide()) - 32, 20};
        const std::uint32_t ink = widgets::peak(mounted.canvas, word, widgets::over(widgets::background(), TILE));

        EXPECT_LT(widgets::distance(ink, tone_of(GetParam())), 40) << std::hex << ink;
    }

    TEST_P(Tones, FollowAPaletteChange) {
        Mounted mounted(GetParam());

        ttk::Theme::set_mode(ttk::Theme::Mode::Light);
        mounted.canvas.frame();

        EXPECT_EQ(mounted.dot(), tone_of(GetParam()));
        EXPECT_TRUE(mounted.canvas.matches_full());
    }

    INSTANTIATE_TEST_SUITE_P(StatusIndicator, Tones,
                             ::testing::Values(Status::Launching, Status::Running, Status::Stopping, Status::Closed,
                                               Status::Failed));

    TEST_F(Painted, StatusIndicatorSetTonesReplacesEveryTone) {
        ttk::StatusIndicator::set_tones(&fixed_tones);

        const Mounted running(Status::Running);
        const Mounted closed(Status::Closed);
        const Mounted failed(Status::Failed);
        const Mounted launching(Status::Launching);

        EXPECT_EQ(running.dot(), 0xff00ff00U);
        EXPECT_EQ(closed.dot(), 0xff808080U);
        EXPECT_EQ(failed.dot(), 0xffff0000U);
        EXPECT_EQ(launching.dot(), 0xff0000ffU);
    }

    TEST_F(Painted, StatusIndicatorSetRepaintsOnlyTheIndicator) {
        Mounted mounted(Status::Closed);
        const std::vector<std::uint32_t> before = mounted.canvas.pixels();

        mounted.indicator->set(Status::Running);

        const std::size_t painted = mounted.canvas.frame();

        EXPECT_GT(painted, 0U);
        EXPECT_LE(painted, widgets::area(BOX));
        EXPECT_NE(mounted.canvas.pixels(), before);
        EXPECT_TRUE(mounted.canvas.matches_full());
    }

    TEST_F(Painted, StatusIndicatorSetToTheSameStatusDamagesNothing) {
        Mounted mounted(Status::Running);

        mounted.indicator->set(Status::Running);

        EXPECT_FALSE(mounted.canvas.root().dirty());
    }

    TEST_F(Painted, StatusIndicatorSetRestartsTheBlinkOnItsFullPhase) {
        Mounted mounted(Status::Running);

        mounted.canvas.frame_at(5.0);
        mounted.indicator->set(Status::Launching);
        mounted.canvas.frame_at(5.0);

        EXPECT_EQ(mounted.dot(), tone_of(Status::Launching)) << "the blink restarted faint";

        mounted.canvas.frame_at(5.0 + (ttk::StatusIndicator::BEAT / 2));

        EXPECT_EQ(mounted.dot(), tone_of(Status::Launching));
        EXPECT_DOUBLE_EQ(mounted.canvas.root().waking(), 5.0 + ttk::StatusIndicator::BEAT);

        mounted.canvas.frame_at(5.0 + ttk::StatusIndicator::BEAT);

        EXPECT_TRUE(widgets::near(mounted.dot(), faint_dot(Status::Launching))) << std::hex << mounted.dot();
    }

    TEST_F(Painted, StatusIndicatorFlipsAtTheTimeItWakes) {
        Mounted mounted(Status::Launching);

        mounted.canvas.frame_at(0.0);

        for (int beat = 1; beat <= 4; ++beat) {
            const double due = mounted.canvas.root().waking();

            mounted.canvas.frame_at(due);

            EXPECT_EQ(beat % 2 == 0, mounted.dot() == tone_of(Status::Launching))
                    << "beat " << beat << " at " << due << " did not flip";
        }
    }

    TEST_F(Painted, StatusIndicatorSetDoesNotRelayout) {
        Mounted mounted(Status::Closed);

        mounted.indicator->set(Status::Launching);

        EXPECT_FALSE(mounted.canvas.root().settle());
        EXPECT_DOUBLE_EQ(mounted.indicator->box().w, BOX.w);
    }

    TEST_F(Painted, StatusIndicatorBlinksEveryBeat) {
        Mounted mounted(Status::Launching);
        const std::uint32_t full = tone_of(Status::Launching);
        const std::uint32_t faint = faint_dot(Status::Launching);

        mounted.canvas.frame_at(0.0);

        EXPECT_EQ(mounted.dot(), full);

        mounted.canvas.frame_at(ttk::StatusIndicator::BEAT / 2);

        EXPECT_EQ(mounted.dot(), full);

        mounted.canvas.frame_at(ttk::StatusIndicator::BEAT);

        EXPECT_TRUE(widgets::near(mounted.dot(), faint)) << std::hex << mounted.dot();

        mounted.canvas.frame_at(ttk::StatusIndicator::BEAT * 2);

        EXPECT_EQ(mounted.dot(), full);
        EXPECT_TRUE(mounted.canvas.matches_full());
    }

    TEST_F(Painted, StatusIndicatorSleepsBetweenBeats) {
        Mounted mounted(Status::Stopping);

        mounted.canvas.frame_at(0.0);

        EXPECT_FALSE(mounted.canvas.root().busy()) << "a blinking dot kept the frame loop running";
        EXPECT_DOUBLE_EQ(mounted.canvas.root().waking(), ttk::StatusIndicator::BEAT);

        mounted.canvas.frame_at(ttk::StatusIndicator::BEAT);

        EXPECT_DOUBLE_EQ(mounted.canvas.root().waking(), ttk::StatusIndicator::BEAT * 2);
    }

    TEST_F(Painted, StatusIndicatorBlinkRepaintsOnlyTheIndicator) {
        Mounted mounted(Status::Launching);

        mounted.canvas.frame_at(0.0);

        for (int beat = 1; beat <= 3; ++beat) {
            const std::size_t painted = mounted.canvas.frame_at((ttk::StatusIndicator::BEAT + LATE) * beat);

            EXPECT_GT(painted, 0U) << "beat " << beat;
            EXPECT_LE(painted, widgets::area(BOX)) << "beat " << beat;
            EXPECT_TRUE(mounted.canvas.matches_full()) << "beat " << beat;
        }
    }

    TEST_F(Painted, StatusIndicatorStopsBlinkingOnASteadyStatus) {
        Mounted mounted(Status::Launching);

        mounted.canvas.frame_at(0.0);
        mounted.canvas.frame_at(ttk::StatusIndicator::BEAT);
        mounted.indicator->set(Status::Running);
        mounted.canvas.frame_at(ttk::StatusIndicator::BEAT * 2);
        mounted.canvas.frame_at(ttk::StatusIndicator::BEAT * 3);

        EXPECT_EQ(mounted.dot(), tone_of(Status::Running)) << "a steady status kept a faint dot";
        EXPECT_FALSE(mounted.canvas.root().busy());
        EXPECT_LT(mounted.canvas.root().waking(), 0.0) << "a steady status is still scheduled";
    }

    TEST_F(Painted, StatusIndicatorSteadyStatusPaintsTheSameEveryBeat) {
        Mounted mounted(Status::Running);
        const std::vector<std::uint32_t> first = mounted.canvas.pixels();

        for (int beat = 1; beat <= 3; ++beat) {
            EXPECT_EQ(mounted.canvas.frame_at(ttk::StatusIndicator::BEAT * beat), 0U);
        }

        EXPECT_EQ(mounted.canvas.pixels(), first);
    }

    TEST_F(Painted, StatusIndicatorClickInsideCalls) {
        Mounted mounted(Status::Running);
        int clicks = 0;

        mounted.indicator->on_click([&clicks] { ++clicks; }, "Click to stop it.");
        widgets::click(mounted.canvas, BOX.x + 10, BOX.y + 10);

        EXPECT_EQ(clicks, 1);
        EXPECT_EQ(mounted.canvas.root().grabbed(), nullptr);
    }

    TEST_F(Painted, StatusIndicatorReleaseOutsideDoesNotCall) {
        Mounted mounted(Status::Running);
        int clicks = 0;

        mounted.indicator->on_click([&clicks] { ++clicks; }, "");
        mounted.canvas.root().press(ttk::Pointer{.x = BOX.x + 10, .y = BOX.y + 10});

        EXPECT_EQ(mounted.canvas.root().grabbed(), mounted.indicator);

        mounted.canvas.root().release(ttk::Pointer{.x = BOX.x + 10, .y = BOX.y + 40});

        EXPECT_EQ(clicks, 0);
    }

    TEST_F(Painted, StatusIndicatorWithoutAHandlerIsOnlyRestedOn) {
        Mounted mounted(Status::Running);

        mounted.canvas.root().press(ttk::Pointer{.x = BOX.x + 10, .y = BOX.y + 10});

        EXPECT_EQ(mounted.canvas.root().grabbed(), nullptr);
        EXPECT_EQ(mounted.canvas.root().hovered(), mounted.indicator) << "the tooltip makes it hit";
    }

    TEST_F(Painted, StatusIndicatorEmptyIsNotHit) {
        const Mounted mounted;

        EXPECT_EQ(mounted.indicator->at(BOX.x + 10, BOX.y + 10), nullptr);
    }
}
