// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "ttk/draw/Damage.h"

namespace {
BLRect rect(const int x, const int y, const int w, const int h) {
    return {static_cast<double>(x), static_cast<double>(y), static_cast<double>(w), static_cast<double>(h)};
}

bool same(const BLRectI &one, const BLRectI &two) {
    return one.x == two.x && one.y == two.y && one.w == two.w && one.h == two.h;
}

bool holds(const ttk::Damage &damage, const BLRectI &wanted) {
    return std::ranges::any_of(damage.regions(), [&](const BLRectI &r) { return same(r, wanted); });
}

bool covered(const ttk::Damage &damage, const int x, const int y) {
    return std::ranges::any_of(
            damage.regions(), [&](const BLRectI &r) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; });
}

std::size_t area(const ttk::Damage &damage) {
    std::size_t total = 0;

    for (const BLRectI &r : damage.regions()) {
        total += static_cast<std::size_t>(r.w) * static_cast<std::size_t>(r.h);
    }

    return total;
}

class Bitmap {
public:
    Bitmap(const int width, const int height)
        : _width(width), _height(height), _bits(static_cast<std::size_t>(width) * static_cast<std::size_t>(height)) {}

    void mark(const BLRectI &r) {
        for (int y = std::max(0, r.y); y < std::min(_height, r.y + r.h); ++y) {
            for (int x = std::max(0, r.x); x < std::min(_width, r.x + r.w); ++x) {
                _bits[index(x, y)] = true;
            }
        }
    }

    [[nodiscard]] bool at(const int x, const int y) const { return _bits[index(x, y)]; }

    [[nodiscard]] int width() const { return _width; }
    [[nodiscard]] int height() const { return _height; }

    // The first marked pixel the damage leaves uncovered, or -1.
    [[nodiscard]] long lost(const ttk::Damage &damage) const {
        Bitmap shown(_width, _height);

        for (const BLRectI &r : damage.regions()) {
            shown.mark(r);
        }

        for (std::size_t at = 0; at < _bits.size(); ++at) {
            if (_bits[at] && !shown._bits[at]) {
                return static_cast<long>(at);
            }
        }

        return -1;
    }

private:
    [[nodiscard]] std::size_t index(const int x, const int y) const {
        return (static_cast<std::size_t>(y) * static_cast<std::size_t>(_width)) + static_cast<std::size_t>(x);
    }

    int _width;
    int _height;
    std::vector<bool> _bits;
};

bool inside_frame(const ttk::Damage &damage, const int width, const int height) {
    return std::ranges::all_of(damage.regions(), [&](const BLRectI &r) {
        return r.w > 0 && r.h > 0 && r.x >= 0 && r.y >= 0 && r.x + r.w <= width && r.y + r.h <= height;
    });
}

TEST(Damage, FreshSetIsEmptyAndDropsEverything) {
    ttk::Damage damage;

    EXPECT_TRUE(damage.empty());
    EXPECT_FALSE(damage.whole());

    damage.add(rect(0, 0, 10, 10));

    EXPECT_TRUE(damage.empty()) << "a frame with no size kept a rectangle";

    const BLRectI clamped = damage.clamp_to(rect(0, 0, 10, 10));

    EXPECT_EQ(clamped.w, 0);
    EXPECT_EQ(clamped.h, 0);
}

TEST(Damage, ResizeClearsEveryRectangle) {
    ttk::Damage damage;

    damage.resize(1000, 1000);
    damage.add(rect(0, 0, 10, 10));
    damage.add(rect(800, 800, 10, 10));

    ASSERT_EQ(damage.regions().size(), 2U);

    damage.resize(200, 50);

    EXPECT_TRUE(damage.empty());

    damage.add(rect(150, 40, 100, 100));

    ASSERT_EQ(damage.regions().size(), 1U);
    EXPECT_TRUE(same(damage.regions()[0], {150, 40, 50, 10})) << "new frame size not used for clipping";
}

TEST(Damage, ResizeToNothingDropsLaterRectangles) {
    ttk::Damage damage;

    damage.resize(100, 100);
    damage.resize(0, 100);
    damage.add(rect(0, 0, 10, 10));

    EXPECT_TRUE(damage.empty());

    damage.resize(100, -5);
    damage.add(rect(0, 0, 10, 10));
    damage.all();

    EXPECT_TRUE(damage.empty());
}

TEST(Damage, ClampToRoundsOutward) {
    ttk::Damage damage;

    damage.resize(100, 100);

    EXPECT_TRUE(same(damage.clamp_to({0.2, 0.7, 1.1, 1.2}), {0, 0, 2, 2}));
    EXPECT_TRUE(same(damage.clamp_to({10.0, 20.0, 5.0, 6.0}), {10, 20, 5, 6}));
    EXPECT_TRUE(same(damage.clamp_to({10.999, 20.001, 0.002, 0.001}), {10, 20, 2, 1}));
}

TEST(Damage, ClampToClipsToTheFrame) {
    ttk::Damage damage;

    damage.resize(100, 50);

    EXPECT_TRUE(same(damage.clamp_to(rect(-10, -10, 20, 20)), {0, 0, 10, 10}));
    EXPECT_TRUE(same(damage.clamp_to(rect(90, 40, 50, 50)), {90, 40, 10, 10}));
    EXPECT_TRUE(same(damage.clamp_to(rect(-1000, -1000, 5000, 5000)), {0, 0, 100, 50}));
    EXPECT_TRUE(same(damage.clamp_to({99.5, 49.5, 0.25, 0.25}), {99, 49, 1, 1}));
}

TEST(Damage, ClampToIsEmptyOutsideTheFrame) {
    ttk::Damage damage;

    damage.resize(100, 50);

    for (const BLRect &outside : {
                 rect(100, 0, 10, 10),
                 rect(0, 50, 10, 10),
                 rect(-20, 0, 20, 10),
                 rect(0, -20, 10, 20),
                 rect(200, 200, 5, 5),
         }) {
        const BLRectI clamped = damage.clamp_to(outside);

        EXPECT_TRUE(clamped.w <= 0 || clamped.h <= 0) << "at " << outside.x << "," << outside.y;
    }
}

TEST(Damage, ClampToIsEmptyWithoutArea) {
    ttk::Damage damage;
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    constexpr double inf = std::numeric_limits<double>::infinity();

    damage.resize(100, 50);

    for (const BLRect &none : {
                 BLRect{10, 10, 0, 10},
                 BLRect{10, 10, 10, 0},
                 BLRect{10, 10, -5, 10},
                 BLRect{10, 10, 10, -5},
                 BLRect{nan, 0, 10, 10},
                 BLRect{0, 0, nan, 10},
                 BLRect{0, 0, inf, 10},
                 BLRect{-inf, 0, 10, 10},
                 BLRect{0, 0, 10, inf},
         }) {
        const BLRectI clamped = damage.clamp_to(none);

        EXPECT_EQ(clamped.w, 0);
        EXPECT_EQ(clamped.h, 0);
    }
}

TEST(Damage, AddDropsWhatClampToEmpties) {
    ttk::Damage damage;
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();

    damage.resize(100, 50);
    damage.add(BLRect{10, 10, 0, 10});
    damage.add(BLRect{nan, nan, nan, nan});
    damage.add(rect(100, 50, 10, 10));
    damage.add(rect(-10, -10, 10, 10));

    EXPECT_TRUE(damage.empty());
}

TEST(Damage, AddStoresTheClampedRectangle) {
    ttk::Damage damage;

    damage.resize(100, 50);
    damage.add({-0.5, 10.25, 20.0, 5.5});

    ASSERT_EQ(damage.regions().size(), 1U);
    EXPECT_TRUE(same(damage.regions()[0], damage.clamp_to({-0.5, 10.25, 20.0, 5.5})));
}

TEST(Damage, AllCoversTheFrame) {
    ttk::Damage damage;

    damage.resize(320, 200);
    damage.add(rect(5, 5, 5, 5));
    damage.add(rect(300, 150, 5, 5));
    damage.all();

    ASSERT_EQ(damage.regions().size(), 1U);
    EXPECT_TRUE(same(damage.regions()[0], {0, 0, 320, 200}));
    EXPECT_TRUE(damage.whole());
    EXPECT_FALSE(damage.empty());
}

TEST(Damage, AddingTheWholeFrameReplacesEverything) {
    ttk::Damage damage;

    damage.resize(320, 200);

    for (int at = 0; at < 10; ++at) {
        damage.add(rect(at * 30, at * 15, 4, 4));
    }

    damage.add(rect(-5, -5, 400, 400));

    ASSERT_EQ(damage.regions().size(), 1U);
    EXPECT_TRUE(damage.whole());

    damage.add(rect(10, 10, 10, 10));

    EXPECT_EQ(damage.regions().size(), 1U) << "damage inside the whole frame was kept apart";
}

TEST(Damage, WholeNeedsOneRectangleOverEverything) {
    ttk::Damage damage;

    damage.resize(100, 100);
    damage.add(rect(0, 0, 100, 99));

    EXPECT_FALSE(damage.whole());

    damage.add(rect(0, 99, 100, 1));

    EXPECT_TRUE(damage.whole()) << "adjacent halves did not merge into the frame";
}

TEST(Damage, ClearEmptiesButKeepsTheFrame) {
    ttk::Damage damage;

    damage.resize(100, 100);
    damage.all();
    damage.clear();

    EXPECT_TRUE(damage.empty());
    EXPECT_FALSE(damage.whole());

    damage.add(rect(90, 90, 20, 20));

    ASSERT_EQ(damage.regions().size(), 1U);
    EXPECT_TRUE(same(damage.regions()[0], {90, 90, 10, 10})) << "clear forgot the frame size";
}

TEST(Damage, ContainedRectangleIsDropped) {
    ttk::Damage damage;

    damage.resize(400, 400);
    damage.add(rect(10, 10, 100, 100));
    damage.add(rect(20, 20, 10, 10));
    damage.add(rect(10, 10, 100, 100));
    damage.add(rect(109, 109, 1, 1));

    ASSERT_EQ(damage.regions().size(), 1U);
    EXPECT_TRUE(same(damage.regions()[0], {10, 10, 100, 100}));
}

TEST(Damage, ContainingRectangleReplacesWhatItCovers) {
    ttk::Damage damage;

    damage.resize(1000, 1000);
    damage.add(rect(10, 10, 2, 2));
    damage.add(rect(300, 300, 2, 2));
    damage.add(rect(900, 900, 2, 2));

    ASSERT_EQ(damage.regions().size(), 3U);

    damage.add(rect(0, 0, 400, 400));

    EXPECT_EQ(damage.regions().size(), 2U);
    EXPECT_TRUE(holds(damage, {0, 0, 400, 400}));
    EXPECT_TRUE(holds(damage, {900, 900, 2, 2}));
}

TEST(Damage, JoinsAtExactlyWalkCleanPixels) {
    ttk::Damage damage;

    damage.resize(128, 1000);
    damage.add(rect(0, 0, 128, 1));
    damage.add(rect(0, 129, 128, 1));

    ASSERT_EQ(damage.regions().size(), 1U) << "a join costing exactly WALK was refused";
    EXPECT_TRUE(same(damage.regions()[0], {0, 0, 128, 130}));
}

TEST(Damage, KeepsApartPastWalkCleanPixels) {
    ttk::Damage damage;

    damage.resize(128, 1000);
    damage.add(rect(0, 0, 128, 1));
    damage.add(rect(0, 130, 128, 1));

    EXPECT_EQ(damage.regions().size(), 2U) << "a join costing WALK + 128 was taken";
    EXPECT_EQ(area(damage), 256U);
}

TEST(Damage, OverlapIsCountedOnceWhenCostingAJoin) {
    ttk::Damage damage;

    damage.resize(1000, 1000);
    damage.add(rect(0, 0, 200, 200));
    damage.add(rect(100, 100, 200, 200));

    EXPECT_EQ(damage.regions().size(), 2U) << "a join adding 20000 clean pixels was taken";

    damage.clear();
    damage.add(rect(0, 0, 100, 100));
    damage.add(rect(50, 50, 100, 100));

    ASSERT_EQ(damage.regions().size(), 1U) << "a join adding 5000 clean pixels was refused";
    EXPECT_TRUE(same(damage.regions()[0], {0, 0, 150, 150}));
}

TEST(Damage, JoinsThePartnerAddingFewestCleanPixels) {
    ttk::Damage damage;

    damage.resize(2000, 2000);
    damage.add(rect(0, 0, 10, 10));
    damage.add(rect(1000, 1000, 10, 10));

    ASSERT_EQ(damage.regions().size(), 2U);

    damage.add(rect(1020, 1000, 10, 10));

    ASSERT_EQ(damage.regions().size(), 2U);
    EXPECT_TRUE(holds(damage, {0, 0, 10, 10})) << "the distant rectangle was touched";
    EXPECT_TRUE(holds(damage, {1000, 1000, 30, 10}));
}

TEST(Damage, JoinRepeatsWhileItReachesAnother) {
    ttk::Damage damage;

    damage.resize(100, 1000);
    damage.add(rect(0, 0, 100, 1));
    damage.add(rect(0, 300, 100, 1));

    ASSERT_EQ(damage.regions().size(), 2U);

    damage.add(rect(0, 150, 100, 1));

    ASSERT_EQ(damage.regions().size(), 1U) << "the join stopped after its first partner";
    EXPECT_TRUE(same(damage.regions()[0], {0, 0, 100, 301}));
}

TEST(Damage, AdjacentRowStripsMergeExactly) {
    ttk::Damage damage;

    damage.resize(640, 480);

    for (int y = 0; y < 480; ++y) {
        damage.add(rect(0, y, 640, 1));
    }

    ASSERT_EQ(damage.regions().size(), 1U);
    EXPECT_TRUE(damage.whole());
}

TEST(Damage, AdjacentColumnStripsMergeExactly) {
    ttk::Damage damage;

    damage.resize(640, 480);

    for (int x = 100; x < 300; x += 4) {
        damage.add(rect(x, 50, 4, 200));
    }

    ASSERT_EQ(damage.regions().size(), 1U);
    EXPECT_TRUE(same(damage.regions()[0], {100, 50, 200, 200}));
    EXPECT_EQ(area(damage), 40000U) << "adjacent strips gained clean pixels";
}

TEST(Damage, StripsInReverseOrderMergeExactly) {
    ttk::Damage damage;

    damage.resize(640, 480);

    for (int y = 477; y >= 0; y -= 3) {
        damage.add(rect(20, y, 600, 3));
    }

    ASSERT_EQ(damage.regions().size(), 1U);
    EXPECT_TRUE(same(damage.regions()[0], {20, 0, 600, 480}));
}

TEST(Damage, CrowdedSetHoldsAtMostCrowded) {
    ttk::Damage damage;
    constexpr int spacing = 200;
    const int side = static_cast<int>(ttk::Damage::CROWDED + 2) * spacing;
    Bitmap expected(side, side);

    damage.resize(side, side);

    for (std::size_t at = 0; at < ttk::Damage::CROWDED; ++at) {
        const BLRectI pixel{static_cast<int>(at) * spacing, static_cast<int>(at) * spacing, 1, 1};

        damage.add(rect(pixel.x, pixel.y, 1, 1));
        expected.mark(pixel);
    }

    ASSERT_EQ(damage.regions().size(), ttk::Damage::CROWDED) << "far apart pixels were joined";

    const int last = static_cast<int>(ttk::Damage::CROWDED + 1) * spacing;

    damage.add(rect(last, last, 1, 1));
    expected.mark({last, last, 1, 1});

    EXPECT_LE(damage.regions().size(), ttk::Damage::CROWDED);
    EXPECT_EQ(expected.lost(damage), -1);
    EXPECT_TRUE(holds(damage, {0, 0, 1, 1})) << "the crowded join did not pick the nearest partner";
}

TEST(Damage, RepeatsAddNothing) {
    ttk::Damage damage;

    damage.resize(500, 500);
    damage.add(rect(10, 10, 5, 5));
    damage.add(rect(400, 400, 5, 5));

    const std::vector<BLRectI> before = damage.regions();

    for (int at = 0; at < 50; ++at) {
        damage.add(rect(400, 400, 5, 5));
        damage.add({10.25, 10.25, 4.5, 4.5});
    }

    ASSERT_EQ(damage.regions().size(), before.size());

    for (const BLRectI &r : before) {
        EXPECT_TRUE(holds(damage, r));
    }
}

struct Seeded : ::testing::TestWithParam<unsigned> {};

TEST_P(Seeded, RandomRectanglesNeverLoseAPixel) {
    constexpr int width = 300;
    constexpr int height = 200;
    ttk::Damage damage;
    Bitmap expected(width, height);
    std::mt19937 random(GetParam());
    std::uniform_int_distribution<int> across(-20, width + 20);
    std::uniform_int_distribution<int> down(-20, height + 20);
    std::uniform_int_distribution<int> size(0, 40);

    damage.resize(width, height);

    for (int at = 0; at < 300; ++at) {
        const BLRect added{static_cast<double>(across(random)) + 0.25,
                           static_cast<double>(down(random)),
                           static_cast<double>(size(random)),
                           static_cast<double>(size(random)) + 0.5};

        damage.add(added);
        expected.mark(damage.clamp_to(added));

        ASSERT_LE(damage.regions().size(), ttk::Damage::CROWDED);
        ASSERT_TRUE(inside_frame(damage, width, height)) << "a rectangle reached past the frame";
        ASSERT_EQ(expected.lost(damage), -1) << "after " << at + 1 << " rectangles";
    }
}

TEST_P(Seeded, ScatteredPixelsNeverExceedCrowded) {
    constexpr int side = 4096;
    ttk::Damage damage;
    Bitmap expected(side, side);
    std::mt19937 random(GetParam());
    std::uniform_int_distribution<int> where(0, side - 1);

    damage.resize(side, side);

    for (int at = 0; at < 120; ++at) {
        const BLRectI pixel{where(random), where(random), 1, 1};

        damage.add(rect(pixel.x, pixel.y, 1, 1));
        expected.mark(pixel);

        ASSERT_LE(damage.regions().size(), ttk::Damage::CROWDED);
    }

    EXPECT_EQ(expected.lost(damage), -1);
}

TEST_P(Seeded, FarApartRectanglesStayApart) {
    constexpr int cell = 2000;
    constexpr int cells = 5;
    ttk::Damage damage;
    std::mt19937 random(GetParam());
    std::uniform_int_distribution<int> offset(0, 30);
    std::uniform_int_distribution<int> size(10, 40);
    std::vector<BLRectI> added;

    damage.resize(cell * cells, cell * cells);

    for (int row = 0; row < cells; ++row) {
        for (int column = 0; column < cells; ++column) {
            const BLRectI one{
                    (column * cell) + offset(random), (row * cell) + offset(random), size(random), size(random)};

            damage.add(rect(one.x, one.y, one.w, one.h));
            added.push_back(one);
        }
    }

    ASSERT_EQ(damage.regions().size(), added.size());

    for (const BLRectI &one : added) {
        EXPECT_TRUE(holds(damage, one)) << "at " << one.x << "," << one.y;
    }
}

TEST_P(Seeded, ShiftedDamageCoversWhereThePixelsLand) {
    constexpr int width = 200;
    constexpr int height = 200;
    std::mt19937 random(GetParam());
    std::uniform_int_distribution<int> where(0, 180);
    std::uniform_int_distribution<int> size(1, 30);
    std::uniform_int_distribution<int> move(-60, 60);

    for (int round = 0; round < 40; ++round) {
        ttk::Damage damage;
        Bitmap before(width, height);

        damage.resize(width, height);

        for (int at = 0; at < 4; ++at) {
            const BLRectI one{where(random), where(random), size(random), size(random)};

            damage.add(rect(one.x, one.y, one.w, one.h));
        }

        for (const BLRectI &r : damage.regions()) {
            before.mark(r);
        }

        const BLRectI region{where(random) / 2, where(random) / 2, 20 + (where(random) / 2), 20 + (where(random) / 2)};
        const int dy = move(random);
        Bitmap expected = before;

        for (int y = region.y; y < region.y + region.h; ++y) {
            for (int x = region.x; x < region.x + region.w; ++x) {
                const int to = y + dy;

                if (before.at(x, y) && to >= region.y && to < region.y + region.h && to < height && x < width) {
                    expected.mark({x, to, 1, 1});
                }
            }
        }

        damage.shift(region, dy);

        ASSERT_EQ(expected.lost(damage), -1) << "round " << round << " dy " << dy;
        ASSERT_LE(damage.regions().size(), ttk::Damage::CROWDED);
    }
}

TEST_P(Seeded, ShiftedDamageGoesOnlyWhereThePixelsLand) {
    constexpr int side = 400;
    std::mt19937 random(GetParam());
    std::uniform_int_distribution<int> where(0, side - 60);
    std::uniform_int_distribution<int> size(1, 60);
    std::uniform_int_distribution<int> move(-200, 200);

    for (int round = 0; round < 200; ++round) {
        ttk::Damage damage;
        const BLRectI held{where(random), where(random), size(random), size(random)};
        const BLRectI region{where(random) / 2, where(random) / 2, 60 + (where(random) / 2), 60 + (where(random) / 2)};
        const int dy = move(random);

        damage.resize(side, side);
        damage.add(rect(held.x, held.y, held.w, held.h));
        damage.shift(region, dy);

        const int x0 = std::max(held.x, region.x);
        const int x1 = std::min(held.x + held.w, region.x + region.w);
        const int y0 = std::max(std::max(held.y, region.y) + dy, region.y);
        const int y1 = std::min(std::min(held.y + held.h, region.y + region.h) + dy, region.y + region.h);
        BLRectI reach = held;

        if (x1 > x0 && y1 > y0) {
            reach.x = std::min(held.x, x0);
            reach.y = std::min(held.y, y0);
            reach.w = std::max(held.x + held.w, x1) - reach.x;
            reach.h = std::max(held.y + held.h, y1) - reach.y;
        }

        for (const BLRectI &r : damage.regions()) {
            ASSERT_GE(r.x, reach.x) << "round " << round;
            ASSERT_GE(r.y, reach.y) << "round " << round;
            ASSERT_LE(r.x + r.w, reach.x + reach.w) << "round " << round;
            ASSERT_LE(r.y + r.h, reach.y + reach.h) << "round " << round;
        }

        ASSERT_TRUE(covered(damage, held.x, held.y)) << "round " << round;
    }
}

INSTANTIATE_TEST_SUITE_P(Damage, Seeded, ::testing::Values(1U, 2U, 3U, 17U, 4242U, 90210U));

TEST(Damage, ShiftByZeroChangesNothing) {
    ttk::Damage damage;

    damage.resize(100, 100);
    damage.add(rect(10, 10, 10, 10));
    damage.shift({0, 0, 100, 100}, 0);

    ASSERT_EQ(damage.regions().size(), 1U);
    EXPECT_TRUE(same(damage.regions()[0], {10, 10, 10, 10}));
}

TEST(Damage, ShiftAfterWholeDamageChangesNothing) {
    ttk::Damage damage;

    damage.resize(100, 100);
    damage.all();
    damage.shift({0, 0, 100, 100}, 10);

    ASSERT_EQ(damage.regions().size(), 1U);
    EXPECT_TRUE(damage.whole());
}

TEST(Damage, ShiftOfAnEmptySetAddsNothing) {
    ttk::Damage damage;

    damage.resize(100, 100);
    damage.shift({0, 0, 100, 100}, 10);

    EXPECT_TRUE(damage.empty());
}

TEST(Damage, ShiftLeavesDamageOutsideItsRegionAlone) {
    ttk::Damage damage;

    damage.resize(1000, 1000);
    damage.add(rect(10, 10, 10, 10));
    damage.add(rect(800, 800, 10, 10));
    damage.shift({0, 400, 1000, 300}, 50);

    EXPECT_EQ(damage.regions().size(), 2U);
    EXPECT_EQ(area(damage), 200U) << "shift damaged pixels outside its region";
}

TEST(Damage, ShiftDownAddsTheMovedCopy) {
    ttk::Damage damage;

    damage.resize(1000, 1000);
    damage.add(rect(100, 300, 100, 10));
    damage.shift({0, 200, 1000, 600}, 300);

    EXPECT_TRUE(holds(damage, {100, 300, 100, 10})) << "the pending rectangle did not stay";
    EXPECT_TRUE(holds(damage, {100, 600, 100, 10})) << "the moved copy is missing";
    EXPECT_EQ(area(damage), 2000U);
}

TEST(Damage, ShiftUpAddsTheMovedCopy) {
    ttk::Damage damage;

    damage.resize(1000, 1000);
    damage.add(rect(100, 700, 100, 10));
    damage.shift({0, 200, 1000, 600}, -300);

    EXPECT_TRUE(holds(damage, {100, 700, 100, 10}));
    EXPECT_TRUE(holds(damage, {100, 400, 100, 10}));
    EXPECT_EQ(area(damage), 2000U);
}

TEST(Damage, ShiftDropsWhatMovesOutOfItsRegion) {
    ttk::Damage damage;

    damage.resize(1000, 1000);
    damage.add(rect(100, 750, 20, 40));
    damage.shift({0, 200, 1000, 600}, 30);

    ASSERT_EQ(damage.regions().size(), 1U);
    EXPECT_TRUE(same(damage.regions()[0], {100, 750, 20, 50})) << "the copy was not cut at the region's bottom";
    EXPECT_FALSE(covered(damage, 100, 800));
}

TEST(Damage, ShiftMovesOnlyThePartInsideItsRegion) {
    ttk::Damage damage;

    damage.resize(1000, 1000);
    damage.add(rect(100, 150, 200, 100));
    damage.shift({150, 200, 100, 500}, 400);

    EXPECT_TRUE(holds(damage, {100, 150, 200, 100}));
    EXPECT_TRUE(holds(damage, {150, 600, 100, 50})) << "the copy was not cut to the region";
    EXPECT_EQ(area(damage), 25000U);
}

TEST(Damage, ShiftByTheWholeRegionOrMoreAddsNothing) {
    ttk::Damage damage;

    damage.resize(1000, 1000);
    damage.add(rect(100, 300, 20, 10));

    for (const int dy : {300, -300, 301, -301, std::numeric_limits<int>::max(), std::numeric_limits<int>::min()}) {
        damage.shift({0, 200, 1000, 300}, dy);

        ASSERT_EQ(damage.regions().size(), 1U) << "dy " << dy;
        EXPECT_TRUE(same(damage.regions()[0], {100, 300, 20, 10})) << "dy " << dy;
    }
}

TEST(Damage, ShiftBeforeASizeDoesNothing) {
    ttk::Damage damage;

    damage.shift({0, 0, 100, 100}, 10);

    EXPECT_TRUE(damage.empty());
}
}
