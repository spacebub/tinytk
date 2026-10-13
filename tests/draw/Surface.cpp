// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <vector>

#include <gtest/gtest.h>

#include "draw/Pixels.h"
#include "support/Sandbox.h"
#include "ttk/draw/Paint.h"
#include "ttk/draw/Surface.h"

namespace {
    constexpr int WIDE = 64;
    constexpr int TALL = 48;

    bool same(const BLRectI &one, const BLRectI &two) {
        return one.x == two.x && one.y == two.y && one.w == two.w && one.h == two.h;
    }

    bool holds(const ttk::Surface &surface, const BLRectI &wanted) {
        return std::ranges::any_of(surface.regions(), [&](const BLRectI &r) { return same(r, wanted); });
    }

    std::uint32_t row_colour(const int y) {
        return static_cast<std::uint32_t>(y) * 0x010203U;
    }

    // The X byte of an XRGB pixel carries nothing.
    std::uint32_t rgb(const ttk::Surface &surface, const int x, const int y) {
        return tests::pixel(surface.image(), x, y) & 0xffffffU;
    }

    // Each row a colour of its own, presented so nothing is pending.
    void stripe(ttk::Surface &surface) {
        BLContext &context = surface.context();

        for (int y = 0; y < surface.height(); ++y) {
            context.fill_rect(BLRectI{0, y, surface.width(), 1}, BLRgba32(0xff000000U | row_colour(y)));
        }

        surface.present();
    }

    TEST(Surface, DetachedByDefault) {
        const ttk::Surface surface;

        EXPECT_FALSE(surface.ready());
        EXPECT_EQ(surface.width(), 0);
        EXPECT_EQ(surface.height(), 0);
        EXPECT_FALSE(surface.dirty());
        EXPECT_TRUE(surface.regions().empty());
        EXPECT_TRUE(surface.image().is_empty());
    }

    TEST(Surface, CallsWhileDetachedAreIgnored) {
        ttk::Surface surface;

        surface.damage({0, 0, 10, 10});
        surface.damage_all();
        surface.shift({0, 0, 10, 10}, 3);
        surface.present();

        EXPECT_FALSE(surface.dirty());
        EXPECT_TRUE(surface.regions().empty());
        EXPECT_FALSE(surface.save((tests::Sandbox::scratch("surface-detached") / "never.png").string().c_str()));
    }

    TEST(Surface, AttachMakesATargetDamagedWhole) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));

        EXPECT_TRUE(surface.ready());
        EXPECT_FALSE(surface.direct()) << "a target with no window drew in place";
        EXPECT_EQ(surface.width(), WIDE);
        EXPECT_EQ(surface.height(), TALL);
        EXPECT_EQ(surface.image().width(), WIDE);
        EXPECT_EQ(surface.image().height(), TALL);
        EXPECT_EQ(surface.image().format(), BL_FORMAT_XRGB32);
        EXPECT_TRUE(surface.dirty());
        ASSERT_EQ(surface.regions().size(), 1U);
        EXPECT_TRUE(same(surface.regions()[0], {0, 0, WIDE, TALL}));
    }

    TEST(Surface, AttachFailsWithoutPixels) {
        for (const auto [w, h] : {std::pair{0, 10}, std::pair{10, 0}, std::pair{-4, 10}, std::pair{0, 0}}) {
            ttk::Surface surface;

            EXPECT_FALSE(surface.attach(w, h)) << w << "x" << h;
            EXPECT_FALSE(surface.ready());
            EXPECT_EQ(surface.width(), 0);
            EXPECT_EQ(surface.height(), 0);
            EXPECT_FALSE(surface.dirty());
        }
    }

    TEST(Surface, FailedAttachReleasesThePreviousTarget) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));
        EXPECT_FALSE(surface.attach(0, 0));
        EXPECT_FALSE(surface.ready());
        EXPECT_EQ(surface.width(), 0);
        EXPECT_FALSE(surface.dirty());
    }

    TEST(Surface, PresentClearsDamage) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));

        surface.present();

        EXPECT_FALSE(surface.dirty());
        EXPECT_TRUE(surface.regions().empty());
        EXPECT_TRUE(surface.ready()) << "present released the target";

        surface.present();

        EXPECT_FALSE(surface.dirty());
    }

    TEST(Surface, DamageIsClampedAndPending) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));
        surface.present();
        surface.damage({-4.5, 40.25, 10, 20});

        EXPECT_TRUE(surface.dirty());
        ASSERT_EQ(surface.regions().size(), 1U);
        EXPECT_TRUE(same(surface.regions()[0], {0, 40, 6, 8}));

        surface.damage({500, 500, 10, 10});

        EXPECT_EQ(surface.regions().size(), 1U) << "damage outside the target was kept";
    }

    TEST(Surface, DamageAllCoversTheTarget) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));
        surface.present();
        surface.damage({1, 1, 2, 2});
        surface.damage_all();

        ASSERT_EQ(surface.regions().size(), 1U);
        EXPECT_TRUE(same(surface.regions()[0], {0, 0, WIDE, TALL}));
    }

    TEST(Surface, DamageAllDropsPendingShifts) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));
        stripe(surface);
        surface.shift({0, 0, WIDE, TALL}, 4);

        ASSERT_TRUE(surface.dirty());

        surface.damage_all();
        surface.present();

        EXPECT_FALSE(surface.dirty());

        surface.present();

        EXPECT_FALSE(surface.dirty()) << "a dropped shift came back";
    }

    TEST(Surface, PaintingThroughTheContextLandsInTheImage) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));

        BLContext &context = surface.context();

        context.fill_all(BLRgba32(0xff102030));
        context.fill_rect(BLRectI{10, 12, 5, 6}, BLRgba32(0xffa0b0c0));
        surface.present();

        EXPECT_EQ(rgb(surface, 10, 12), 0xa0b0c0U);
        EXPECT_EQ(rgb(surface, 14, 17), 0xa0b0c0U);
        EXPECT_EQ(rgb(surface, 9, 12), 0x102030U);
        EXPECT_EQ(rgb(surface, 15, 17), 0x102030U);
        EXPECT_EQ(rgb(surface, 10, 18), 0x102030U);
    }

    TEST(Surface, TranslucentPaintBlendsOverWhatIsThere) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));
        surface.context().fill_all(BLRgba32(0xffffffff));
        surface.context().fill_all(BLRgba32(0x80000000));
        surface.present();

        EXPECT_TRUE(tests::close(rgb(surface, 3, 3), 0x7f7f7fU, 1)) << std::hex << rgb(surface, 3, 3);
    }

    TEST(Surface, PaintingADamagedRectangleChangesOnlyIt) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));
        surface.context().fill_all(BLRgba32(0xff000000));
        surface.present();
        surface.damage({20, 10, 8, 4});

        BLContext &context = surface.context();

        for (const BLRectI &r : surface.regions()) {
            context.save();
            context.clip_to_rect(r);
            context.fill_all(BLRgba32(0xffffffff));
            context.restore();
        }

        surface.present();

        const tests::Ink changed = tests::ink(surface.image(), tests::pixel(surface.image(), 0, 0));

        EXPECT_EQ(changed.x0, 20);
        EXPECT_EQ(changed.y0, 10);
        EXPECT_EQ(changed.x1, 28);
        EXPECT_EQ(changed.y1, 14);
        EXPECT_EQ(changed.count, 32U);
    }

    TEST(Surface, ShiftDownMovesThePixels) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));
        stripe(surface);
        surface.shift({0, 0, WIDE, TALL}, 5);

        EXPECT_TRUE(surface.dirty()) << "a shift alone left nothing to present";
        EXPECT_TRUE(surface.regions().empty()) << "the uncovered strip was damaged";

        surface.present();

        for (int y = 5; y < TALL; ++y) {
            ASSERT_EQ(rgb(surface, 7, y), row_colour(y - 5)) << "row " << y;
        }

        for (int y = 0; y < 5; ++y) {
            EXPECT_EQ(rgb(surface, 7, y), row_colour(y)) << "the uncovered strip changed at " << y;
        }

        EXPECT_FALSE(surface.dirty());
    }

    TEST(Surface, ShiftUpMovesThePixels) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));
        stripe(surface);
        surface.shift({0, 0, WIDE, TALL}, -7);
        surface.present();

        for (int y = 0; y < TALL - 7; ++y) {
            ASSERT_EQ(rgb(surface, 0, y), row_colour(y + 7)) << "row " << y;
        }

        for (int y = TALL - 7; y < TALL; ++y) {
            EXPECT_EQ(rgb(surface, 0, y), row_colour(y));
        }
    }

    TEST(Surface, ShiftMovesOnlyItsRegion) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));
        stripe(surface);
        surface.shift({10, 8, 20, 30}, 3);
        surface.present();

        EXPECT_EQ(rgb(surface, 10, 11), row_colour(8));
        EXPECT_EQ(rgb(surface, 29, 37), row_colour(34));
        EXPECT_EQ(rgb(surface, 9, 11), row_colour(11)) << "pixels left of the region moved";
        EXPECT_EQ(rgb(surface, 30, 11), row_colour(11)) << "pixels right of the region moved";
        EXPECT_EQ(rgb(surface, 15, 38), row_colour(38)) << "pixels below the region moved";
        EXPECT_EQ(rgb(surface, 15, 7), row_colour(7)) << "pixels above the region moved";
    }

    TEST(Surface, ShiftIsClippedToTheTarget) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));
        stripe(surface);
        surface.shift({-50, -50, WIDE + 100, TALL + 100}, 2);
        surface.present();

        EXPECT_EQ(rgb(surface, 0, 2), row_colour(0));
        EXPECT_EQ(rgb(surface, WIDE - 1, TALL - 1), row_colour(TALL - 3));
    }

    TEST(Surface, ShiftCarriesPendingDamage) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(1000, 1000));
        surface.present();
        surface.damage({100, 300, 100, 10});
        surface.shift({0, 200, 1000, 600}, 300);

        EXPECT_TRUE(holds(surface, {100, 300, 100, 10})) << "the pending damage did not stay";
        EXPECT_TRUE(holds(surface, {100, 600, 100, 10})) << "the pending damage was not carried with the pixels";
    }

    TEST(Surface, ShiftByZeroDoesNothing) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));
        stripe(surface);
        surface.shift({0, 0, WIDE, TALL}, 0);

        EXPECT_FALSE(surface.dirty());
        EXPECT_EQ(rgb(surface, 0, 10), row_colour(10));
    }

    TEST(Surface, ShiftOutsideTheTargetDoesNothing) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));
        stripe(surface);

        for (const BLRectI &outside : {
                     BLRectI{WIDE, 0, 10, TALL},
                     BLRectI{0, TALL, WIDE, 10},
                     BLRectI{-20, 0, 20, TALL},
                     BLRectI{0, 0, 0, TALL},
             }) {
            surface.shift(outside, 3);

            EXPECT_FALSE(surface.dirty()) << "at " << outside.x << "," << outside.y;
        }
    }

    TEST(Surface, ShiftAfterWholeDamageDoesNothing) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));
        stripe(surface);
        surface.damage_all();
        surface.shift({0, 0, WIDE, TALL}, 6);

        ASSERT_EQ(surface.regions().size(), 1U);
        EXPECT_TRUE(same(surface.regions()[0], {0, 0, WIDE, TALL}));

        surface.present();

        EXPECT_EQ(rgb(surface, 0, 10), row_colour(10)) << "pixels moved under whole damage";
    }

    TEST(Surface, ShiftAsFarAsTheRegionDamagesItInstead) {
        for (const int dy : {20, 21, -20, 500}) {
            ttk::Surface surface;

            ASSERT_TRUE(surface.attach(WIDE, TALL));
            stripe(surface);
            surface.shift({0, 10, WIDE, 20}, dy);

            ASSERT_EQ(surface.regions().size(), 1U) << "dy " << dy;
            EXPECT_TRUE(same(surface.regions()[0], {0, 10, WIDE, 20})) << "dy " << dy;

            surface.present();

            EXPECT_EQ(rgb(surface, 0, 25), row_colour(25)) << "pixels moved for dy " << dy;
        }
    }

    TEST(Surface, ShiftComparesWithTheClippedHeight) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));
        stripe(surface);
        surface.shift({0, TALL - 10, WIDE, 100}, 10);

        ASSERT_EQ(surface.regions().size(), 1U);
        EXPECT_TRUE(same(surface.regions()[0], {0, TALL - 10, WIDE, 10}));
    }

    TEST(Surface, ShiftsAccumulateUntilPresent) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));
        stripe(surface);
        surface.shift({0, 0, WIDE, TALL}, 2);
        surface.shift({0, 0, WIDE, TALL}, 3);
        surface.present();

        EXPECT_EQ(rgb(surface, 0, 10), row_colour(5));
        EXPECT_FALSE(surface.dirty());
    }

    TEST(Surface, DetachReleasesTheTarget) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));
        surface.detach();

        EXPECT_FALSE(surface.ready());
        EXPECT_EQ(surface.width(), 0);
        EXPECT_EQ(surface.height(), 0);
        EXPECT_FALSE(surface.dirty());

        surface.damage({0, 0, 4, 4});

        EXPECT_FALSE(surface.dirty()) << "damage was kept after detach";

        surface.detach();

        EXPECT_FALSE(surface.ready());
    }

    TEST(Surface, ReattachTakesTheNewSize) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));
        surface.present();
        ASSERT_TRUE(surface.attach(30, 70));

        EXPECT_EQ(surface.width(), 30);
        EXPECT_EQ(surface.height(), 70);
        EXPECT_EQ(surface.image().width(), 30);
        EXPECT_EQ(surface.image().height(), 70);
        ASSERT_EQ(surface.regions().size(), 1U);
        EXPECT_TRUE(same(surface.regions()[0], {0, 0, 30, 70}));

        surface.context().fill_rect(BLRectI{29, 69, 1, 1}, BLRgba32(0xff00ff00));
        surface.present();

        EXPECT_EQ(rgb(surface, 29, 69), 0x00ff00U) << "the context still drew into the old image";
    }

    TEST(Surface, ReattachAfterDetach) {
        ttk::Surface surface;

        ASSERT_TRUE(surface.attach(WIDE, TALL));
        surface.detach();
        ASSERT_TRUE(surface.attach(8, 8));

        EXPECT_TRUE(surface.ready());
        EXPECT_TRUE(surface.dirty());
    }

    TEST(Surface, SaveWritesWhatWasPainted) {
        ttk::Surface surface;
        const std::filesystem::path file = tests::Sandbox::scratch("surface-save") / "frame.png";

        ASSERT_TRUE(surface.attach(WIDE, TALL));
        surface.context().fill_all(BLRgba32(0xff204060));
        surface.context().fill_rect(BLRectI{4, 5, 3, 2}, BLRgba32(0xffc08040));

        ASSERT_TRUE(surface.save(file.string().c_str()));
        ASSERT_TRUE(std::filesystem::exists(file));

        const BLImage read = ttk::Paint::load(file.string());

        ASSERT_EQ(read.width(), WIDE);
        ASSERT_EQ(read.height(), TALL);
        EXPECT_EQ(tests::pixel(read, 4, 5) & 0xffffffU, 0xc08040U);
        EXPECT_EQ(tests::pixel(read, 0, 0) & 0xffffffU, 0x204060U);
    }

    TEST(Surface, SaveFailsWhereItCannotWrite) {
        ttk::Surface surface;
        const std::filesystem::path missing = tests::Sandbox::scratch("surface-unwritable") / "no" / "such" / "x.png";

        ASSERT_TRUE(surface.attach(WIDE, TALL));

        EXPECT_FALSE(surface.save(missing.string().c_str()));
    }
}
