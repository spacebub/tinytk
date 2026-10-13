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

#include <blend2d/blend2d.h>
#include <gtest/gtest.h>

#include "Blocks.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/layout/Box.h"
#include "ttk/toolkit/layout/Picture.h"

namespace {
using tests::Block;
using tests::boxed;

constexpr std::uint32_t RED = 0xffff0000U;
constexpr std::uint32_t GREEN = 0xff00ff00U;
constexpr std::uint32_t BLUE = 0xff0000ffU;
constexpr std::uint32_t WHITE = 0xffffffffU;

ttk::Typeface &type() {
    return tests::fonts();
}

// Two by two, one colour per quadrant.
BLImage quadrants() {
    BLImage image(2, 2, BL_FORMAT_PRGB32);
    BLContext context(image);

    context.fill_rect(BLRectI{0, 0, 1, 1}, BLRgba32(RED));
    context.fill_rect(BLRectI{1, 0, 1, 1}, BLRgba32(GREEN));
    context.fill_rect(BLRectI{0, 1, 1, 1}, BLRgba32(BLUE));
    context.fill_rect(BLRectI{1, 1, 1, 1}, BLRgba32(WHITE));
    context.end();

    return image;
}

BLImage solid(const int width, const int height, const std::uint32_t colour) {
    BLImage image(width, height, BL_FORMAT_PRGB32);
    BLContext context(image);

    context.fill_all(BLRgba32(colour));
    context.end();

    return image;
}

TEST(Picture, DoesNotSizeItselfFromTheImage) {
    ttk::Picture picture(solid(64, 32, RED));

    EXPECT_EQ(picture.wanted_width(type()), 0.0);
    EXPECT_EQ(picture.wanted_height(type(), 100.0), 0.0);

    picture.fixedWidth = 40.0;
    picture.fixedHeight = 20.0;

    EXPECT_EQ(picture.wanted_width(type()), 40.0);
    EXPECT_EQ(picture.wanted_height(type(), 100.0), 20.0);
}

TEST(Picture, TakesItsFixedSizeInARow) {
    auto row = ttk::Box::row();

    row->cross(ttk::Box::Place::Centre);

    auto *picture = row->append(std::make_unique<ttk::Picture>(solid(8, 8, RED)));

    picture->fixedWidth = 40.0;
    picture->fixedHeight = 20.0;
    row->place({0, 0, 200, 60}, type());

    EXPECT_TRUE(boxed(picture->box(), {0, 20, 40, 20}));
}

TEST(Picture, FillsItsBoxAndNothingElse) {
    tests::Canvas canvas(160, 120);
    auto *picture = canvas.root().layer(ttk::Root::POPUPS)->append(std::make_unique<ttk::Picture>(solid(4, 4, RED)));

    picture->place({50, 60, 40, 20}, type());
    canvas.frame();

    const std::uint32_t background = ttk::Theme::palette().background.value;

    EXPECT_EQ(canvas.at(50, 60), RED);
    EXPECT_EQ(canvas.at(89, 79), RED);
    EXPECT_EQ(canvas.at(70, 70), RED);
    EXPECT_EQ(canvas.at(49, 70), background);
    EXPECT_EQ(canvas.at(90, 70), background);
    EXPECT_EQ(canvas.at(70, 59), background);
    EXPECT_EQ(canvas.at(70, 80), background);
    EXPECT_TRUE(canvas.matches_full());
}

TEST(Picture, StretchesTheImageToTheBox) {
    tests::Canvas canvas(160, 120);
    auto *picture = canvas.root().layer(ttk::Root::POPUPS)->append(std::make_unique<ttk::Picture>(quadrants()));

    picture->place({0, 0, 120, 80}, type());
    canvas.frame();

    EXPECT_EQ(canvas.at(10, 10), RED);
    EXPECT_EQ(canvas.at(110, 10), GREEN);
    EXPECT_EQ(canvas.at(10, 70), BLUE);
    EXPECT_EQ(canvas.at(110, 70), WHITE);
}

TEST(Picture, EmptyImageDrawsNothing) {
    tests::Canvas canvas(80, 60);

    canvas.root().content()->add(std::make_unique<ttk::Picture>(BLImage()));
    canvas.frame();

    const std::uint32_t background = ttk::Theme::palette().background.value;

    for (const std::uint32_t pixel : canvas.pixels()) {
        ASSERT_EQ(pixel, background);
    }
}

TEST(Picture, NeverPaintsChildren) {
    tests::Canvas canvas(80, 60);
    auto *picture = canvas.root().content()->append(std::make_unique<ttk::Picture>(solid(2, 2, RED)));
    Block *child = picture->append(std::make_unique<Block>(10.0, 10.0, GREEN));

    canvas.frame();

    EXPECT_EQ(child->painted, 0);
    EXPECT_EQ(canvas.at(40, 30), RED);
}

TEST(Picture, RepaintsWhenMoved) {
    tests::Canvas canvas(160, 120);
    auto *picture = canvas.root().layer(ttk::Root::POPUPS)->append(std::make_unique<ttk::Picture>(solid(4, 4, BLUE)));

    picture->place({10, 10, 30, 30}, type());
    canvas.frame();
    picture->invalidate();
    picture->place({100, 70, 30, 30}, type());
    picture->invalidate();
    canvas.frame();

    EXPECT_EQ(canvas.at(20, 20), ttk::Theme::palette().background.value);
    EXPECT_EQ(canvas.at(110, 80), BLUE);
    EXPECT_TRUE(canvas.matches_full());
}
}
