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
#include <utility>
#include <vector>

#include <blend2d/blend2d.h>
#include <gtest/gtest.h>

#include "Blocks.h"
#include "support/Canvas.h"
#include "ttk/toolkit/controls/Card.h"
#include "ttk/toolkit/layout/CardGrid.h"

namespace {
using tests::boxed;

constexpr ttk::ReorderGrid::Metrics METRICS{
        .bleed = 10.0,
        .gutter = 10.0,
        .top = 5.0,
        .narrowest = 80.0,
        .row_height = 100.0,
        .step = 1.0,
};

constexpr double CELL = 87.0;
constexpr double FRAME = 1.0 / 60.0;
constexpr double SETTLED = 0.5;

// On a 400 pixel page the grid has four columns of 87, a cell every 97 across and 110 down.
double cell_x(const int index) {
    return 10.0 + ((index % 4) * 97.0);
}

double cell_y(const int index) {
    const int row = index / 4;

    return 5.0 + (row * 110.0);
}

BLPoint centre(const int index) {
    return {cell_x(index) + 43.0, cell_y(index) + 50.0};
}

class Cards : public ::testing::Test {
protected:
    void SetUp() override {
        if (!tests::fonts_loaded()) {
            GTEST_SKIP() << "no font installed";
        }

        grid = canvas.root().content()->append(std::make_unique<ttk::CardGrid>(METRICS));
        grid->reordered = [this](const int from, const int to) { moves.emplace_back(from, to); };
    }

    std::vector<ttk::Card *> deal(const int count) {
        std::vector<ttk::Card *> dealt;

        dealt.reserve(static_cast<size_t>(count));

        for (int at = 0; at < count; ++at) {
            dealt.push_back(grid->add(std::make_unique<ttk::Card>()));
        }

        canvas.frame();

        return dealt;
    }

    void press(const BLPoint &at) { canvas.root().press(ttk::Pointer{.x = at.x, .y = at.y}); }

    void move(const BLPoint &at) {
        canvas.root().motion(at.x, at.y);
        step();
    }

    void release(const BLPoint &at) {
        canvas.root().release(ttk::Pointer{.x = at.x, .y = at.y});
        step();
    }

    void step() {
        now += FRAME;
        canvas.frame_at(now);
    }

    void wait(const double seconds) {
        const double until = now + seconds;

        while (now < until) {
            step();
        }
    }

    // Picks a card up by its centre and carries it to `to`, passing the slack that starts a drag on the way.
    void carry(const int index, const BLPoint &to) {
        const BLPoint from = centre(index);

        press(from);
        move({from.x + 8.0, from.y});
        move(to);
    }

    tests::Canvas canvas;
    ttk::CardGrid *grid = nullptr;
    std::vector<std::pair<int, int>> moves;
    double now = 0.0;
};

TEST_F(Cards, AddAppendsADraggableCard) {
    auto card = std::make_unique<ttk::Card>();
    ttk::Card *raw = card.get();

    EXPECT_EQ(grid->add(std::move(card)), raw);
    ASSERT_EQ(grid->cards().size(), 1U);
    EXPECT_EQ(grid->cards()[0], raw);
    EXPECT_TRUE(raw->draggable);
    EXPECT_TRUE(raw->drag_started);
    EXPECT_TRUE(raw->drag_moved);
    EXPECT_TRUE(raw->drag_ended);
    EXPECT_EQ(raw->parent(), grid);
}

TEST_F(Cards, CardsKeepTheOrderTheyWereAdded) {
    const std::vector<ttk::Card *> dealt = deal(6);

    EXPECT_EQ(grid->cards(), dealt);
}

TEST_F(Cards, AddAsksForANewLayout) {
    deal(2);

    ASSERT_FALSE(canvas.root().dirty());

    grid->add(std::make_unique<ttk::Card>());

    EXPECT_TRUE(canvas.root().dirty());
}

TEST_F(Cards, NaturalHeightCoversTheRows) {
    EXPECT_EQ(grid->natural_height(tests::fonts(), 400.0), 0.0);

    deal(1);

    EXPECT_EQ(grid->natural_height(tests::fonts(), 400.0), 105.0);

    deal(3);

    EXPECT_EQ(grid->natural_height(tests::fonts(), 400.0), 105.0);

    deal(1);

    EXPECT_EQ(grid->natural_height(tests::fonts(), 400.0), 215.0);
    EXPECT_EQ(grid->natural_height(tests::fonts(), 200.0), 325.0) << "two columns at 200 give three rows";
}

TEST_F(Cards, CardsSitInTheirCells) {
    const std::vector<ttk::Card *> dealt = deal(6);

    for (int index = 0; index < 6; ++index) {
        const ttk::Card *card = dealt[static_cast<size_t>(index)];

        EXPECT_TRUE(boxed(card->box(), {cell_x(index), cell_y(index), CELL, 100})) << "card " << index;
        EXPECT_EQ(card->slot(), index);
    }
}

TEST_F(Cards, NarrowerGridReflowsIntoFewerColumns) {
    const std::vector<ttk::Card *> dealt = deal(3);
    auto held = std::make_unique<ttk::CardGrid>(METRICS);
    ttk::CardGrid *narrow = held.get();

    canvas.root().layer(ttk::Root::POPUPS)->add(std::move(held));

    std::vector<ttk::Card *> placed;

    placed.reserve(3);

    for (int at = 0; at < 3; ++at) {
        placed.push_back(narrow->add(std::make_unique<ttk::Card>()));
    }

    canvas.frame();
    narrow->place({0, 0, 200, 240}, tests::fonts());

    EXPECT_TRUE(boxed(placed[1]->box(), {105, 5, 85, 100}));
    EXPECT_TRUE(boxed(placed[2]->box(), {10, 115, 85, 100}));
    EXPECT_TRUE(boxed(dealt[2]->box(), {cell_x(2), cell_y(2), CELL, 100}));
}

TEST_F(Cards, ClearCardsEmptiesTheGrid) {
    deal(4);
    grid->clear_cards();

    EXPECT_TRUE(grid->cards().empty());
    EXPECT_TRUE(grid->children().empty());
    EXPECT_TRUE(canvas.root().dirty());
    EXPECT_EQ(grid->natural_height(tests::fonts(), 400.0), 0.0);

    canvas.frame();

    EXPECT_TRUE(canvas.matches_full());
}

TEST_F(Cards, SmallMotionIsNotADrag) {
    const std::vector<ttk::Card *> dealt = deal(4);

    press(centre(0));
    move({centre(0).x + 5.0, centre(0).y + 5.0});

    EXPECT_FALSE(dealt[0]->carrying());
    EXPECT_TRUE(boxed(dealt[0]->box(), {cell_x(0), cell_y(0), CELL, 100}));

    release({centre(0).x + 5.0, centre(0).y + 5.0});

    EXPECT_TRUE(moves.empty());
    EXPECT_EQ(grid->cards(), dealt);
}

TEST_F(Cards, CarriedCardFollowsThePointer) {
    const std::vector<ttk::Card *> dealt = deal(4);

    carry(0, {centre(2).x + 3.0, centre(2).y + 7.0});

    EXPECT_TRUE(dealt[0]->carrying());
    EXPECT_TRUE(boxed(dealt[0]->box(), {cell_x(0) + 197.0, cell_y(0) + 7.0, CELL, 100}));

    move({centre(1).x, centre(1).y - 20.0});

    EXPECT_TRUE(boxed(dealt[0]->box(), {cell_x(1), cell_y(1) - 20.0, CELL, 100}));
}

TEST_F(Cards, CardsBetweenShiftTowardTheOrigin) {
    const std::vector<ttk::Card *> dealt = deal(5);

    carry(0, centre(2));
    wait(SETTLED);

    EXPECT_TRUE(boxed(dealt[1]->box(), {cell_x(0), cell_y(0), CELL, 100}));
    EXPECT_TRUE(boxed(dealt[2]->box(), {cell_x(1), cell_y(1), CELL, 100}));
    EXPECT_TRUE(boxed(dealt[3]->box(), {cell_x(3), cell_y(3), CELL, 100})) << "a card past the target moved";
    EXPECT_TRUE(boxed(dealt[4]->box(), {cell_x(4), cell_y(4), CELL, 100}));
    EXPECT_TRUE(moves.empty()) << "reordered before the drop";
}

TEST_F(Cards, NeighboursSlideRatherThanJump) {
    const std::vector<ttk::Card *> dealt = deal(4);

    carry(0, centre(1));
    wait(3 * FRAME);

    EXPECT_GT(dealt[1]->box().x, cell_x(0)) << "the neighbour jumped into the gap";
    EXPECT_LT(dealt[1]->box().x, cell_x(1));
    EXPECT_TRUE(dealt[1]->sliding());

    wait(SETTLED);

    EXPECT_EQ(dealt[1]->box().x, cell_x(0));
    EXPECT_FALSE(dealt[1]->sliding());
}

TEST_F(Cards, DropForwardReorders) {
    const std::vector<ttk::Card *> dealt = deal(4);

    carry(0, centre(2));
    release(centre(2));

    ASSERT_EQ(moves.size(), 1U);
    EXPECT_EQ(moves[0], std::make_pair(0, 2));
    EXPECT_EQ(grid->cards(), (std::vector<ttk::Card *>{dealt[1], dealt[2], dealt[0], dealt[3]}));

    wait(SETTLED);

    for (int index = 0; index < 4; ++index) {
        const ttk::Card *card = grid->cards()[static_cast<size_t>(index)];

        EXPECT_TRUE(boxed(card->box(), {cell_x(index), cell_y(index), CELL, 100})) << "card " << index;
        EXPECT_EQ(card->slot(), index);
    }
}

TEST_F(Cards, DropBackwardAcrossRowsReorders) {
    const std::vector<ttk::Card *> dealt = deal(6);

    carry(5, centre(1));
    release(centre(1));

    ASSERT_EQ(moves.size(), 1U);
    EXPECT_EQ(moves[0], std::make_pair(5, 1));
    EXPECT_EQ(grid->cards(), (std::vector<ttk::Card *>{dealt[0], dealt[5], dealt[1], dealt[2], dealt[3], dealt[4]}));

    wait(SETTLED);

    EXPECT_TRUE(boxed(dealt[3]->box(), {cell_x(4), cell_y(4), CELL, 100})) << "the card did not wrap a row";
}

TEST_F(Cards, DropOnItsOwnCellDoesNotReorder) {
    const std::vector<ttk::Card *> dealt = deal(4);

    carry(1, centre(3));
    move(centre(1));
    release(centre(1));

    EXPECT_TRUE(moves.empty());
    EXPECT_EQ(grid->cards(), dealt);

    wait(SETTLED);

    EXPECT_TRUE(boxed(dealt[3]->box(), {cell_x(3), cell_y(3), CELL, 100}));
    EXPECT_TRUE(boxed(dealt[1]->box(), {cell_x(1), cell_y(1), CELL, 100}));
}

TEST_F(Cards, DropPastTheLastCardTakesTheLastIndex) {
    const std::vector<ttk::Card *> dealt = deal(3);

    carry(0, centre(7));
    release(centre(7));

    ASSERT_EQ(moves.size(), 1U);
    EXPECT_EQ(moves[0], std::make_pair(0, 2));
    EXPECT_EQ(grid->cards().back(), dealt[0]);
}

TEST_F(Cards, DroppedCardSlidesFromWhereItWasLetGo) {
    const std::vector<ttk::Card *> dealt = deal(4);
    const BLPoint letGo{centre(2).x + 20.0, centre(2).y + 15.0};

    carry(0, letGo);
    release(letGo);

    EXPECT_NEAR(dealt[0]->box().x, cell_x(2) + 20.0, 2.0) << "the drop jumped to the cell";
    EXPECT_NEAR(dealt[0]->box().y, cell_y(2) + 15.0, 2.0);
    EXPECT_TRUE(dealt[0]->sliding());

    wait(SETTLED);

    EXPECT_TRUE(boxed(dealt[0]->box(), {cell_x(2), cell_y(2), CELL, 100}));
}

TEST_F(Cards, GridIsLiveUntilEverySlideEnds) {
    deal(4);

    ASSERT_FALSE(canvas.root().busy());

    carry(0, {centre(2).x + 15.0, centre(2).y + 10.0});

    EXPECT_TRUE(canvas.root().busy());

    wait(SETTLED);

    EXPECT_TRUE(canvas.root().busy()) << "the grid slept while a card was held";

    release({centre(2).x + 15.0, centre(2).y + 10.0});

    EXPECT_TRUE(canvas.root().busy()) << "the grid slept while the dropped card slid home";

    wait(SETTLED);

    EXPECT_FALSE(canvas.root().busy()) << "the grid never settled after the drop";
    EXPECT_FALSE(grid->advance(now));
}

TEST_F(Cards, CarriedCardIsPaintedOnTop) {
    const std::vector<ttk::Card *> dealt = deal(4);

    dealt[0]->inset = true;
    carry(0, {centre(0).x + 50.0, centre(0).y});

    ASSERT_TRUE(boxed(dealt[0]->box(), {cell_x(0) + 50.0, cell_y(0), CELL, 100}));
    ASSERT_TRUE(boxed(dealt[1]->box(), {cell_x(1), cell_y(1), CELL, 100}));

    const std::uint32_t carried = canvas.at(80, 55);

    EXPECT_EQ(canvas.at(125, 55), carried) << "the card in hand went under its neighbour";
    EXPECT_NE(canvas.at(170, 55), carried);
    EXPECT_TRUE(canvas.matches_full());
}

TEST_F(Cards, DroppedCardStaysOnTopWhileItSlides) {
    const std::vector<ttk::Card *> dealt = deal(4);

    dealt[0]->inset = true;
    carry(0, {centre(1).x - 30.0, centre(1).y});
    release({centre(1).x - 30.0, centre(1).y});

    ASSERT_EQ(moves.size(), 1U);
    ASSERT_TRUE(dealt[0]->sliding());

    const BLRect at = dealt[0]->box();
    const std::uint32_t carried = canvas.at(static_cast<int>(at.x + 20.0), 55);

    EXPECT_EQ(canvas.at(static_cast<int>(at.x + at.w - 10.0), 55), carried);
    EXPECT_NE(canvas.at(static_cast<int>(at.x + at.w + 20.0), 55), carried);
}

TEST_F(Cards, EveryFrameOfADragMatchesAFullRepaint) {
    deal(6);
    press(centre(0));

    for (const BLPoint &at : {
                 BLPoint{centre(0).x + 8.0, centre(0).y},
                 centre(1),
                 centre(2),
                 BLPoint{centre(5).x + 9.0, centre(5).y - 4.0},
                 centre(3),
         }) {
        move(at);

        EXPECT_TRUE(canvas.matches_full()) << "while carried to " << at.x << "," << at.y;
    }

    release(centre(3));

    EXPECT_TRUE(canvas.matches_full()) << "on the drop";

    for (int frame = 0; frame < 20; ++frame) {
        step();

        EXPECT_TRUE(tests::nearly_full(canvas)) << "while sliding, frame " << frame;
    }
}

TEST_F(Cards, ClearingDuringADragCancelsIt) {
    deal(4);
    carry(0, centre(2));
    grid->clear_cards();

    EXPECT_EQ(canvas.root().grabbed(), nullptr);

    release(centre(2));
    wait(SETTLED);

    EXPECT_TRUE(moves.empty());
    EXPECT_TRUE(grid->cards().empty());
    EXPECT_FALSE(canvas.root().busy());
}

TEST_F(Cards, CardAddedLaterTakesTheNextCell) {
    deal(4);

    const ttk::Card *late = grid->add(std::make_unique<ttk::Card>());

    canvas.frame();

    EXPECT_TRUE(boxed(late->box(), {cell_x(4), cell_y(4), CELL, 100}));
    EXPECT_TRUE(canvas.matches_full());
}
}
