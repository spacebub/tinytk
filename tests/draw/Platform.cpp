// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <array>

#include <gtest/gtest.h>

#include "ttk/draw/Chrome.h"
#include "ttk/draw/WlShm.h"

namespace {
    TEST(WlShm, NothingIsAvailableWithoutAWindow) {
        EXPECT_FALSE(ttk::WlShm::available(nullptr));
    }

    TEST(WlShm, FreshRingIsNotOpened) {
        ttk::WlShm shm;

        EXPECT_FALSE(shm.opened());
    }

    TEST(WlShm, OpenWithoutAWindowFails) {
        ttk::WlShm shm;

        EXPECT_FALSE(shm.open(nullptr));
        EXPECT_FALSE(shm.opened());
    }

    TEST(WlShm, AcquireWhenNotOpenedFails) {
        ttk::WlShm shm;
        ttk::WlShm::Target target;

        EXPECT_FALSE(shm.acquire(64, 48, false, target));
        EXPECT_FALSE(shm.acquire(64, 48, true, target));
        EXPECT_FALSE(shm.acquire(0, 0, true, target));
        EXPECT_FALSE(shm.acquire(-1, 10, true, target));
        EXPECT_EQ(target.pixels, nullptr) << "a failed acquire filled the target";
    }

    TEST(WlShm, PresentWithoutAHeldBufferFails) {
        ttk::WlShm shm;
        const std::array<BLRectI, 1> regions{BLRectI{0, 0, 10, 10}};

        EXPECT_FALSE(shm.present(regions));
        EXPECT_FALSE(shm.present({}));
    }

    TEST(WlShm, CloseIsSafeWhenNothingIsOpen) {
        ttk::WlShm shm;

        shm.close();
        shm.close();

        EXPECT_FALSE(shm.opened());
    }

    TEST(WlShm, TargetStartsEmpty) {
        const ttk::WlShm::Target target;

        EXPECT_EQ(target.pixels, nullptr);
        EXPECT_EQ(target.stride, 0);
        EXPECT_FALSE(target.fresh);
    }

    bool told = false;

    void tell(const bool /*resizing*/) {
        told = true;
    }

    TEST(Chrome, CallsWithoutAWindowDoNothing) {
        told = false;

        ttk::Chrome::outline(10, 20, 30);
        ttk::Chrome::while_resizing(tell);
        ttk::Chrome::apply(nullptr);
        ttk::Chrome::outline(255, 255, 255);
        ttk::Chrome::while_resizing(nullptr);

        EXPECT_FALSE(told) << "the modal loop callback ran on a platform without one";
    }
}
