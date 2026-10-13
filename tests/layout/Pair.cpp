// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "Blocks.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/layout/Pair.h"

namespace {
    using tests::Block;
    using tests::boxed;
    using Place = ttk::Box::Place;

    constexpr double WIDEST = 200.0;

    ttk::Typeface &type() {
        return tests::fonts();
    }

    std::vector<Block *> fill(ttk::Pair &pair, const std::vector<BLSize> &sizes) {
        std::vector<Block *> made;

        made.reserve(sizes.size());

        for (const BLSize &size : sizes) {
            made.push_back(pair.append(std::make_unique<Block>(size.w, size.h)));
        }

        return made;
    }

    TEST(Pair, SideBySideSplitsTheWidthEquallyWhateverTheChildrenWant) {
        ttk::Pair pair(WIDEST);

        pair.spacing(10.0);

        const std::vector<Block *> blocks = fill(pair, {{30, 10}, {120, 20}});

        pair.place({0, 0, 300, 50}, type());

        EXPECT_TRUE(boxed(blocks[0]->box(), {0, 0, 145, 50}));
        EXPECT_TRUE(boxed(blocks[1]->box(), {155, 0, 145, 50}));
    }

    TEST(Pair, ExactlyTheWidestStaysSideBySide) {
        ttk::Pair pair(WIDEST);
        const std::vector<Block *> blocks = fill(pair, {{30, 10}, {30, 10}});

        pair.place({0, 0, WIDEST, 40}, type());

        EXPECT_EQ(blocks[1]->box().y, 0.0);
        EXPECT_EQ(blocks[1]->box().x, 100.0);
    }

    TEST(Pair, NarrowerThanTheWidestStacksAtFullWidth) {
        ttk::Pair pair(WIDEST);

        pair.spacing(10.0);

        const std::vector<Block *> blocks = fill(pair, {{30, 10}, {120, 20}});

        pair.place({5, 5, WIDEST - 1.0, 100}, type());

        EXPECT_TRUE(boxed(blocks[0]->box(), {5, 5, 199, 10}));
        EXPECT_TRUE(boxed(blocks[1]->box(), {5, 25, 199, 20}));
    }

    TEST(Pair, NaturalHeightFollowsTheArrangementForTheWidth) {
        ttk::Pair pair(WIDEST);

        pair.spacing(10.0);

        const std::vector<Block *> blocks = fill(pair, {{30, 0}, {30, 0}});

        blocks[0]->area = 1500.0;
        blocks[1]->area = 3000.0;

        EXPECT_EQ(pair.natural_height(type(), 310.0), 20.0) << "side by side is the taller half";
        EXPECT_EQ(pair.natural_height(type(), 150.0), 10.0 + 20.0 + 10.0) << "stacked is the sum and the spacing";
    }

    TEST(Pair, NaturalWidthIsTheRowOfWhatTheChildrenWant) {
        ttk::Pair pair(WIDEST);

        pair.spacing(10.0);
        fill(pair, {{30, 10}, {50, 10}});

        EXPECT_EQ(pair.natural_width(type()), 90.0);
    }

    TEST(Pair, CrossReturnsThePair) {
        ttk::Pair pair(WIDEST);

        EXPECT_EQ(pair.cross(Place::Centre), &pair);
    }

    TEST(Pair, CrossAppliesWhileSideBySide) {
        ttk::Pair pair(WIDEST);
        const std::vector<Block *> blocks = fill(pair, {{30, 30}, {30, 10}});

        pair.cross(Place::Centre);
        pair.place({0, 0, 300, 50}, type());

        EXPECT_TRUE(boxed(blocks[0]->box(), {0, 10, 150, 30}));
        EXPECT_TRUE(boxed(blocks[1]->box(), {150, 20, 150, 10}));
    }

    TEST(Pair, StackedChildrenAlwaysFillTheWidth) {
        ttk::Pair pair(WIDEST);
        const std::vector<Block *> blocks = fill(pair, {{30, 30}, {30, 10}});

        pair.cross(Place::End);
        pair.place({0, 0, 100, 100}, type());

        EXPECT_TRUE(boxed(blocks[0]->box(), {0, 0, 100, 30}));
        EXPECT_TRUE(boxed(blocks[1]->box(), {0, 30, 100, 10}));
    }

    TEST(Pair, CrossComesBackWhenWideAgain) {
        ttk::Pair pair(WIDEST);
        const std::vector<Block *> blocks = fill(pair, {{30, 30}, {30, 10}});

        pair.cross(Place::Start);
        pair.place({0, 0, 100, 100}, type());
        pair.place({0, 0, 300, 50}, type());

        EXPECT_TRUE(boxed(blocks[1]->box(), {150, 0, 150, 10}));
    }

    TEST(Pair, OneChildTakesItsWantedWidth) {
        ttk::Pair pair(WIDEST);
        const std::vector<Block *> blocks = fill(pair, {{40, 10}});

        pair.place({0, 0, 300, 50}, type());

        EXPECT_TRUE(boxed(blocks[0]->box(), {0, 0, 40, 50}));
    }

    TEST(Pair, OneVisibleChildTakesItsWantedWidth) {
        ttk::Pair pair(WIDEST);
        const std::vector<Block *> blocks = fill(pair, {{40, 10}, {60, 10}});

        blocks[0]->set_visible(false);
        pair.place({0, 0, 300, 50}, type());

        EXPECT_TRUE(boxed(blocks[1]->box(), {0, 0, 60, 50}));
    }

    TEST(Pair, ThreeChildrenSplitInThirds) {
        ttk::Pair pair(WIDEST);

        pair.spacing(10.0);

        const std::vector<Block *> blocks = fill(pair, {{10, 10}, {10, 10}, {10, 10}});

        pair.place({0, 0, 320, 50}, type());

        EXPECT_TRUE(boxed(blocks[0]->box(), {0, 0, 100, 50}));
        EXPECT_TRUE(boxed(blocks[1]->box(), {110, 0, 100, 50}));
        EXPECT_TRUE(boxed(blocks[2]->box(), {220, 0, 100, 50}));
    }

    TEST(Pair, HiddenChildIsLeftOutOfTheSplit) {
        ttk::Pair pair(WIDEST);

        pair.spacing(10.0);

        const std::vector<Block *> blocks = fill(pair, {{10, 10}, {10, 10}, {10, 10}});

        blocks[1]->set_visible(false);
        pair.place({0, 0, 310, 50}, type());

        EXPECT_TRUE(boxed(blocks[0]->box(), {0, 0, 150, 50}));
        EXPECT_TRUE(boxed(blocks[2]->box(), {160, 0, 150, 50}));
    }

    TEST(Pair, FlipsEachTimeTheWidthCrosses) {
        ttk::Pair pair(WIDEST);
        const std::vector<Block *> blocks = fill(pair, {{10, 10}, {10, 10}});

        for (int round = 0; round < 3; ++round) {
            pair.place({0, 0, 300, 60}, type());

            EXPECT_EQ(blocks[1]->box().y, 0.0) << "round " << round;

            pair.place({0, 0, 150, 60}, type());

            EXPECT_EQ(blocks[1]->box().y, 10.0) << "round " << round;
            EXPECT_EQ(blocks[1]->box().w, 150.0) << "round " << round;
        }
    }

    TEST(Pair, PaintsSideBySide) {
        tests::Canvas canvas(300, 60);
        auto *pair = canvas.root().content()->append(std::make_unique<ttk::Pair>(WIDEST));

        pair->spacing(20.0);

        const std::vector<Block *> blocks = fill(*pair, {{10, 10}, {10, 10}});

        blocks[0]->colour = 0xffff0000U;
        blocks[1]->colour = 0xff0000ffU;
        canvas.frame();

        EXPECT_EQ(canvas.at(139, 30), 0xffff0000U);
        EXPECT_EQ(canvas.at(150, 30), ttk::Theme::palette().background.value);
        EXPECT_EQ(canvas.at(160, 30), 0xff0000ffU);
        EXPECT_EQ(canvas.at(299, 59), 0xff0000ffU);
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST(Pair, PaintsStacked) {
        tests::Canvas canvas(150, 60);
        auto *pair = canvas.root().content()->append(std::make_unique<ttk::Pair>(WIDEST));
        const std::vector<Block *> blocks = fill(*pair, {{10, 10}, {10, 20}});

        blocks[0]->colour = 0xffff0000U;
        blocks[1]->colour = 0xff0000ffU;
        canvas.frame();

        EXPECT_EQ(canvas.at(149, 5), 0xffff0000U);
        EXPECT_EQ(canvas.at(149, 15), 0xff0000ffU);
        EXPECT_EQ(canvas.at(75, 45), ttk::Theme::palette().background.value);
        EXPECT_TRUE(canvas.matches_full());
    }
}
