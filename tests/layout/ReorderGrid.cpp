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

#include <blend2d/blend2d.h>
#include <gtest/gtest.h>

#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Widget.h"
#include "ttk/toolkit/layout/ReorderGrid.h"

namespace {
// What offsets() reads off a card.
struct Slid {
    double x = 0.0;
    double y = 0.0;

    [[nodiscard]] double slide_x() const { return x; }
    [[nodiscard]] double slide_y() const { return y; }
};

constexpr ttk::ReorderGrid::Metrics SMALL{
        .bleed = 10.0,
        .gutter = 10.0,
        .top = 5.0,
        .narrowest = 80.0,
        .row_height = 60.0,
        .step = 1.0,
};

constexpr ttk::ReorderGrid::Metrics SHELF{.top = 10.0, .narrowest = 244.0, .row_height = 232.0, .step = 8.0};

// Four columns of 87 pixels, starting at x 110 and y 55, a cell every 97 across and 70 down.
class Grid : public ::testing::Test {
protected:
    void SetUp() override { grid.place({100, 50, 400, 300}); }

    ttk::Widget page;
    ttk::ReorderGrid grid{&page, SMALL};
};

TEST(ReorderGrid, MetricsDefaultToTheTheme) {
    const ttk::ReorderGrid::Metrics metrics;

    EXPECT_EQ(metrics.bleed, ttk::Theme::bleed);
    EXPECT_EQ(metrics.gutter, ttk::Theme::gutter);
    EXPECT_EQ(metrics.top, 0.0);
    EXPECT_EQ(metrics.narrowest, 0.0);
    EXPECT_EQ(metrics.row_height, 0.0);
    EXPECT_EQ(metrics.step, 1.0);
}

TEST(ReorderGrid, FitsAsManyColumnsAsTheNarrowestAllows) {
    ttk::Widget page;
    ttk::ReorderGrid grid(&page, SHELF);

    grid.measure(1180.0);

    EXPECT_EQ(grid.columns(), 4);
    EXPECT_EQ(grid.cell(), 272.0);

    grid.measure(1000.0);

    EXPECT_EQ(grid.columns(), 3);
}

TEST(ReorderGrid, CellIsRoundedDownToTheStep) {
    ttk::Widget page;
    ttk::ReorderGrid grid(&page, SHELF);

    grid.measure(1000.0);

    EXPECT_EQ(grid.cell(), 304.0);

    ttk::ReorderGrid::Metrics fine = SHELF;

    fine.step = 1.0;
    grid.set_metrics(fine);
    grid.measure(1000.0);

    EXPECT_EQ(grid.cell(), 309.0);
}

TEST(ReorderGrid, TooNarrowGivesOneColumnOfTheNarrowestWidth) {
    ttk::Widget page;
    ttk::ReorderGrid grid(&page, SMALL);

    grid.measure(50.0);

    EXPECT_EQ(grid.columns(), 1);
    EXPECT_EQ(grid.cell(), 80.0);

    grid.measure(0.0);

    EXPECT_EQ(grid.columns(), 1);
    EXPECT_EQ(grid.cell(), 80.0);
}

TEST(ReorderGrid, ColumnsAreEqualAndFillTheRoom) {
    ttk::Widget page;
    ttk::ReorderGrid grid(&page, SMALL);

    for (int step = 0; step < 52; ++step) {
        const double width = 100.0 + (step * 37.0);

        grid.measure(width);

        const double room = width - (2 * SMALL.bleed);
        const double used = (grid.columns() * grid.cell()) + ((grid.columns() - 1) * SMALL.gutter);

        EXPECT_GE(grid.cell(), SMALL.narrowest) << "at " << width;
        EXPECT_LE(used, room) << "at " << width;
        EXPECT_GT(used, room - grid.columns()) << "the cells left more than a pixel each unused at " << width;
        EXPECT_GT(((grid.columns() + 1) * SMALL.narrowest) + (grid.columns() * SMALL.gutter), room)
                << "another column would have fit at " << width;
    }
}

TEST_F(Grid, PlaceMeasuresForTheBoxWidth) {
    EXPECT_EQ(grid.columns(), 4);
    EXPECT_EQ(grid.cell(), 87.0);
}

TEST_F(Grid, MeasureLeavesTheBoxAlone) {
    grid.measure(200.0);

    EXPECT_EQ(grid.columns(), 2);
    EXPECT_EQ(grid.cell_x(0), 110.0) << "measure moved the box";
    EXPECT_EQ(grid.cell_y(0), 55.0);
}

TEST_F(Grid, AccessorsReturnTheMetrics) {
    EXPECT_EQ(grid.row_height(), 60.0);
    EXPECT_EQ(grid.gutter(), 10.0);
    EXPECT_EQ(grid.top(), 5.0);
}

TEST_F(Grid, NewMetricsWaitForTheNextMeasure) {
    ttk::ReorderGrid::Metrics wide = SMALL;

    wide.narrowest = 180.0;
    grid.set_metrics(wide);

    EXPECT_EQ(grid.columns(), 4);

    grid.measure(400.0);

    EXPECT_EQ(grid.columns(), 2);
}

TEST_F(Grid, SetRowHeightMovesTheRows) {
    grid.set_row_height(100.0);

    EXPECT_EQ(grid.row_height(), 100.0);
    EXPECT_EQ(grid.cell_y(4), 55.0 + 110.0);
}

TEST_F(Grid, RowsForCountsWholeRows) {
    EXPECT_EQ(grid.rows_for(0), 0);
    EXPECT_EQ(grid.rows_for(1), 1);
    EXPECT_EQ(grid.rows_for(4), 1);
    EXPECT_EQ(grid.rows_for(5), 2);
    EXPECT_EQ(grid.rows_for(8), 2);
    EXPECT_EQ(grid.rows_for(9), 3);
}

TEST_F(Grid, RowsWithAdderCountsTheTileAfterTheLast) {
    EXPECT_EQ(grid.rows_with_adder(0), 1);
    EXPECT_EQ(grid.rows_with_adder(3), 1);
    EXPECT_EQ(grid.rows_with_adder(4), 2);
    EXPECT_EQ(grid.rows_with_adder(7), 2);
    EXPECT_EQ(grid.rows_with_adder(8), 3);
}

TEST_F(Grid, CellsFillRowByRow) {
    EXPECT_EQ(grid.cell_x(0), 110.0);
    EXPECT_EQ(grid.cell_x(1), 207.0);
    EXPECT_EQ(grid.cell_x(3), 401.0);
    EXPECT_EQ(grid.cell_x(4), 110.0);
    EXPECT_EQ(grid.cell_x(6), 304.0);
    EXPECT_EQ(grid.cell_y(0), 55.0);
    EXPECT_EQ(grid.cell_y(3), 55.0);
    EXPECT_EQ(grid.cell_y(4), 125.0);
    EXPECT_EQ(grid.cell_y(11), 195.0);
}

TEST_F(Grid, SlotIsTheIndexWithoutADrag) {
    EXPECT_FALSE(grid.dragging());
    EXPECT_EQ(grid.origin(), -1);
    EXPECT_EQ(grid.target(), -1);

    for (int index = 0; index < 8; ++index) {
        EXPECT_EQ(grid.slot(index), index);
    }
}

TEST_F(Grid, GrabbedStartsADragInPlace) {
    grid.grabbed(2, grid.cell_x(2) + 20.0, grid.cell_y(2) + 30.0);

    EXPECT_TRUE(grid.dragging());
    EXPECT_EQ(grid.origin(), 2);
    EXPECT_EQ(grid.target(), 2);
    EXPECT_EQ(grid.carry_x(), 0.0);
    EXPECT_EQ(grid.carry_y(), 0.0);

    for (int index = 0; index < 6; ++index) {
        EXPECT_EQ(grid.slot(index), index) << "a drag that has not moved shifted " << index;
    }
}

TEST_F(Grid, DragForwardShiftsTheItemsBetweenBack) {
    grid.grabbed(1, grid.cell_x(1) + 20.0, grid.cell_y(1) + 20.0);
    grid.carried(8, grid.cell_x(4) + 20.0, grid.cell_y(4) + 20.0);

    ASSERT_EQ(grid.target(), 4);

    const std::vector<int> expected{0, 1, 1, 2, 3, 5, 6, 7};

    for (int index = 0; index < 8; ++index) {
        EXPECT_EQ(grid.slot(index), expected[static_cast<size_t>(index)]) << "item " << index;
    }
}

TEST_F(Grid, DragBackwardShiftsTheItemsBetweenOn) {
    grid.grabbed(5, grid.cell_x(5) + 20.0, grid.cell_y(5) + 20.0);
    grid.carried(8, grid.cell_x(2) + 20.0, grid.cell_y(2) + 20.0);

    ASSERT_EQ(grid.target(), 2);

    const std::vector<int> expected{0, 1, 3, 4, 5, 5, 6, 7};

    for (int index = 0; index < 8; ++index) {
        EXPECT_EQ(grid.slot(index), expected[static_cast<size_t>(index)]) << "item " << index;
    }
}

TEST_F(Grid, CarryFollowsThePointerFromWhereItWasHeld) {
    grid.grabbed(0, grid.cell_x(0) + 20.0, grid.cell_y(0) + 30.0);
    grid.carried(8, grid.cell_x(6) + 25.0, grid.cell_y(6) + 33.0);

    EXPECT_EQ(grid.target(), 6);
    EXPECT_EQ(grid.carry_x(), (2 * 97.0) + 5.0);
    EXPECT_EQ(grid.carry_y(), 70.0 + 3.0);
}

TEST_F(Grid, TargetIsClampedToTheColumns) {
    grid.grabbed(0, grid.cell_x(0) + 1.0, grid.cell_y(0) + 1.0);
    grid.carried(8, 2000.0, grid.cell_y(0) + 1.0);

    EXPECT_EQ(grid.target(), 3) << "a pointer right of the grid did not land on the last column";

    grid.carried(8, -500.0, grid.cell_y(4) + 1.0);

    EXPECT_EQ(grid.target(), 4) << "a pointer left of the grid did not land on the first column";
}

TEST_F(Grid, TargetIsClampedToTheItems) {
    grid.grabbed(0, grid.cell_x(0) + 1.0, grid.cell_y(0) + 1.0);
    grid.carried(6, grid.cell_x(3) + 1.0, 2000.0);

    EXPECT_EQ(grid.target(), 5);

    grid.carried(6, grid.cell_x(3) + 1.0, -2000.0);

    EXPECT_EQ(grid.target(), 0);
}

TEST_F(Grid, GutterBelongsToTheCellBeforeIt) {
    grid.grabbed(0, grid.cell_x(0) + 1.0, grid.cell_y(0) + 1.0);
    grid.carried(8, grid.cell_x(0) + 87.0 + 5.0, grid.cell_y(4) + 60.0 + 5.0);

    EXPECT_EQ(grid.target(), 4);
}

TEST_F(Grid, CarryWithoutItemsTargetsTheFirst) {
    grid.grabbed(0, grid.cell_x(0), grid.cell_y(0));
    grid.carried(0, grid.cell_x(3), grid.cell_y(4));

    EXPECT_EQ(grid.target(), 0);
}

TEST_F(Grid, LandedClearsTheDrag) {
    grid.grabbed(1, grid.cell_x(1), grid.cell_y(1));
    grid.carried(8, grid.cell_x(5), grid.cell_y(5));
    grid.landed();

    EXPECT_FALSE(grid.dragging());
    EXPECT_EQ(grid.origin(), -1);
    EXPECT_EQ(grid.target(), -1);
    EXPECT_EQ(grid.carry_x(), 0.0);
    EXPECT_EQ(grid.carry_y(), 0.0);
    EXPECT_EQ(grid.slot(3), 3);
}

TEST_F(Grid, OffsetsWithoutADragAreTheSlides) {
    std::vector<Slid> cards{{1, 2}, {3, 4}, {0, 0}};
    std::vector<Slid *> held;

    held.reserve(cards.size());

    for (Slid &card : cards) {
        held.push_back(&card);
    }

    const std::vector<BLPoint> where = grid.offsets(held);

    ASSERT_EQ(where.size(), 3U);
    EXPECT_EQ(where[0].x, 1.0);
    EXPECT_EQ(where[0].y, 2.0);
    EXPECT_EQ(where[1].x, 3.0);
    EXPECT_EQ(where[1].y, 4.0);
    EXPECT_EQ(where[2].x, 0.0);
}

TEST_F(Grid, OffsetsPutEachItemUnderTheIndexItWillTake) {
    std::vector<Slid> cards{{0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4}};
    std::vector<Slid *> held;

    held.reserve(cards.size());

    for (Slid &card : cards) {
        held.push_back(&card);
    }

    grid.grabbed(0, grid.cell_x(0) + 10.0, grid.cell_y(0) + 10.0);
    grid.carried(5, grid.cell_x(2) + 14.0, grid.cell_y(2) + 17.0);

    ASSERT_EQ(grid.target(), 2);

    const std::vector<BLPoint> where = grid.offsets(held);

    ASSERT_EQ(where.size(), 5U);
    EXPECT_EQ(where[0].x, 1.0) << "the item moving into 0 is the one that was at 1";
    EXPECT_EQ(where[1].x, 2.0);
    EXPECT_EQ(where[2].x, 4.0) << "the carried item is not drawn at the pointer";
    EXPECT_EQ(where[2].y, 7.0);
    EXPECT_EQ(where[3].x, 3.0);
    EXPECT_EQ(where[4].y, 4.0);
}

TEST_F(Grid, OffsetsOfABackwardDropAcrossRows) {
    std::vector<Slid> cards(6);
    std::vector<Slid *> held;

    held.reserve(cards.size());

    for (Slid &card : cards) {
        held.push_back(&card);
    }

    cards[5].x = 2.0;
    grid.grabbed(5, grid.cell_x(5), grid.cell_y(5));
    grid.carried(6, grid.cell_x(1), grid.cell_y(1));

    ASSERT_EQ(grid.target(), 1);

    const std::vector<BLPoint> where = grid.offsets(held);

    EXPECT_EQ(where[1].x, 2.0) << "the carried item is not where it was let go";
    EXPECT_EQ(where[1].y, 0.0);
}

TEST(ReorderGrid, CarriedWakesThePage) {
    tests::Canvas canvas;
    ttk::Widget *page = canvas.root().content()->add(std::make_unique<ttk::Widget>());

    canvas.frame();

    ttk::ReorderGrid grid(page, SMALL);

    grid.place({0, 0, 400, 240});

    ASSERT_FALSE(canvas.root().busy());

    grid.grabbed(0, 20, 20);
    grid.carried(4, 120, 20);

    EXPECT_TRUE(canvas.root().busy());
}
}
