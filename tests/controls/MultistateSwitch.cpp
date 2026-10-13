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
#include <optional>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "Mount.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Event.h"
#include "ttk/toolkit/controls/MultistateSwitch.h"

namespace {
    using controls::key;
    using Choice = ttk::MultistateSwitch::Choice;
    namespace Code = ttk::Code;

    constexpr BLRect BOX{20, 20, 360, 42};
    constexpr double PADDING = 34.0;

    double word(const std::string_view label) {
        ttk::Typeface &type = tests::fonts();

        return type.width(type.at(400, ttk::Theme::fontBody), label);
    }

    std::vector<Choice> choices() {
        return {{.value = 10, .label = "Low"}, {.value = 20, .label = "Medium"}, {.value = 30, .label = "High"}};
    }

    class SwitchTest : public controls::Painted {
    protected:
        tests::Canvas canvas;
        std::vector<int> picked;
        ttk::MultistateSwitch *control = nullptr;

        ttk::MultistateSwitch *make(const std::vector<Choice> &options = choices()) {
            auto made = std::make_unique<ttk::MultistateSwitch>([this](const int value) { picked.push_back(value); });

            made->set_options(options);
            control = controls::mount(canvas, std::move(made), BOX);

            return control;
        }

        [[nodiscard]] static BLRect lane(const int index) {
            double x = BOX.x + 4.0;

            for (int at = 0; at < index; ++at) {
                x += word(choices()[static_cast<std::size_t>(at)].label) + PADDING;
            }

            return BLRect{x, BOX.y + 4.0, word(choices()[static_cast<std::size_t>(index)].label) + PADDING,
                          BOX.h - 8.0};
        }

        void click_lane(const int index) {
            const BLRect box = lane(index);

            controls::click(canvas.root(), box.x + (box.w / 2.0), box.y + (box.h / 2.0));
        }

        void settle() {
            for (int step = 0; step < 10; ++step) {
                canvas.frame_at(canvas.root().now() + 0.05);
            }
        }

        // A point inside the marker of `index`, clear of its word and its rim.
        [[nodiscard]] std::uint32_t inside(const int index) const {
            const BLRect box = lane(index);

            return canvas.at(static_cast<int>(box.x + 4.0), static_cast<int>(box.y + (box.h / 2.0)));
        }
    };

    TEST_F(SwitchTest, StartsWithNothingCurrent) {
        const ttk::MultistateSwitch made(nullptr);

        EXPECT_FALSE(made.current().has_value());
    }

    TEST_F(SwitchTest, SetCurrentIsReadBack) {
        make();
        control->set_current(20);

        EXPECT_EQ(control->current(), std::optional<int>(20));
        EXPECT_TRUE(picked.empty());
    }

    TEST_F(SwitchTest, SetCurrentToAValueNoChoiceHasIsStillKept) {
        make();
        control->set_current(99);

        EXPECT_EQ(control->current(), std::optional<int>(99));
    }

    TEST_F(SwitchTest, NaturalWidthIsEveryWordWithItsPadding) {
        ttk::MultistateSwitch made(nullptr);

        made.set_options(choices());

        EXPECT_NEAR(made.natural_width(tests::fonts()),
                    8.0 + word("Low") + word("Medium") + word("High") + (3 * PADDING), 1e-3);

        made.fixedWidth = 77.0;

        EXPECT_DOUBLE_EQ(made.natural_width(tests::fonts()), 77.0);
    }

    TEST_F(SwitchTest, NaturalHeightIsAControlWhateverIsFixed) {
        ttk::MultistateSwitch made(nullptr);

        made.fixedHeight = 10.0;

        EXPECT_DOUBLE_EQ(made.natural_height(tests::fonts(), 100.0), ttk::Theme::control);
    }

    TEST_F(SwitchTest, ClickingAWordReportsItsValue) {
        make();
        click_lane(1);
        click_lane(2);
        click_lane(0);

        EXPECT_EQ(picked, (std::vector<int>{20, 30, 10}));
        EXPECT_FALSE(control->current().has_value()) << "a click changed the current choice by itself";
    }

    TEST_F(SwitchTest, ClickingTheCurrentWordStillReports) {
        make();
        control->set_current(20);
        click_lane(1);

        EXPECT_EQ(picked, (std::vector<int>{20}));
    }

    TEST_F(SwitchTest, TheLanesFollowTheWords) {
        make();

        for (int index = 0; index < 3; ++index) {
            const BLRect box = lane(index);

            controls::click(canvas.root(), box.x + 0.5, box.y + 2.0);
            controls::click(canvas.root(), box.x + box.w - 0.5, box.y + 2.0);
        }

        EXPECT_EQ(picked, (std::vector<int>{10, 10, 20, 20, 30, 30}));
    }

    TEST_F(SwitchTest, ClickingPastTheWordsReportsNothing) {
        make();

        const BLRect last = lane(2);

        controls::click(canvas.root(), last.x + last.w + 10.0, BOX.y + 20.0);
        controls::click(canvas.root(), BOX.x + 2.0, BOX.y + 20.0);

        EXPECT_TRUE(picked.empty());
    }

    TEST_F(SwitchTest, ReleasingOutsideTheSwitchReportsNothing) {
        make();
        controls::sweep(canvas.root(), lane(0).x + 10.0, BOX.y + 20.0, lane(0).x + 10.0, BOX.y + 120.0);

        EXPECT_TRUE(picked.empty());
    }

    TEST_F(SwitchTest, ADisabledSwitchIgnoresClicks) {
        make();
        control->set_enabled(false);

        EXPECT_FALSE(control->press(ttk::Pointer{.x = lane(0).x + 5.0, .y = BOX.y + 20.0}));

        click_lane(0);

        EXPECT_TRUE(picked.empty());
    }

    TEST_F(SwitchTest, SetOptionsReplacesTheLanesAtOnce) {
        make();
        control->set_options(std::vector<Choice>{{.value = 1, .label = "On"}, {.value = 2, .label = "Off"}});

        controls::click(canvas.root(), BOX.x + 4.0 + word("On") + PADDING + 2.0, BOX.y + 20.0);

        EXPECT_EQ(picked, (std::vector<int>{2})) << "a click before the relayout used the old lanes";
    }

    TEST_F(SwitchTest, SetOptionsAsksForALayout) {
        make();
        canvas.frame();
        control->set_options(std::vector<Choice>{{.value = 1, .label = "Only"}});

        EXPECT_TRUE(canvas.root().dirty());
    }

    TEST_F(SwitchTest, SetOptionsWithTheSameChoicesDoesNothing) {
        make();
        canvas.frame();

        const std::vector<Choice> same = choices();

        control->set_options(same);

        EXPECT_FALSE(canvas.root().dirty());

        control->set_options(choices());

        EXPECT_FALSE(canvas.root().dirty());
    }

    TEST_F(SwitchTest, ChoicesCompareEveryField) {
        const Choice one{.value = 1, .label = "a", .badge = false};

        EXPECT_EQ(one, (Choice{.value = 1, .label = "a", .badge = false}));
        EXPECT_NE(one, (Choice{.value = 2, .label = "a", .badge = false}));
        EXPECT_NE(one, (Choice{.value = 1, .label = "b", .badge = false}));
        EXPECT_NE(one, (Choice{.value = 1, .label = "a", .badge = true}));
    }

    TEST_F(SwitchTest, LeftAndRightReportTheNeighbours) {
        make();
        control->set_current(20);

        EXPECT_TRUE(control->key(key(Code::Left)));
        EXPECT_TRUE(control->key(key(Code::Right)));
        EXPECT_EQ(picked, (std::vector<int>{10, 30}));
    }

    TEST_F(SwitchTest, ArrowsStopAtTheEnds) {
        make();
        control->set_current(10);

        EXPECT_TRUE(control->key(key(Code::Left)));

        control->set_current(30);

        EXPECT_TRUE(control->key(key(Code::Right)));
        EXPECT_TRUE(picked.empty());
    }

    TEST_F(SwitchTest, HomeAndEndReportTheEnds) {
        make();
        control->set_current(20);

        EXPECT_TRUE(control->key(key(Code::Home)));
        EXPECT_TRUE(control->key(key(Code::End)));
        EXPECT_EQ(picked, (std::vector<int>{10, 30}));
    }

    TEST_F(SwitchTest, ArrowsWithoutACurrentChoiceGoToTheFirst) {
        make();
        control->key(key(Code::Right));
        control->key(key(Code::Left));
        control->set_current(99);
        control->key(key(Code::Right));

        EXPECT_EQ(picked, (std::vector<int>{10, 10, 10}));
    }

    TEST_F(SwitchTest, KeysOnAnEmptySwitchReportNothing) {
        make({});

        control->key(key(Code::Left));
        control->key(key(Code::Right));
        control->key(key(Code::Home));
        control->key(key(Code::End));

        EXPECT_TRUE(picked.empty());
    }

    TEST_F(SwitchTest, ReturnAndSpaceAreConsumedAndDoNothing) {
        make();
        control->set_current(10);
        canvas.root().focus(control);

        EXPECT_TRUE(control->key(key(Code::Return)));
        EXPECT_TRUE(control->key(key(Code::Space)));
        EXPECT_TRUE(canvas.root().key(key(Code::Return)));
        EXPECT_TRUE(picked.empty());
    }

    TEST_F(SwitchTest, OtherKeysAreNotConsumed) {
        make();

        EXPECT_FALSE(control->key(key(Code::Up)));
        EXPECT_FALSE(control->key(key(Code::Down)));
        EXPECT_FALSE(control->key(key(Code::Tab)));
        EXPECT_FALSE(control->key(key('x')));
    }

    TEST_F(SwitchTest, TheCurrentChoiceIsMarked) {
        make();

        const ttk::Theme::Palette &palette = ttk::Theme::palette();

        canvas.full();

        EXPECT_EQ(inside(1), palette.sunken.value) << "a marker was drawn with nothing current";

        control->set_current(20);
        settle();
        canvas.full();

        EXPECT_EQ(inside(1), palette.accentSoft.value);
        EXPECT_EQ(inside(0), palette.sunken.value);
    }

    TEST_F(SwitchTest, TheMarkerSlidesToANewChoice) {
        make();
        control->set_current(10);
        settle();
        control->set_current(30);
        canvas.frame_at(canvas.root().now() + 0.01);
        canvas.full();

        const ttk::Theme::Palette &palette = ttk::Theme::palette();

        EXPECT_NE(inside(2), palette.accentSoft.value) << "the marker jumped instead of sliding";

        settle();
        canvas.full();

        EXPECT_EQ(inside(2), palette.accentSoft.value);
        EXPECT_EQ(inside(0), palette.sunken.value);
    }

    TEST_F(SwitchTest, TheSlideEnds) {
        make();
        control->set_current(10);
        settle();
        control->set_current(20);

        EXPECT_TRUE(canvas.root().busy());

        settle();

        EXPECT_FALSE(canvas.root().busy());
    }

    TEST_F(SwitchTest, AValueNoChoiceHasRemovesTheMarker) {
        make();
        control->set_current(20);
        settle();
        control->set_current(99);
        canvas.root().relayout();
        settle();
        canvas.full();

        EXPECT_EQ(inside(1), ttk::Theme::palette().sunken.value);
    }

    TEST_F(SwitchTest, TheCurrentWordIsDrawnInTheAccent) {
        make();
        control->set_current(10);
        settle();
        canvas.full();

        const BLRect box = lane(0);
        const BLRectI area{static_cast<int>(box.x + 8.0), static_cast<int>(box.y + 4.0), static_cast<int>(box.w - 16.0),
                           static_cast<int>(box.h - 8.0)};
        const BLRect other = lane(1);
        const BLRectI otherArea{static_cast<int>(other.x + 8.0), static_cast<int>(other.y + 4.0),
                                static_cast<int>(other.w - 16.0), static_cast<int>(other.h - 8.0)};

        EXPECT_GT(controls::count_near(canvas, area, ttk::Theme::palette().accent, 40), 0);
        EXPECT_EQ(controls::count_near(canvas, otherArea, ttk::Theme::palette().accent, 40), 0);
    }

    TEST_F(SwitchTest, ABadgeDrawsADot) {
        std::vector<Choice> options = choices();

        options[2].badge = true;
        make(options);
        canvas.full();

        const BLRect box = lane(2);

        EXPECT_EQ(canvas.at(static_cast<int>(box.x + box.w - 11.0), static_cast<int>(box.y + 10.0)),
                  ttk::Theme::palette().accent.value);
    }

    TEST_F(SwitchTest, HoverChangesTheWordAndLeaveRestoresIt) {
        make();
        canvas.full();

        const BLRect box = lane(1);
        const BLRectI area{static_cast<int>(box.x), static_cast<int>(box.y), static_cast<int>(box.w),
                           static_cast<int>(box.h)};
        const std::vector<std::uint32_t> resting = controls::pixels_in(canvas, area);

        canvas.root().motion(box.x + 3.0, box.y + 3.0);
        canvas.full();

        EXPECT_NE(controls::pixels_in(canvas, area), resting);

        canvas.root().leave();
        canvas.full();

        EXPECT_EQ(controls::pixels_in(canvas, area), resting);
    }

    TEST_F(SwitchTest, PartialRepaintsMatchAFullOne) {
        make();
        control->set_current(10);
        settle();
        canvas.root().motion(lane(2).x + 5.0, BOX.y + 20.0);
        canvas.frame();
        control->set_current(30);
        settle();

        EXPECT_TRUE(canvas.matches_full());
    }
}
