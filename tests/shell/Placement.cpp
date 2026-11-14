// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <vector>

#include <gtest/gtest.h>

#include "ttk/shell/Placement.h"

namespace {
using ttk::Placement::GRAB_HEIGHT;
using ttk::Placement::GRAB_WIDTH;

const SDL_Rect PRIMARY{0, 0, 1920, 1040};
const SDL_Rect LEFT{-1280, -100, 1280, 1024};
const SDL_Rect ABOVE{0, -1040, 1920, 1040};

SDL_Point fit(const SDL_Rect &window, const std::vector<SDL_Rect> &displays) {
    return ttk::Placement::fit(window, displays);
}

void expect_point(const SDL_Point &got, const int x, const int y) {
    EXPECT_EQ(got.x, x);
    EXPECT_EQ(got.y, y);
}
}

TEST(Placement, WindowInsideADisplayStays) {
    expect_point(fit({100, 80, 800, 600}, {PRIMARY}), 100, 80);
}

TEST(Placement, NegativePositionOnADisplayLeftOfThePrimaryStays) {
    expect_point(fit({-1000, 50, 720, 520}, {PRIMARY, LEFT}), -1000, 50);
}

TEST(Placement, NegativePositionAboveThePrimaryStays) {
    expect_point(fit({200, -900, 720, 520}, {PRIMARY, ABOVE}), 200, -900);
}

TEST(Placement, WindowOnAGoneDisplayMovesToThePrimary) {
    expect_point(fit({-1000, 50, 720, 520}, {PRIMARY}), 0, 50);
}

TEST(Placement, WindowFarOffToTheRightMovesAgainstTheRightEdge) {
    expect_point(fit({5000, 300, 800, 600}, {PRIMARY}), 1920 - 800, 300);
}

TEST(Placement, WindowBelowEveryDisplayMovesAgainstTheBottomEdge) {
    expect_point(fit({300, 4000, 800, 600}, {PRIMARY}), 300, 1040 - 600);
}

TEST(Placement, WindowAboveEveryDisplayMovesDown) {
    expect_point(fit({300, -700, 800, 600}, {PRIMARY}), 300, 0);
}

TEST(Placement, TitleBarAboveTheUsableAreaIsNotReachable) {
    expect_point(fit({100, -10, 800, 600}, {PRIMARY}), 100, 0);
}

TEST(Placement, TitleBarJustReachableStays) {
    expect_point(fit({100, 0, 800, 600}, {PRIMARY}), 100, 0);
}

TEST(Placement, WindowHangingOffTheRightStaysWhileTheGripFits) {
    expect_point(fit({1920 - GRAB_WIDTH, 100, 800, 600}, {PRIMARY}), 1920 - GRAB_WIDTH, 100);
}

TEST(Placement, WindowHangingOffTheRightMovesWhenTheGripDoesNotFit) {
    expect_point(fit({1920 - GRAB_WIDTH + 1, 100, 800, 600}, {PRIMARY}), 1920 - 800, 100);
}

TEST(Placement, WindowHangingOffTheLeftStaysWhileTheGripFits) {
    expect_point(fit({GRAB_WIDTH - 800, 100, 800, 600}, {PRIMARY}), GRAB_WIDTH - 800, 100);
}

TEST(Placement, TitleBarBelowTheUsableAreaIsNotReachable) {
    expect_point(fit({100, 1040 - GRAB_HEIGHT + 1, 800, 600}, {PRIMARY}), 100, 1040 - 600);
}

TEST(Placement, WindowNarrowerThanTheGripNeedsAllOfItsWidth) {
    expect_point(fit({1900, 100, 50, 300}, {PRIMARY}), 1920 - 50, 100);
}

TEST(Placement, WindowBetweenTwoDisplaysGoesToTheNearer) {
    const std::vector<SDL_Rect> displays{PRIMARY, {3000, 0, 1920, 1040}};

    expect_point(fit({2100, 100, 800, 600}, displays), 3000, 100);
    expect_point(fit({1950, 100, 800, 600}, displays), 1920 - 800, 100);
}

TEST(Placement, TieGoesToTheEarlierDisplay) {
    const std::vector<SDL_Rect> displays{PRIMARY, {2520, 0, 1920, 1040}};

    expect_point(fit({1920, 100, 600, 400}, displays), 1920 - 600, 100);
    expect_point(fit({1920, 100, 600, 400}, {displays[1], displays[0]}), 2520, 100);
}

TEST(Placement, WindowLargerThanTheDisplayGoesToItsCorner) {
    expect_point(fit({-300, -300, 3000, 2000}, {PRIMARY}), 0, 0);
    expect_point(fit({9000, 9000, 3000, 2000}, {PRIMARY}), 0, 0);
}

TEST(Placement, WindowReachableOnASecondDisplayStaysWhenFirstIsFar) {
    expect_point(fit({-1200, -50, 720, 520}, {PRIMARY, LEFT}), -1200, -50);
}

TEST(Placement, NoDisplaysKeepsThePosition) {
    expect_point(fit({-4000, 9000, 800, 600}, {}), -4000, 9000);
}
