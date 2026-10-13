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
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "Mount.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Event.h"
#include "ttk/toolkit/controls/TabStrip.h"

namespace {
    using controls::key;
    namespace Code = ttk::Code;

    constexpr BLRect BOX{20, 20, 360, 42};
    constexpr double SIDES = 30.0;
    constexpr double GAP = 2.0;

    double label_width(const std::string_view label) {
        ttk::Typeface &type = tests::fonts();

        return type.width(type.at(600, ttk::Theme::fontBody), label);
    }

    std::vector<ttk::TabStrip::Tab> tabs() {
        return {{.label = "General"}, {.label = "Audio"}, {.label = "Video"}};
    }

    class TabStripTest : public controls::Painted {
    protected:
        tests::Canvas canvas;
        std::vector<int> picked;
        ttk::TabStrip *strip = nullptr;

        ttk::TabStrip *make(std::vector<ttk::TabStrip::Tab> held = tabs()) {
            auto made = std::make_unique<ttk::TabStrip>([this](const int index) { picked.push_back(index); });

            made->set_tabs(std::move(held));
            strip = controls::mount(canvas, std::move(made), BOX);

            return strip;
        }

        [[nodiscard]] static double left_of(const int index) {
            double x = BOX.x;

            for (int at = 0; at < index; ++at) {
                x += label_width(tabs()[static_cast<std::size_t>(at)].label) + SIDES + GAP;
            }

            return x;
        }

        [[nodiscard]] static double width_of(const int index) {
            return label_width(tabs()[static_cast<std::size_t>(index)].label) + SIDES;
        }

        void click_tab(const int index) {
            controls::click(canvas.root(), left_of(index) + (width_of(index) / 2.0), BOX.y + (BOX.h / 2.0));
        }

        void settle() {
            for (int step = 0; step < 10; ++step) {
                canvas.frame_at(canvas.root().now() + 0.05);
            }
        }

        [[nodiscard]] std::uint32_t underline(const int index) const {
            return canvas.at(static_cast<int>(left_of(index) + (width_of(index) / 2.0)),
                             static_cast<int>(BOX.y + BOX.h - 1.0));
        }

        std::vector<std::uint32_t> tab_pixels(const int index) {
            canvas.full();

            return controls::pixels_in(canvas,
                                       BLRectI{static_cast<int>(std::ceil(left_of(index))), static_cast<int>(BOX.y),
                                               static_cast<int>(width_of(index) - 2.0), static_cast<int>(BOX.h - 4.0)});
        }
    };

    TEST_F(TabStripTest, StartsWithNoCurrentTab) {
        const ttk::TabStrip empty(nullptr);

        EXPECT_EQ(empty.current(), -1);
    }

    TEST_F(TabStripTest, NaturalWidthIsEveryTabSideBySide) {
        ttk::TabStrip made(nullptr);

        made.set_tabs(tabs());

        const double expected = width_of(0) + GAP + width_of(1) + GAP + width_of(2);

        EXPECT_NEAR(made.natural_width(tests::fonts()), expected, 1e-3);

        made.fixedWidth = 10.0;

        EXPECT_NEAR(made.natural_width(tests::fonts()), expected, 1e-3) << "fixedWidth is meant to be ignored";
    }

    TEST_F(TabStripTest, NaturalWidthOfNoTabsIsZero) {
        ttk::TabStrip made(nullptr);

        EXPECT_DOUBLE_EQ(made.natural_width(tests::fonts()), 0.0);
    }

    TEST_F(TabStripTest, NaturalHeightIsAControl) {
        ttk::TabStrip made(nullptr);

        EXPECT_DOUBLE_EQ(made.natural_height(tests::fonts(), 100.0), ttk::Theme::control);
    }

    TEST_F(TabStripTest, ClickingATabReportsItWithoutChangingTheCurrentTab) {
        make();
        click_tab(1);

        EXPECT_EQ(picked, (std::vector<int>{1}));
        EXPECT_EQ(strip->current(), -1);
    }

    TEST_F(TabStripTest, EveryTabAnswersAtItsPlace) {
        make();

        for (int index = 0; index < 3; ++index) {
            controls::click(canvas.root(), left_of(index) + 1.0, BOX.y + 2.0);
            controls::click(canvas.root(), left_of(index) + width_of(index) - 1.0, BOX.y + BOX.h - 2.0);
        }

        EXPECT_EQ(picked, (std::vector<int>{0, 0, 1, 1, 2, 2}));
    }

    TEST_F(TabStripTest, TheGapBetweenTabsAnswersNothing) {
        make();

        const double gap = left_of(1) - 1.0;

        EXPECT_FALSE(strip->press(ttk::Pointer{.x = gap, .y = BOX.y + 10.0}));

        controls::click(canvas.root(), gap, BOX.y + 10.0);

        EXPECT_TRUE(picked.empty());
    }

    TEST_F(TabStripTest, PastTheLastTabAnswersNothing) {
        make();

        const double past = left_of(2) + width_of(2) + 5.0;

        EXPECT_FALSE(strip->press(ttk::Pointer{.x = past, .y = BOX.y + 10.0}));

        controls::click(canvas.root(), past, BOX.y + 10.0);

        EXPECT_TRUE(picked.empty());
    }

    TEST_F(TabStripTest, ClickingTheCurrentTabStillReports) {
        make();
        strip->set_current(0);
        click_tab(0);

        EXPECT_EQ(picked, (std::vector<int>{0}));
    }

    TEST_F(TabStripTest, ReleasingOnAnotherTabReportsThatOne) {
        make();
        controls::sweep(canvas.root(), left_of(0) + 5.0, BOX.y + 10.0, left_of(2) + 5.0, BOX.y + 10.0);

        EXPECT_EQ(picked, (std::vector<int>{2}));
    }

    TEST_F(TabStripTest, ReleasingOffEveryTabReportsNothing) {
        make();
        controls::sweep(canvas.root(), left_of(0) + 5.0, BOX.y + 10.0, 390.0, 200.0);

        EXPECT_TRUE(picked.empty());
    }

    TEST_F(TabStripTest, SetCurrentDoesNotReport) {
        make();
        strip->set_current(2);

        EXPECT_EQ(strip->current(), 2);
        EXPECT_TRUE(picked.empty());
    }

    TEST_F(TabStripTest, SetTabsKeepsACurrentTabInRange) {
        make();
        strip->set_current(1);
        strip->set_tabs({{.label = "One"}, {.label = "Two"}});

        EXPECT_EQ(strip->current(), 1);
    }

    TEST_F(TabStripTest, SetTabsDropsACurrentTabOutOfRange) {
        make();
        strip->set_current(2);
        strip->set_tabs({{.label = "One"}});

        EXPECT_EQ(strip->current(), -1);
    }

    TEST_F(TabStripTest, SetTabsAsksForALayout) {
        make();
        canvas.frame();
        strip->set_tabs({{.label = "Longer name"}, {.label = "Other"}});

        EXPECT_TRUE(canvas.root().dirty());

        canvas.frame();
        controls::click(canvas.root(), BOX.x + label_width("Longer name") + SIDES + GAP + 5.0, BOX.y + 10.0);

        EXPECT_EQ(picked, (std::vector<int>{1})) << "the tabs were not laid out again";
    }

    TEST_F(TabStripTest, LeftAndRightReportTheNeighbours) {
        make();
        strip->set_current(1);

        EXPECT_TRUE(strip->key(key(Code::Left)));
        EXPECT_TRUE(strip->key(key(Code::Right)));
        EXPECT_EQ(picked, (std::vector<int>{0, 2}));
        EXPECT_EQ(strip->current(), 1);
    }

    TEST_F(TabStripTest, LeftAtTheFirstTabReportsNothing) {
        make();
        strip->set_current(0);

        EXPECT_TRUE(strip->key(key(Code::Left)));
        EXPECT_TRUE(picked.empty());
    }

    TEST_F(TabStripTest, RightAtTheLastTabReportsNothing) {
        make();
        strip->set_current(2);

        EXPECT_TRUE(strip->key(key(Code::Right)));
        EXPECT_TRUE(picked.empty());
    }

    TEST_F(TabStripTest, HomeAndEndReportTheEnds) {
        make();
        strip->set_current(1);

        EXPECT_TRUE(strip->key(key(Code::Home)));
        EXPECT_TRUE(strip->key(key(Code::End)));
        EXPECT_EQ(picked, (std::vector<int>{0, 2}));
    }

    TEST_F(TabStripTest, HomeOnTheFirstTabReportsNothing) {
        make();
        strip->set_current(0);

        EXPECT_TRUE(strip->key(key(Code::Home)));
        EXPECT_TRUE(picked.empty());
    }

    TEST_F(TabStripTest, ArrowsWithoutACurrentTabGoToTheFirst) {
        make();

        strip->key(key(Code::Right));
        strip->key(key(Code::Left));

        EXPECT_EQ(picked, (std::vector<int>{0, 0}));
    }

    TEST_F(TabStripTest, KeysOnAnEmptyStripReportNothing) {
        make({});

        strip->key(key(Code::Left));
        strip->key(key(Code::Right));
        strip->key(key(Code::Home));
        strip->key(key(Code::End));

        EXPECT_TRUE(picked.empty());
    }

    TEST_F(TabStripTest, ReturnAndSpaceAreConsumedAndDoNothing) {
        make();
        strip->set_current(1);
        canvas.root().focus(strip);

        EXPECT_TRUE(strip->key(key(Code::Return)));
        EXPECT_TRUE(strip->key(key(Code::Space)));
        EXPECT_TRUE(canvas.root().key(key(Code::Space)));
        EXPECT_TRUE(picked.empty()) << "the root's activation fallback clicked a tab";
    }

    TEST_F(TabStripTest, OtherKeysAreNotConsumed) {
        make();

        EXPECT_FALSE(strip->key(key(Code::Up)));
        EXPECT_FALSE(strip->key(key(Code::Down)));
        EXPECT_FALSE(strip->key(key(Code::Tab)));
        EXPECT_FALSE(strip->key(key('a')));
    }

    TEST_F(TabStripTest, TheCurrentTabIsUnderlined) {
        make();

        const std::uint32_t bare = underline(1);

        strip->set_current(1);
        settle();

        EXPECT_NE(underline(1), bare);
        EXPECT_TRUE(controls::leans(underline(1), ttk::Theme::palette().accent, ttk::Theme::palette().background));
        EXPECT_EQ(underline(0), bare);
    }

    TEST_F(TabStripTest, TheUnderlineAnimatesAndStops) {
        make();
        canvas.root().advance(0.0);
        strip->set_current(1);

        EXPECT_TRUE(canvas.root().busy());

        settle();

        EXPECT_FALSE(canvas.root().busy());
        EXPECT_FALSE(strip->advance(canvas.root().now()));
    }

    TEST_F(TabStripTest, AnOutOfRangeCurrentTabUnderlinesNothing) {
        make();

        const std::uint32_t bare = underline(2);

        strip->set_current(9);
        settle();

        for (int index = 0; index < 3; ++index) {
            EXPECT_EQ(underline(index), bare) << "tab " << index;
        }
    }

    TEST_F(TabStripTest, SetTabsMovesTheUnderlineWithoutAnimating) {
        make();
        strip->set_current(1);
        settle();
        strip->set_tabs(tabs());
        canvas.full();

        EXPECT_TRUE(controls::leans(underline(1), ttk::Theme::palette().accent, ttk::Theme::palette().background));
    }

    TEST_F(TabStripTest, ABadgeDrawsADotOnATabThatIsNotCurrent) {
        make();
        strip->set_current(0);
        settle();

        const auto dot = [&](const int index) {
            return canvas.at(static_cast<int>(left_of(index) + width_of(index) - 11.0), static_cast<int>(BOX.y + 16.0));
        };

        canvas.full();

        const std::uint32_t bare = dot(1);

        strip->set_badge(1, true);
        canvas.frame();

        EXPECT_EQ(dot(1), ttk::Theme::palette().accent.value);

        strip->set_badge(0, true);
        canvas.frame();

        EXPECT_NE(dot(0), ttk::Theme::palette().accent.value) << "the current tab drew its badge";

        strip->set_badge(1, false);
        canvas.frame();

        EXPECT_EQ(dot(1), bare);
    }

    TEST_F(TabStripTest, SetBadgeOutOfRangeDoesNothing) {
        make();
        canvas.frame();
        strip->set_badge(-1, true);
        strip->set_badge(3, true);

        EXPECT_FALSE(canvas.root().dirty());
    }

    TEST_F(TabStripTest, SetBadgeToItsValueRepaintsNothing) {
        make({{.label = "One", .badge = true}});
        canvas.frame();
        strip->set_badge(0, true);

        EXPECT_FALSE(canvas.root().dirty());
    }

    TEST_F(TabStripTest, HoverHighlightsTheNameAndLeaveClearsIt) {
        make();

        const std::vector<std::uint32_t> resting = tab_pixels(1);

        canvas.root().motion(left_of(1) + 4.0, BOX.y + 10.0);

        EXPECT_NE(tab_pixels(1), resting);

        canvas.root().leave();

        EXPECT_EQ(tab_pixels(1), resting);
    }

    TEST_F(TabStripTest, PartialRepaintsMatchAFullOne) {
        make();
        strip->set_current(0);
        settle();
        canvas.root().motion(left_of(2) + 4.0, BOX.y + 10.0);
        canvas.frame();
        strip->set_current(2);
        settle();
        strip->set_badge(1, true);
        canvas.frame();

        EXPECT_TRUE(canvas.matches_full());
    }
}
