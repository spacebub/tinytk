// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "draw/Pixels.h"
#include "support/Sandbox.h"
#include "ttk/draw/Paint.h"

namespace {
std::uint32_t alpha_of(const std::uint32_t pixel) {
    return pixel >> 24U;
}

TEST(Paint, BleedIsAWholeNumberOfPixels) {
    for (const double blur : {0.0, 0.3, 1.0, 4.5, 10.0, 33.3}) {
        const double bleed = ttk::Paint::bleed(blur);

        EXPECT_EQ(bleed, std::floor(bleed)) << "blur " << blur;
        EXPECT_GE(bleed, blur * 1.5) << "the sprite cuts the tail at blur " << blur;
    }
}

TEST(Paint, BleedGrowsWithBlur) {
    double last = 0.0;

    for (int half = 0; half < 80; ++half) {
        const double blur = half * 0.5;

        EXPECT_GE(ttk::Paint::bleed(blur), last);
        last = ttk::Paint::bleed(blur);
    }

    EXPECT_GT(ttk::Paint::bleed(0.0), 0.0) << "an unblurred shadow left no room for its edge";
}

TEST(Paint, ShadowSpriteReachesBleedPastTheRectangle) {
    const BLImage &sprite = ttk::Paint::shadow(40, 20, 4.0, 6.0, BLRgba32(0xff000000));
    const int bleed = static_cast<int>(ttk::Paint::bleed(6.0));

    ASSERT_FALSE(sprite.is_empty());
    EXPECT_EQ(sprite.width(), 40 + (2 * bleed));
    EXPECT_EQ(sprite.height(), 20 + (2 * bleed));
    EXPECT_EQ(sprite.format(), BL_FORMAT_PRGB32);
}

TEST(Paint, ShadowIsPremultiplied) {
    const BLImage &sprite = ttk::Paint::shadow(30, 30, 6.0, 8.0, BLRgba32(0xc0ff8040));

    for (int y = 0; y < sprite.height(); ++y) {
        for (int x = 0; x < sprite.width(); ++x) {
            const std::uint32_t at = tests::pixel(sprite, x, y);
            const std::uint32_t a = alpha_of(at);

            ASSERT_LE((at >> 16U) & 0xffU, a) << x << "," << y;
            ASSERT_LE((at >> 8U) & 0xffU, a) << x << "," << y;
            ASSERT_LE(at & 0xffU, a) << x << "," << y;
        }
    }
}

TEST(Paint, ShadowIsSolidInsideAndFadesOutside) {
    const BLImage &sprite = ttk::Paint::shadow(80, 60, 4.0, 4.0, BLRgba32(0x80000000));
    const int bleed = static_cast<int>(ttk::Paint::bleed(4.0));

    EXPECT_NEAR(alpha_of(tests::pixel(sprite, bleed + 40, bleed + 30)), 0x80U, 2U) << "the middle was not the tint";
    EXPECT_EQ(alpha_of(tests::pixel(sprite, 0, 0)), 0U) << "the corner of the sprite was not clear";
    EXPECT_LT(alpha_of(tests::pixel(sprite, 0, bleed + 30)), 0x80U / 10U) << "the tail was cut at the sprite's edge";

    const std::uint32_t edge = alpha_of(tests::pixel(sprite, bleed, bleed + 30));

    EXPECT_GT(edge, 0x20U);
    EXPECT_LT(edge, 0x70U) << "the edge of the rectangle was not blurred";
}

TEST(Paint, ShadowIsSymmetric) {
    const BLImage &sprite = ttk::Paint::shadow(31, 17, 5.0, 7.0, BLRgba32(0xff000000));
    const int w = sprite.width();
    const int h = sprite.height();

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const std::uint32_t here = alpha_of(tests::pixel(sprite, x, y));

            ASSERT_NEAR(here, alpha_of(tests::pixel(sprite, w - 1 - x, y)), 2U) << x << "," << y;
            ASSERT_NEAR(here, alpha_of(tests::pixel(sprite, x, h - 1 - y)), 2U) << x << "," << y;
        }
    }
}

TEST(Paint, MoreBlurSpreadsFurther) {
    const int outside = 3;
    const BLImage &soft = ttk::Paint::shadow(40, 40, 0.0, 12.0, BLRgba32(0xff000000));
    const int softBleed = static_cast<int>(ttk::Paint::bleed(12.0));
    const std::uint32_t far = alpha_of(tests::pixel(soft, softBleed - outside, softBleed + 20));
    const BLImage &hard = ttk::Paint::shadow(40, 40, 0.0, 2.0, BLRgba32(0xff000000));
    const int hardBleed = static_cast<int>(ttk::Paint::bleed(2.0));
    const std::uint32_t near = alpha_of(tests::pixel(hard, hardBleed - outside, hardBleed + 20));

    EXPECT_GT(far, near);
}

TEST(Paint, ShadowTakesTheTint) {
    const BLImage &sprite = ttk::Paint::shadow(40, 40, 0.0, 2.0, BLRgba32(0xff3060c0));
    const int bleed = static_cast<int>(ttk::Paint::bleed(2.0));
    const std::uint32_t middle = tests::pixel(sprite, bleed + 20, bleed + 20);

    EXPECT_TRUE(tests::close(middle, 0xff3060c0U, 2)) << std::hex << middle;
}

TEST(Paint, ShadowsAreCachedToAQuarterPixel) {
    const BLImage &first = ttk::Paint::shadow(50, 30, 4.0, 6.0, BLRgba32(0x80000000));

    EXPECT_EQ(&ttk::Paint::shadow(50, 30, 4.0, 6.0, BLRgba32(0x80000000)), &first);
    EXPECT_EQ(&ttk::Paint::shadow(50, 30, 4.1, 6.1, BLRgba32(0x80000000)), &first);
    EXPECT_NE(&ttk::Paint::shadow(50, 30, 4.25, 6.0, BLRgba32(0x80000000)), &first);
    EXPECT_NE(&ttk::Paint::shadow(50, 30, 4.0, 6.25, BLRgba32(0x80000000)), &first);
    EXPECT_NE(&ttk::Paint::shadow(50, 30, 4.0, 6.0, BLRgba32(0x81000000)), &first);
    EXPECT_NE(&ttk::Paint::shadow(51, 30, 4.0, 6.0, BLRgba32(0x80000000)), &first);
}

TEST(Paint, ShadowAfterTheCacheEmptiesIsMadeAgain) {
    const BLImage &first = ttk::Paint::shadow(20, 20, 2.0, 3.0, BLRgba32(0xff000000));
    const std::uint32_t middle = tests::pixel(first, 15, 15);

    for (int size = 1; size <= 40; ++size) {
        ttk::Paint::shadow(size, size, 1.0, 2.0, BLRgba32(0xff102030));
    }

    const BLImage &again = ttk::Paint::shadow(20, 20, 2.0, 3.0, BLRgba32(0xff000000));

    ASSERT_FALSE(again.is_empty());
    EXPECT_EQ(again.width(), 20 + (2 * static_cast<int>(ttk::Paint::bleed(3.0))));
    EXPECT_EQ(tests::pixel(again, 15, 15), middle);
}

TEST(Paint, ShadowOfNothingIsClear) {
    const BLImage &sprite = ttk::Paint::shadow(0, 0, 0.0, 4.0, BLRgba32(0xff000000));
    const int bleed = static_cast<int>(ttk::Paint::bleed(4.0));

    ASSERT_EQ(sprite.width(), 2 * bleed);
    EXPECT_FALSE(tests::ink(sprite, 0U).any());
}

TEST(Paint, ShadowTooLargeToAllocateIsEmpty) {
    EXPECT_TRUE(ttk::Paint::shadow(1U << 20U, 10, 0.0, 1.0, BLRgba32(0xff000000)).is_empty());
}

TEST(Paint, DownRunsTopToBottomWithNoStops) {
    const BLGradient gradient = ttk::Paint::down({10, 20, 30, 40});

    EXPECT_EQ(gradient.type(), BL_GRADIENT_TYPE_LINEAR);
    EXPECT_EQ(gradient.size(), 0U);
    EXPECT_DOUBLE_EQ(gradient.linear().x0, 10.0);
    EXPECT_DOUBLE_EQ(gradient.linear().y0, 20.0);
    EXPECT_DOUBLE_EQ(gradient.linear().x1, 10.0);
    EXPECT_DOUBLE_EQ(gradient.linear().y1, 60.0);
}

TEST(Paint, DownFillsDarkToLight) {
    BLImage image;

    ASSERT_EQ(image.create(20, 40, BL_FORMAT_PRGB32), BL_SUCCESS);

    BLGradient gradient = ttk::Paint::down({0, 0, 20, 40});

    gradient.add_stop(0.0, BLRgba32(0xff000000));
    gradient.add_stop(1.0, BLRgba32(0xffffffff));

    {
        BLContext context(image);

        context.fill_all(gradient);
    }

    const std::uint32_t top = tests::pixel(image, 5, 0) & 0xffU;
    const std::uint32_t middle = tests::pixel(image, 5, 20) & 0xffU;
    const std::uint32_t bottom = tests::pixel(image, 5, 39) & 0xffU;

    EXPECT_LT(top, 0x10U);
    EXPECT_NEAR(middle, 0x80U, 8U);
    EXPECT_GT(bottom, 0xf0U);
    EXPECT_EQ(tests::pixel(image, 0, 20), tests::pixel(image, 19, 20)) << "the gradient ran sideways";
}

TEST(Paint, LoadOfAMissingFileIsEmpty) {
    EXPECT_TRUE(ttk::Paint::load((tests::Sandbox::scratch("paint-missing") / "nothing.png").string()).is_empty());
    EXPECT_TRUE(ttk::Paint::load("").is_empty());
}

TEST(Paint, LoadOfAFileThatIsNoImageIsEmpty) {
    const std::filesystem::path file = tests::Sandbox::scratch("paint-garbage") / "text.png";

    std::ofstream(file) << "not an image at all";

    EXPECT_TRUE(ttk::Paint::load(file.string()).is_empty());
}

TEST(Paint, LoadDecodesAPng) {
    const std::filesystem::path file = tests::Sandbox::scratch("paint-png") / "dot.png";
    BLImage made;

    ASSERT_EQ(made.create(7, 5, BL_FORMAT_PRGB32), BL_SUCCESS);

    {
        BLContext context(made);

        context.fill_all(BLRgba32(0xff112233));
        context.fill_rect(BLRectI{6, 4, 1, 1}, BLRgba32(0xffaabbcc));
    }

    ASSERT_EQ(made.write_to_file(file.string().c_str()), BL_SUCCESS);

    const BLImage read = ttk::Paint::load(file.string());

    ASSERT_EQ(read.width(), 7);
    ASSERT_EQ(read.height(), 5);
    EXPECT_EQ(tests::pixel(read, 0, 0) & 0xffffffU, 0x112233U);
    EXPECT_EQ(tests::pixel(read, 6, 4) & 0xffffffU, 0xaabbccU);
}

class Covered : public ::testing::Test {
protected:
    static constexpr std::uint32_t GROUND = 0xff000000U;
    static constexpr std::uint32_t RED = 0xffff0000U;
    static constexpr std::uint32_t BLUE = 0xff0000ffU;
    static constexpr std::uint32_t GREEN = 0xff00ff00U;

    // Three stripes ten pixels each, across or down.
    static BLImage stripes(const bool across) {
        BLImage made;

        made.create(across ? 30 : 10, across ? 10 : 30, BL_FORMAT_PRGB32);

        BLContext into(made);
        int at = 0;

        for (const std::uint32_t colour : {RED, BLUE, GREEN}) {
            into.fill_rect(across ? BLRectI{at, 0, 10, 10} : BLRectI{0, at, 10, 10}, BLRgba32(colour));
            at += 10;
        }

        return made;
    }

    void SetUp() override {
        ASSERT_EQ(image.create(100, 100, BL_FORMAT_PRGB32), BL_SUCCESS);
        context.begin(image);
        context.fill_all(BLRgba32(GROUND));

        wide = stripes(true);

        ASSERT_FALSE(wide.is_empty());
    }

    void TearDown() override { context.end(); }

    tests::Ink settle() {
        context.flush(BL_CONTEXT_FLUSH_SYNC);

        return tests::ink(image, GROUND);
    }

    BLImage image;
    BLImage wide;
    BLContext context;
};

TEST_F(Covered, EmptySourceDrawsNothing) {
    ttk::Paint::cover(context, {10, 10, 40, 40}, BLImage(), 0.0);
    ttk::Paint::cover(context, {10, 10, 40, 40}, BLImage(), 8.0);

    EXPECT_FALSE(settle().any());
}

TEST_F(Covered, CoverFillsTheBoxAndNothingElse) {
    ttk::Paint::cover(context, {20, 30, 40, 40}, wide);

    const tests::Ink found = settle();

    EXPECT_EQ(found.x0, 20);
    EXPECT_EQ(found.y0, 30);
    EXPECT_EQ(found.x1, 60);
    EXPECT_EQ(found.y1, 70);
    EXPECT_EQ(found.count, 1600U) << "a pixel inside the box was left uncovered";
}

TEST_F(Covered, CoverCropsTheLongerSideAroundTheCentre) {
    ttk::Paint::cover(context, {20, 30, 40, 40}, wide);
    settle();

    for (int y = 34; y < 66; y += 4) {
        for (int x = 24; x < 56; x += 4) {
            ASSERT_TRUE(tests::close(tests::pixel(image, x, y), BLUE, 8))
                    << "a side stripe showed in a square box at " << x << "," << y;
        }
    }
}

TEST_F(Covered, CoverCropsATallSourceAroundTheCentre) {
    ttk::Paint::cover(context, {20, 30, 40, 40}, stripes(false));
    settle();

    EXPECT_TRUE(tests::close(tests::pixel(image, 40, 34), BLUE, 8));
    EXPECT_TRUE(tests::close(tests::pixel(image, 40, 65), BLUE, 8));
    EXPECT_TRUE(tests::close(tests::pixel(image, 22, 50), BLUE, 8));
}

TEST_F(Covered, CoverKeepsTheAspectRatio) {
    ttk::Paint::cover(context, {0, 0, 40, 20}, wide);
    settle();

    EXPECT_TRUE(tests::close(tests::pixel(image, 4, 10), RED, 8));
    EXPECT_TRUE(tests::close(tests::pixel(image, 20, 10), BLUE, 8));
    EXPECT_TRUE(tests::close(tests::pixel(image, 14, 10), BLUE, 8)) << "the source was stretched, not scaled";
    EXPECT_TRUE(tests::close(tests::pixel(image, 35, 10), GREEN, 8));
}

TEST_F(Covered, RadiusRoundsTheCorners) {
    ttk::Paint::cover(context, {20, 20, 40, 40}, wide, 10.0);
    settle();

    EXPECT_EQ(tests::pixel(image, 20, 20), GROUND) << "the corner was not rounded off";
    EXPECT_EQ(tests::pixel(image, 59, 59), GROUND);
    EXPECT_TRUE(tests::close(tests::pixel(image, 40, 40), BLUE, 8));
    EXPECT_TRUE(tests::close(tests::pixel(image, 40, 20), BLUE, 8)) << "the straight top edge was cut";
}

TEST_F(Covered, RadiusIsClampedToHalfTheShorterSide) {
    ttk::Paint::cover(context, {10, 40, 80, 20}, wide, 1000.0);

    const tests::Ink found = settle();

    EXPECT_GE(found.x0, 10);
    EXPECT_LE(found.x1, 90);
    EXPECT_GE(found.y0, 40);
    EXPECT_LE(found.y1, 60);
    EXPECT_NE(tests::pixel(image, 50, 41), GROUND) << "a huge radius ate the middle of the long edge";
    EXPECT_NE(tests::pixel(image, 11, 50), GROUND) << "the rounded end was not a half circle";
    EXPECT_EQ(tests::pixel(image, 10, 40), GROUND);
}

TEST_F(Covered, NegativeRadiusIsSquare) {
    ttk::Paint::cover(context, {20, 20, 40, 40}, wide, -5.0);
    settle();

    EXPECT_NE(tests::pixel(image, 20, 20), GROUND);
}
}
