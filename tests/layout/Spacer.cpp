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
#include <vector>

#include <gtest/gtest.h>

#include "Blocks.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/layout/Box.h"
#include "ttk/toolkit/layout/Spacer.h"

namespace {
using tests::Block;
using tests::boxed;

ttk::Typeface &type() {
    return tests::fonts();
}

TEST(Spacer, StretchesByOneByDefault) {
    const ttk::Spacer spacer;

    EXPECT_EQ(spacer.stretch, 1.0);
}

TEST(Spacer, TakesItsWeightAsStretch) {
    const ttk::Spacer heavy(2.5);
    const ttk::Spacer fixed(0.0);

    EXPECT_EQ(heavy.stretch, 2.5);
    EXPECT_EQ(fixed.stretch, 0.0);
}

TEST(Spacer, WantsNothingUnlessFixed) {
    ttk::Spacer spacer;

    EXPECT_EQ(spacer.natural_width(type()), 0.0);
    EXPECT_EQ(spacer.natural_height(type(), 500.0), 0.0);

    spacer.fixedWidth = 12.0;
    spacer.fixedHeight = 7.0;

    EXPECT_EQ(spacer.natural_width(type()), 12.0);
    EXPECT_EQ(spacer.natural_height(type(), 500.0), 7.0);
}

TEST(Spacer, FloorsStillApplyToTheWantedSize) {
    ttk::Spacer spacer;

    spacer.minWidth = 9.0;
    spacer.minHeight = 4.0;

    EXPECT_EQ(spacer.wanted_width(type()), 9.0);
    EXPECT_EQ(spacer.wanted_height(type(), 100.0), 4.0);
}

TEST(Spacer, PairOfSpacersCentresARow) {
    auto row = ttk::Box::row();

    row->append(std::make_unique<ttk::Spacer>());

    Block *middle = row->append(std::make_unique<Block>(40.0, 10.0));

    row->append(std::make_unique<ttk::Spacer>());
    row->place({0, 0, 200, 20}, type());

    EXPECT_TRUE(boxed(middle->box(), {80, 0, 40, 20}));
}

TEST(Spacer, PushesTheRestToTheEnd) {
    auto row = ttk::Box::row();

    row->spacing(10.0);

    Block *first = row->append(std::make_unique<Block>(30.0, 10.0));

    row->append(std::make_unique<ttk::Spacer>());

    Block *last = row->append(std::make_unique<Block>(30.0, 10.0));

    row->place({0, 0, 200, 20}, type());

    EXPECT_EQ(first->box().x, 0.0);
    EXPECT_EQ(last->box().x, 170.0);
}

TEST(Spacer, WeightsShareTheSpareRoom) {
    auto row = ttk::Box::row();
    const auto *light = row->append(std::make_unique<ttk::Spacer>(1.0));
    const auto *heavy = row->append(std::make_unique<ttk::Spacer>(3.0));

    row->place({0, 0, 200, 20}, type());

    EXPECT_EQ(light->box().w, 50.0);
    EXPECT_EQ(heavy->box().w, 150.0);
}

TEST(Spacer, ZeroWeightIsAFixedGap) {
    auto column = ttk::Box::column();

    column->append(std::make_unique<Block>(10.0, 10.0));
    column->append(std::make_unique<ttk::Spacer>(0.0))->fixedHeight = 25.0;

    Block *after = column->append(std::make_unique<Block>(10.0, 10.0));

    column->place({0, 0, 50, 300}, type());

    EXPECT_EQ(after->box().y, 35.0);
    EXPECT_EQ(column->natural_height(type(), 50.0), 45.0);
}

TEST(Spacer, IsNotHitByThePointer) {
    ttk::Spacer spacer;

    spacer.place({0, 0, 50, 50}, type());

    EXPECT_EQ(spacer.at(25, 25), nullptr);
}

TEST(Spacer, PaintsNothing) {
    tests::Canvas canvas(80, 40);

    canvas.root().content()->add(std::make_unique<ttk::Spacer>());
    canvas.frame();

    const std::uint32_t background = ttk::Theme::palette().background.value;

    for (const std::uint32_t pixel : canvas.pixels()) {
        ASSERT_EQ(pixel, background);
    }
}
}
