// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "Blocks.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/layout/Wrap.h"

namespace {
using tests::Block;
using tests::boxed;

ttk::Typeface &type() {
    return tests::fonts();
}

std::vector<Block *> fill(ttk::Wrap &wrap, const std::vector<BLSize> &sizes) {
    std::vector<Block *> made;

    made.reserve(sizes.size());

    for (const BLSize &size : sizes) {
        made.push_back(wrap.append(std::make_unique<Block>(size.w, size.h)));
    }

    return made;
}

TEST(Wrap, SpacingReturnsTheWrap) {
    ttk::Wrap wrap;

    EXPECT_EQ(wrap.spacing(1.0, 2.0), &wrap);
}

TEST(Wrap, EmptyWrapWantsNothing) {
    ttk::Wrap wrap;

    EXPECT_EQ(wrap.natural_width(type()), 0.0);
    EXPECT_EQ(wrap.natural_height(type(), 300.0), 0.0);
}

TEST(Wrap, NaturalWidthIsTheWidestChildNotTheSum) {
    ttk::Wrap wrap;

    fill(wrap, {{50, 10}, {120, 10}, {80, 10}});

    EXPECT_EQ(wrap.natural_width(type()), 120.0);
}

TEST(Wrap, FixedSizesOverrideTheChildren) {
    ttk::Wrap wrap;

    fill(wrap, {{50, 10}, {120, 10}});
    wrap.fixedWidth = 33.0;
    wrap.fixedHeight = 44.0;

    EXPECT_EQ(wrap.natural_width(type()), 33.0);
    EXPECT_EQ(wrap.natural_height(type(), 10.0), 44.0);
}

TEST(Wrap, DefaultSpacingIsEightBothWays) {
    ttk::Wrap wrap;
    const std::vector<Block *> blocks = fill(wrap, {{50, 10}, {50, 20}, {50, 10}});

    wrap.place({0, 0, 120, 100}, type());

    EXPECT_TRUE(boxed(blocks[0]->box(), {0, 0, 50, 10}));
    EXPECT_TRUE(boxed(blocks[1]->box(), {58, 0, 50, 20}));
    EXPECT_TRUE(boxed(blocks[2]->box(), {0, 28, 50, 10}));
}

TEST(Wrap, OneLineIsAsTallAsItsTallestChild) {
    ttk::Wrap wrap;

    wrap.spacing(4.0, 6.0);
    fill(wrap, {{30, 10}, {30, 25}, {30, 15}});

    EXPECT_EQ(wrap.natural_height(type(), 200.0), 25.0);
}

TEST(Wrap, LinesStackWithTheGapBetween) {
    ttk::Wrap wrap;

    wrap.spacing(4.0, 6.0);
    fill(wrap, {{60, 10}, {60, 25}, {60, 15}, {60, 5}});

    EXPECT_EQ(wrap.natural_height(type(), 130.0), 25.0 + 6.0 + 15.0);
    EXPECT_EQ(wrap.natural_height(type(), 60.0), 10.0 + 25.0 + 15.0 + 5.0 + (3 * 6.0));
}

TEST(Wrap, ChildrenSitAtTheTopOfTheirLine) {
    ttk::Wrap wrap;

    wrap.spacing(4.0, 6.0);

    const std::vector<Block *> blocks = fill(wrap, {{60, 10}, {60, 25}, {60, 15}, {60, 5}});

    wrap.place({20, 30, 130, 200}, type());

    EXPECT_TRUE(boxed(blocks[0]->box(), {20, 30, 60, 10})) << "a short child was not left at the line top";
    EXPECT_TRUE(boxed(blocks[1]->box(), {84, 30, 60, 25}));
    EXPECT_TRUE(boxed(blocks[2]->box(), {20, 61, 60, 15}));
    EXPECT_TRUE(boxed(blocks[3]->box(), {84, 61, 60, 5}));
}

TEST(Wrap, ExactFitStaysOnTheLine) {
    ttk::Wrap wrap;

    wrap.spacing(10.0, 10.0);

    const std::vector<Block *> blocks = fill(wrap, {{45, 10}, {45, 10}});

    wrap.place({0, 0, 100, 50}, type());

    EXPECT_EQ(blocks[1]->box().y, 0.0);
    EXPECT_EQ(blocks[1]->box().x, 55.0);

    wrap.place({0, 0, 99, 50}, type());

    EXPECT_EQ(blocks[1]->box().y, 20.0);
    EXPECT_EQ(blocks[1]->box().x, 0.0);
}

TEST(Wrap, WideChildIsCappedAndTakesItsOwnLine) {
    ttk::Wrap wrap;
    const std::vector<Block *> blocks = fill(wrap, {{30, 10}, {500, 0}, {30, 10}});

    blocks[1]->area = 2000.0;
    wrap.place({0, 0, 100, 200}, type());

    EXPECT_TRUE(boxed(blocks[0]->box(), {0, 0, 30, 10}));
    EXPECT_TRUE(boxed(blocks[1]->box(), {0, 18, 100, 20})) << "not capped at the width or measured at the cap";
    EXPECT_TRUE(boxed(blocks[2]->box(), {0, 46, 30, 10}));
}

TEST(Wrap, FirstChildOfALineNeverWraps) {
    ttk::Wrap wrap;
    const std::vector<Block *> blocks = fill(wrap, {{500, 10}});

    wrap.place({0, 0, 100, 200}, type());

    EXPECT_TRUE(boxed(blocks[0]->box(), {0, 0, 100, 10}));
    EXPECT_EQ(wrap.natural_height(type(), 100.0), 10.0) << "an empty line was counted above the child";
}

TEST(Wrap, HiddenChildrenAreSkipped) {
    ttk::Wrap wrap;
    const std::vector<Block *> blocks = fill(wrap, {{60, 10}, {300, 90}, {60, 10}});

    blocks[1]->set_visible(false);
    wrap.place({0, 0, 130, 200}, type());

    EXPECT_EQ(wrap.natural_width(type()), 60.0);
    EXPECT_EQ(wrap.natural_height(type(), 130.0), 10.0);
    EXPECT_TRUE(boxed(blocks[2]->box(), {68, 0, 60, 10}));
    EXPECT_TRUE(boxed(blocks[1]->box(), {0, 0, 0, 0}));
}

TEST(Wrap, MeasuredHeightMatchesTheArrangedLines) {
    ttk::Wrap wrap;

    wrap.spacing(5.0, 7.0);

    std::vector<BLSize> sizes;

    sizes.reserve(9);

    for (int at = 0; at < 9; ++at) {
        sizes.emplace_back(20.0 + (at * 7.0), 8.0 + (at % 4 * 3.0));
    }

    const std::vector<Block *> blocks = fill(wrap, sizes);

    for (const double width : {60.0, 97.0, 150.0, 400.0}) {
        const double tall = wrap.natural_height(type(), width);

        wrap.place({0, 0, width, tall}, type());

        double bottom = 0.0;

        for (const Block *block : blocks) {
            EXPECT_LE(block->box().x + block->box().w, width) << "ran past the width at " << width;
            bottom = std::max(bottom, block->box().y + block->box().h);
        }

        EXPECT_EQ(bottom, tall) << "at width " << width;
    }
}

TEST(Wrap, NarrowingAddsLines) {
    ttk::Wrap wrap;

    fill(wrap, {{40, 10}, {40, 10}, {40, 10}, {40, 10}});

    double last = 0.0;

    for (const double width : {400.0, 150.0, 40.0}) {
        const double tall = wrap.natural_height(type(), width);

        EXPECT_GT(tall, last) << "at width " << width;

        last = tall;
    }

    EXPECT_EQ(last, (4 * 10.0) + (3 * 8.0));
}

TEST(Wrap, AddedChildJoinsTheLastLine) {
    ttk::Wrap wrap;

    fill(wrap, {{40, 10}});
    wrap.place({0, 0, 200, 50}, type());

    Block *late = wrap.append(std::make_unique<Block>(40.0, 10.0));

    wrap.place({0, 0, 200, 50}, type());

    EXPECT_TRUE(boxed(late->box(), {48, 0, 40, 10}));
}

TEST(Wrap, PaintsChildrenWhereTheyLand) {
    tests::Canvas canvas(120, 80);
    auto *wrap = canvas.root().content()->append(std::make_unique<ttk::Wrap>());
    const std::vector<Block *> blocks = fill(*wrap, {{50, 20}, {50, 20}, {50, 20}});

    blocks[2]->colour = 0xffff8000U;
    canvas.frame();

    EXPECT_EQ(canvas.at(25, 38), 0xffff8000U);
    EXPECT_EQ(canvas.at(80, 38), ttk::Theme::palette().background.value);
    EXPECT_EQ(canvas.at(54, 10), ttk::Theme::palette().background.value);
    EXPECT_EQ(canvas.at(25, 24), ttk::Theme::palette().background.value);
    EXPECT_TRUE(canvas.matches_full());
}
}
