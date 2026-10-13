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
#include "ttk/toolkit/layout/Box.h"

namespace {
using tests::Block;
using tests::boxed;
using Place = ttk::Box::Place;

ttk::Typeface &type() {
    return tests::fonts();
}

std::vector<Block *> fill(ttk::Box &box, const std::vector<BLSize> &sizes) {
    std::vector<Block *> made;

    made.reserve(sizes.size());

    for (const BLSize &size : sizes) {
        made.push_back(box.append(std::make_unique<Block>(size.w, size.h)));
    }

    return made;
}

TEST(Box, FactoriesMakeTheirFlow) {
    auto row = ttk::Box::row();
    auto column = ttk::Box::column();

    fill(*row, {{30, 10}, {40, 20}});
    fill(*column, {{30, 10}, {40, 20}});

    EXPECT_EQ(row->natural_width(type()), 70.0);
    EXPECT_EQ(column->natural_width(type()), 40.0);
    EXPECT_EQ(row->natural_height(type(), 70.0), 20.0);
    EXPECT_EQ(column->natural_height(type(), 40.0), 30.0);
}

TEST(Box, SettersReturnTheBox) {
    auto box = ttk::Box::row();
    ttk::Box *raw = box.get();

    EXPECT_EQ(box->spacing(4.0), raw);
    EXPECT_EQ(box->pad(1.0), raw);
    EXPECT_EQ(box->pad(1.0, 2.0), raw);
    EXPECT_EQ(box->pad(1.0, 2.0, 3.0, 4.0), raw);
    EXPECT_EQ(box->align(Place::End), raw);
    EXPECT_EQ(box->cross(Place::Centre), raw);
    EXPECT_EQ(box->grow(2.5), raw);
    EXPECT_EQ(box->stretch, 2.5);
}

TEST(Box, EmptyRowWantsOnlyItsPadding) {
    auto box = ttk::Box::row();

    box->spacing(10.0)->pad(1.0, 2.0, 3.0, 4.0);

    EXPECT_EQ(box->natural_width(type()), 4.0);
    EXPECT_EQ(box->natural_height(type(), 100.0), 6.0);
}

TEST(Box, EmptyColumnWantsOnlyItsPadding) {
    auto box = ttk::Box::column();

    box->spacing(10.0)->pad(5.0);

    EXPECT_EQ(box->natural_width(type()), 10.0);
    EXPECT_EQ(box->natural_height(type(), 100.0), 10.0);
}

TEST(Box, OneChildHasNoSpacing) {
    auto row = ttk::Box::row();

    row->spacing(25.0);
    fill(*row, {{30, 10}});

    EXPECT_EQ(row->natural_width(type()), 30.0);

    auto column = ttk::Box::column();

    column->spacing(25.0);
    fill(*column, {{30, 10}});

    EXPECT_EQ(column->natural_height(type(), 30.0), 10.0);
}

TEST(Box, RowWidthSumsChildrenSpacingAndPadding) {
    auto box = ttk::Box::row();

    box->spacing(5.0)->pad(1.0, 2.0, 3.0, 4.0);
    fill(*box, {{30, 10}, {40, 10}, {50, 10}});

    EXPECT_EQ(box->natural_width(type()), 30.0 + 40.0 + 50.0 + 10.0 + 1.0 + 3.0);
}

TEST(Box, ColumnWidthIsTheWidestPlusPadding) {
    auto box = ttk::Box::column();

    box->spacing(5.0)->pad(1.0, 2.0, 3.0, 4.0);
    fill(*box, {{30, 10}, {70, 10}, {50, 10}});

    EXPECT_EQ(box->natural_width(type()), 74.0);
}

TEST(Box, ColumnHeightSumsChildrenSpacingAndPadding) {
    auto box = ttk::Box::column();

    box->spacing(5.0)->pad(1.0, 2.0, 3.0, 4.0);
    fill(*box, {{30, 10}, {30, 20}, {30, 30}});

    EXPECT_EQ(box->natural_height(type(), 100.0), 60.0 + 10.0 + 6.0);
}

TEST(Box, RowHeightIsTheTallestPlusPadding) {
    auto box = ttk::Box::row();

    box->spacing(5.0)->pad(1.0, 2.0, 3.0, 4.0);
    fill(*box, {{30, 10}, {30, 45}, {30, 30}});

    EXPECT_EQ(box->natural_height(type(), 200.0), 51.0);
}

TEST(Box, HiddenChildrenTakeNoRoomAndNoSpacing) {
    auto box = ttk::Box::row();

    box->spacing(10.0);

    const std::vector<Block *> blocks = fill(*box, {{30, 10}, {40, 90}, {50, 10}});

    blocks[1]->set_visible(false);

    EXPECT_EQ(box->natural_width(type()), 30.0 + 50.0 + 10.0);
    EXPECT_EQ(box->natural_height(type(), 200.0), 10.0) << "a hidden child still set the row height";

    blocks[0]->set_visible(false);
    blocks[2]->set_visible(false);

    EXPECT_EQ(box->natural_width(type()), 0.0);
}

TEST(Box, FixedSizesOverrideWhatTheChildrenWant) {
    auto box = ttk::Box::row();

    fill(*box, {{30, 10}, {40, 20}});
    box->fixedWidth = 7.0;
    box->fixedHeight = 9.0;

    EXPECT_EQ(box->natural_width(type()), 7.0);
    EXPECT_EQ(box->natural_height(type(), 500.0), 9.0);
}

TEST(Box, HeightIsRaisedToTheFloor) {
    auto box = ttk::Box::column();

    fill(*box, {{30, 10}});
    box->minHeight = 50.0;

    EXPECT_EQ(box->natural_height(type(), 30.0), 50.0);

    box->minHeight = 5.0;

    EXPECT_EQ(box->natural_height(type(), 30.0), 10.0);
}

TEST(Box, ChildFloorsCountTowardTheNaturalWidth) {
    auto box = ttk::Box::row();

    const std::vector<Block *> blocks = fill(*box, {{30, 10}, {40, 10}});

    blocks[0]->minWidth = 80.0;

    EXPECT_EQ(box->natural_width(type()), 120.0);
}

TEST(Box, ColumnMeasuresChildrenAtTheInnerWidth) {
    auto box = ttk::Box::column();

    box->pad(10.0, 0.0);

    Block *flowing = box->append(std::make_unique<Block>(30.0));

    flowing->area = 1600.0;

    EXPECT_EQ(box->natural_height(type(), 100.0), 20.0) << "measured at the outer width, not inside the padding";
}

TEST(Box, ColumnMeasuresAFixedWidthChildAtItsOwnWidth) {
    auto box = ttk::Box::column();

    Block *flowing = box->append(std::make_unique<Block>(30.0));

    flowing->area = 1600.0;
    flowing->fixedWidth = 40.0;

    EXPECT_EQ(box->natural_height(type(), 400.0), 40.0);
}

TEST(Box, RowMeasuresChildrenAtTheWidthTheyAreGiven) {
    auto box = ttk::Box::row();

    Block *one = box->append(std::make_unique<Block>(20.0));
    Block *two = box->append(std::make_unique<Block>(20.0));

    one->area = 1000.0;
    two->area = 3000.0;
    one->stretch = 1.0;
    two->stretch = 1.0;

    EXPECT_EQ(box->natural_height(type(), 240.0), 3000.0 / 120.0) << "the spare width was not shared before measuring";
}

TEST(Box, RowPacksChildrenAtTheStartWithSpacing) {
    ttk::Box box(ttk::Box::Flow::Row);

    box.spacing(5.0);

    const std::vector<Block *> blocks = fill(box, {{30, 10}, {40, 20}, {50, 30}});

    box.place({10, 20, 300, 60}, type());

    EXPECT_TRUE(boxed(blocks[0]->box(), {10, 20, 30, 60}));
    EXPECT_TRUE(boxed(blocks[1]->box(), {45, 20, 40, 60}));
    EXPECT_TRUE(boxed(blocks[2]->box(), {90, 20, 50, 60}));
}

TEST(Box, ColumnPacksChildrenAtTheTopWithSpacing) {
    ttk::Box box(ttk::Box::Flow::Column);

    box.spacing(4.0);

    const std::vector<Block *> blocks = fill(box, {{30, 10}, {40, 20}, {50, 30}});

    box.place({10, 20, 100, 300}, type());

    EXPECT_TRUE(boxed(blocks[0]->box(), {10, 20, 100, 10}));
    EXPECT_TRUE(boxed(blocks[1]->box(), {10, 34, 100, 20}));
    EXPECT_TRUE(boxed(blocks[2]->box(), {10, 58, 100, 30}));
}

TEST(Box, PaddingInsetsTheChildren) {
    auto row = ttk::Box::row();

    row->pad(1.0, 2.0, 3.0, 4.0);

    Block *inside = fill(*row, {{30, 10}})[0];

    row->place({100, 100, 200, 50}, type());

    EXPECT_TRUE(boxed(inside->box(), {101, 102, 30, 44}));

    auto column = ttk::Box::column();

    column->pad(6.0, 8.0);

    Block *stacked = fill(*column, {{30, 10}})[0];

    column->place({0, 0, 100, 100}, type());

    EXPECT_TRUE(boxed(stacked->box(), {6, 8, 88, 10}));
}

TEST(Box, SpareRoomIsSharedByStretch) {
    auto box = ttk::Box::row();
    const std::vector<Block *> blocks = fill(*box, {{20, 10}, {20, 10}, {20, 10}});

    blocks[0]->stretch = 1.0;
    blocks[2]->stretch = 3.0;
    box->place({0, 0, 160, 10}, type());

    EXPECT_TRUE(boxed(blocks[0]->box(), {0, 0, 45, 10}));
    EXPECT_TRUE(boxed(blocks[1]->box(), {45, 0, 20, 10}));
    EXPECT_TRUE(boxed(blocks[2]->box(), {65, 0, 95, 10}));
}

TEST(Box, StretchingColumnFillsItsHeight) {
    auto box = ttk::Box::column();

    box->spacing(10.0);

    const std::vector<Block *> blocks = fill(*box, {{20, 10}, {20, 30}});

    blocks[1]->stretch = 1.0;
    box->place({0, 0, 50, 200}, type());

    EXPECT_TRUE(boxed(blocks[0]->box(), {0, 0, 50, 10}));
    EXPECT_TRUE(boxed(blocks[1]->box(), {0, 20, 50, 180}));
}

TEST(Box, AlignPlacesSpareRoomAlongTheAxis) {
    struct Case {
        Place where;
        double first;
    };

    for (const auto [where, first] : {
                 Case{Place::Fill, 0.0},
                 Case{Place::Start, 0.0},
                 Case{Place::Centre, 35.0},
                 Case{Place::End, 70.0},
         }) {
        auto box = ttk::Box::row();

        box->spacing(10.0)->align(where);

        const std::vector<Block *> blocks = fill(*box, {{40, 10}, {80, 10}});

        box->place({0, 0, 200, 10}, type());

        EXPECT_EQ(blocks[0]->box().x, first) << "align " << static_cast<int>(where);
        EXPECT_EQ(blocks[1]->box().x, first + 50.0) << "align " << static_cast<int>(where);
    }
}

TEST(Box, AlignAppliesToAColumn) {
    auto box = ttk::Box::column();

    box->align(Place::End);

    Block *only = fill(*box, {{40, 30}})[0];

    box->place({0, 0, 40, 100}, type());

    EXPECT_TRUE(boxed(only->box(), {0, 70, 40, 30}));
}

TEST(Box, AlignIsIgnoredOnceAChildStretches) {
    auto box = ttk::Box::row();

    box->align(Place::End);

    const std::vector<Block *> blocks = fill(*box, {{40, 10}, {40, 10}});

    blocks[1]->stretch = 1.0;
    box->place({0, 0, 200, 10}, type());

    EXPECT_EQ(blocks[0]->box().x, 0.0);
    EXPECT_EQ(blocks[1]->box().w, 160.0);
}

TEST(Box, CrossPlacesChildrenAcrossARow) {
    struct Case {
        Place where;
        double y;
        double h;
    };

    for (const auto [where, y, h] : {
                 Case{Place::Fill, 0.0, 100.0},
                 Case{Place::Start, 0.0, 30.0},
                 Case{Place::Centre, 35.0, 30.0},
                 Case{Place::End, 70.0, 30.0},
         }) {
        auto box = ttk::Box::row();

        box->cross(where);

        Block *only = fill(*box, {{40, 30}})[0];

        box->place({0, 0, 200, 100}, type());

        EXPECT_TRUE(boxed(only->box(), {0, y, 40, h})) << "cross " << static_cast<int>(where);
    }
}

TEST(Box, CrossPlacesChildrenAcrossAColumn) {
    struct Case {
        Place where;
        double x;
        double w;
    };

    for (const auto [where, x, w] : {
                 Case{Place::Fill, 0.0, 100.0},
                 Case{Place::Start, 0.0, 40.0},
                 Case{Place::Centre, 30.0, 40.0},
                 Case{Place::End, 60.0, 40.0},
         }) {
        auto box = ttk::Box::column();

        box->cross(where);

        Block *only = fill(*box, {{40, 30}})[0];

        box->place({0, 0, 100, 200}, type());

        EXPECT_TRUE(boxed(only->box(), {x, 0, w, 30})) << "cross " << static_cast<int>(where);
    }
}

TEST(Box, CrossCapsTheNaturalExtentAtTheInnerExtent) {
    auto row = ttk::Box::row();

    row->cross(Place::Centre)->pad(0.0, 5.0);

    Block *tall = fill(*row, {{40, 300}})[0];

    row->place({0, 0, 200, 100}, type());

    EXPECT_TRUE(boxed(tall->box(), {0, 5, 40, 90}));

    auto column = ttk::Box::column();

    column->cross(Place::End);

    Block *wide = fill(*column, {{300, 20}})[0];

    column->place({0, 0, 100, 200}, type());

    EXPECT_TRUE(boxed(wide->box(), {0, 0, 100, 20}));
}

TEST(Box, FixedCrossSizeOverridesFillAndIsNotCapped) {
    auto row = ttk::Box::row();
    Block *tall = fill(*row, {{40, 10}})[0];

    tall->fixedHeight = 150.0;
    row->place({0, 0, 200, 100}, type());

    EXPECT_TRUE(boxed(tall->box(), {0, 0, 40, 150}));

    row->cross(Place::End);
    row->place({0, 0, 200, 100}, type());

    EXPECT_TRUE(boxed(tall->box(), {0, -50, 40, 150}));

    auto column = ttk::Box::column();
    Block *narrow = fill(*column, {{40, 10}})[0];

    narrow->fixedWidth = 30.0;
    column->cross(Place::Centre);
    column->place({0, 0, 100, 100}, type());

    EXPECT_TRUE(boxed(narrow->box(), {35, 0, 30, 10}));
}

TEST(Box, CrossMeasuresARowChildAtItsShare) {
    auto row = ttk::Box::row();

    row->cross(Place::Start);

    Block *flowing = fill(*row, {{20, 0}})[0];

    flowing->area = 2000.0;
    flowing->stretch = 1.0;
    row->place({0, 0, 100, 100}, type());

    EXPECT_TRUE(boxed(flowing->box(), {0, 0, 100, 20}));
}

TEST(Box, ShortfallShrinksEveryChildByItsSizeWhenNoneStretch) {
    auto box = ttk::Box::row();
    const std::vector<Block *> blocks = fill(*box, {{100, 10}, {300, 10}});

    box->place({0, 0, 200, 10}, type());

    EXPECT_EQ(blocks[0]->box().w, 50.0);
    EXPECT_EQ(blocks[1]->box().w, 150.0);
    EXPECT_EQ(blocks[1]->box().x, 50.0);
}

TEST(Box, ShortfallComesFromTheStretchingChildren) {
    auto box = ttk::Box::row();
    const std::vector<Block *> blocks = fill(*box, {{100, 10}, {100, 10}, {100, 10}});

    blocks[1]->stretch = 1.0;
    blocks[2]->stretch = 3.0;
    box->place({0, 0, 260, 10}, type());

    EXPECT_EQ(blocks[0]->box().w, 100.0) << "a child that does not stretch gave up room";
    EXPECT_EQ(blocks[1]->box().w, 90.0);
    EXPECT_EQ(blocks[2]->box().w, 70.0);
}

TEST(Box, ShortfallNeverShrinksBelowTheFloor) {
    auto box = ttk::Box::row();
    const std::vector<Block *> blocks = fill(*box, {{100, 10}, {100, 10}});

    blocks[0]->minWidth = 90.0;
    box->place({0, 0, 100, 10}, type());

    EXPECT_EQ(blocks[0]->box().w, 90.0);
    EXPECT_EQ(blocks[1]->box().w, 50.0);

    blocks[0]->stretch = 1.0;
    blocks[1]->stretch = 1.0;
    blocks[1]->minWidth = 70.0;
    box->place({0, 0, 100, 10}, type());

    EXPECT_EQ(blocks[0]->box().w, 90.0);
    EXPECT_EQ(blocks[1]->box().w, 70.0);
}

TEST(Box, FloorsWiderThanTheRowOverflowIt) {
    auto box = ttk::Box::row();

    box->spacing(10.0);

    const std::vector<Block *> blocks = fill(*box, {{100, 10}, {100, 10}});

    blocks[0]->minWidth = 80.0;
    blocks[1]->minWidth = 80.0;
    box->place({0, 0, 100, 10}, type());

    EXPECT_EQ(blocks[0]->box().w, 80.0);
    EXPECT_EQ(blocks[1]->box().x, 90.0);
    EXPECT_GT(blocks[1]->box().x + blocks[1]->box().w, 100.0) << "floors were squeezed instead of overflowing";
}

TEST(Box, ColumnShortfallShrinksHeights) {
    auto box = ttk::Box::column();
    const std::vector<Block *> blocks = fill(*box, {{10, 100}, {10, 100}});

    blocks[0]->minHeight = 70.0;
    box->place({0, 0, 10, 100}, type());

    EXPECT_EQ(blocks[0]->box().h, 70.0);
    EXPECT_EQ(blocks[1]->box().h, 50.0);
    EXPECT_EQ(blocks[1]->box().y, 70.0);
}

TEST(Box, ZeroSizedBoxGivesItsChildrenNoRoom) {
    auto box = ttk::Box::row();

    box->pad(10.0);

    const std::vector<Block *> blocks = fill(*box, {{30, 10}, {40, 10}});

    box->place({5, 5, 0, 0}, type());

    for (const Block *block : blocks) {
        EXPECT_EQ(block->box().w, 0.0);
        EXPECT_EQ(block->box().h, 0.0) << "padding wider than the box gave a negative inner height";
        EXPECT_EQ(block->box().y, 15.0);
    }
}

TEST(Box, ArrangeWithoutChildrenDoesNothing) {
    auto box = ttk::Box::column();

    box->spacing(10.0)->pad(4.0)->align(Place::Centre)->cross(Place::End);
    box->place({0, 0, 100, 100}, type());

    EXPECT_TRUE(box->children().empty());
    EXPECT_TRUE(boxed(box->box(), {0, 0, 100, 100}));
}

TEST(Box, HiddenChildIsNotPlaced) {
    auto box = ttk::Box::row();

    box->spacing(10.0);

    const std::vector<Block *> blocks = fill(*box, {{30, 10}, {30, 10}, {30, 10}});

    blocks[1]->set_visible(false);
    box->place({0, 0, 200, 10}, type());

    EXPECT_TRUE(boxed(blocks[1]->box(), {0, 0, 0, 0}));
    EXPECT_EQ(blocks[2]->box().x, 40.0);
}

TEST(Box, ErasingAChildClosesItsGap) {
    auto box = ttk::Box::row();

    box->spacing(10.0);

    const std::vector<Block *> blocks = fill(*box, {{30, 10}, {30, 10}, {30, 10}});

    box->place({0, 0, 200, 10}, type());
    box->erase(blocks[1]);
    box->place({0, 0, 200, 10}, type());

    ASSERT_EQ(box->children().size(), 2U);
    EXPECT_EQ(blocks[2]->box().x, 40.0);
    EXPECT_EQ(box->natural_width(type()), 70.0);
}

TEST(Box, AddedChildGoesLast) {
    auto box = ttk::Box::column();

    box->spacing(5.0);

    const std::vector<Block *> blocks = fill(*box, {{30, 10}});

    box->place({0, 0, 50, 100}, type());

    Block *late = box->append(std::make_unique<Block>(30.0, 20.0));

    box->place({0, 0, 50, 100}, type());

    EXPECT_TRUE(boxed(blocks[0]->box(), {0, 0, 50, 10}));
    EXPECT_TRUE(boxed(late->box(), {0, 15, 50, 20}));
}

TEST(Box, NestedBoxesPlaceTheirChildrenInsideTheirOwnBox) {
    auto outer = ttk::Box::column();

    outer->pad(10.0)->spacing(10.0);

    ttk::Box *inner = outer->append(ttk::Box::row());

    inner->spacing(5.0)->pad(2.0);

    const std::vector<Block *> blocks = fill(*inner, {{20, 30}, {40, 10}});

    outer->place({0, 0, 200, 200}, type());

    EXPECT_TRUE(boxed(inner->box(), {10, 10, 180, 34}));
    EXPECT_TRUE(boxed(blocks[0]->box(), {12, 12, 20, 30}));
    EXPECT_TRUE(boxed(blocks[1]->box(), {37, 12, 40, 30}));
    EXPECT_EQ(outer->natural_height(type(), 200.0), 54.0);
}

TEST(Box, MeasuredHeightIsTheHeightArrangeUses) {
    auto box = ttk::Box::column();

    box->spacing(6.0)->pad(3.0);

    const std::vector<Block *> blocks = fill(*box, {{30, 10}, {30, 0}, {30, 15}});

    blocks[1]->area = 900.0;

    const double tall = box->natural_height(type(), 96.0);

    box->place({0, 0, 96, tall}, type());

    EXPECT_EQ(blocks[2]->box().y + blocks[2]->box().h + 3.0, tall);
    EXPECT_EQ(blocks[1]->box().h, 10.0);
}

TEST(Box, PaintsEveryChildInItsBox) {
    tests::Canvas canvas(200, 100);
    auto box = ttk::Box::row();

    box->spacing(20.0)->pad(10.0);

    const std::vector<Block *> blocks = fill(*box, {{40, 10}, {40, 10}});

    blocks[0]->colour = 0xffff0000U;
    blocks[1]->colour = 0xff00ff00U;
    canvas.root().content()->add(std::move(box));

    EXPECT_GT(canvas.frame(), 0U);
    EXPECT_EQ(canvas.at(10, 10), 0xffff0000U);
    EXPECT_EQ(canvas.at(49, 89), 0xffff0000U);
    EXPECT_EQ(canvas.at(70, 50), 0xff00ff00U);
    EXPECT_EQ(canvas.at(60, 50), ttk::Theme::palette().background.value) << "the spacing was painted over";
    EXPECT_EQ(canvas.at(5, 50), ttk::Theme::palette().background.value) << "the padding was painted over";
    EXPECT_EQ(canvas.at(150, 50), ttk::Theme::palette().background.value);
    EXPECT_TRUE(canvas.matches_full());
}

TEST(Box, AddingAChildRepaintsAfterTheRelayout) {
    tests::Canvas canvas(200, 100);
    ttk::Box *box = canvas.root().content()->append(ttk::Box::row());

    fill(*box, {{40, 10}});
    canvas.frame();

    Block *late = box->append(std::make_unique<Block>(40.0, 10.0, 0xff0000ffU));

    canvas.root().relayout();
    canvas.frame();

    EXPECT_TRUE(boxed(late->box(), {40, 0, 40, 100}));
    EXPECT_EQ(canvas.at(60, 50), 0xff0000ffU);
    EXPECT_TRUE(canvas.matches_full());
}

TEST(Box, ErasingAChildRepaintsWhereItWas) {
    tests::Canvas canvas(200, 100);
    ttk::Box *box = canvas.root().content()->append(ttk::Box::row());
    const std::vector<Block *> blocks = fill(*box, {{40, 10}, {40, 10}});

    canvas.frame();
    box->erase(blocks[1]);
    canvas.root().relayout();
    canvas.frame();

    EXPECT_EQ(canvas.at(60, 50), ttk::Theme::palette().background.value);
    EXPECT_TRUE(canvas.matches_full());
}

TEST(Box, ChildRepaintTouchesOnlyThatChild) {
    tests::Canvas canvas(200, 100);
    ttk::Box *box = canvas.root().content()->append(ttk::Box::row());
    const std::vector<Block *> blocks = fill(*box, {{40, 10}, {40, 10}});

    canvas.frame();

    const int before = blocks[0]->painted;

    blocks[1]->colour = 0xff123456U;
    blocks[1]->invalidate();

    EXPECT_EQ(canvas.frame(), 40U * 100U);
    EXPECT_EQ(blocks[0]->painted, before) << "a neighbour outside the damage was painted";
    EXPECT_EQ(canvas.at(60, 50), 0xff123456U);
    EXPECT_TRUE(canvas.matches_full());
}
}
