// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

#include "draw/Pixels.h"
#include "ttk/draw/Svg.h"

namespace {
    BLBox bounds(const BLPath &path) {
        BLBox box{};

        path.get_bounding_box(&box);

        return box;
    }

    ::testing::AssertionResult boxed(const BLPath &path, const double x0, const double y0, const double x1,
                                     const double y1, const double slack = 1e-6) {
        const BLBox box = bounds(path);

        if (std::abs(box.x0 - x0) > slack || std::abs(box.y0 - y0) > slack || std::abs(box.x1 - x1) > slack
            || std::abs(box.y1 - y1) > slack) {
            return ::testing::AssertionFailure()
                   << "bounds " << box.x0 << "," << box.y0 << " .. " << box.x1 << "," << box.y1;
        }

        return ::testing::AssertionSuccess();
    }

    BLPoint vertex(const BLPath &path, const std::size_t at) {
        return path.vertex_data()[at];
    }

    TEST(Svg, NullFailsAndClears) {
        BLPath path;

        path.move_to(1, 1);
        path.line_to(2, 2);

        EXPECT_FALSE(ttk::Svg::parse(nullptr, path));
        EXPECT_EQ(path.size(), 0U);
    }

    TEST(Svg, EmptyAndBlankParseToNothing) {
        BLPath path;

        EXPECT_TRUE(ttk::Svg::parse("", path));
        EXPECT_EQ(path.size(), 0U);
        EXPECT_TRUE(ttk::Svg::parse(" \t\n, ", path));
        EXPECT_EQ(path.size(), 0U);
    }

    TEST(Svg, ParseClearsWhatWasThere) {
        BLPath path;

        path.move_to(-100, -100);
        ASSERT_TRUE(ttk::Svg::parse("M 0 0 L 4 4", path));

        EXPECT_EQ(path.size(), 2U);
        EXPECT_TRUE(boxed(path, 0, 0, 4, 4));
    }

    TEST(Svg, AbsoluteLinesAndClose) {
        BLPath path;

        ASSERT_TRUE(ttk::Svg::parse("M 1 2 L 11 2 L 11 12 Z", path));

        EXPECT_TRUE(boxed(path, 1, 2, 11, 12));
        ASSERT_EQ(path.size(), 4U);
        EXPECT_EQ(path.command_data()[0], BL_PATH_CMD_MOVE);
        EXPECT_EQ(path.command_data()[3], BL_PATH_CMD_CLOSE);
    }

    TEST(Svg, RelativeCommandsMoveFromThePen) {
        BLPath path;

        ASSERT_TRUE(ttk::Svg::parse("m 5 5 l 10 0 l 0 10 z", path));

        EXPECT_TRUE(boxed(path, 5, 5, 15, 15));
    }

    TEST(Svg, HorizontalAndVerticalLines) {
        BLPath path;

        ASSERT_TRUE(ttk::Svg::parse("M 2 3 H 10 V 9 h -4 v 5", path));

        EXPECT_TRUE(boxed(path, 2, 3, 10, 14));
        EXPECT_DOUBLE_EQ(vertex(path, 1).y, 3.0) << "H moved the pen vertically";
        EXPECT_DOUBLE_EQ(vertex(path, 2).x, 10.0) << "V moved the pen horizontally";
        EXPECT_DOUBLE_EQ(vertex(path, 4).x, 6.0);
        EXPECT_DOUBLE_EQ(vertex(path, 4).y, 14.0);
    }

    TEST(Svg, PairsAfterAMoveAreLines) {
        BLPath absolute;
        BLPath relative;

        ASSERT_TRUE(ttk::Svg::parse("M 0 0 10 0 10 10", absolute));
        ASSERT_TRUE(ttk::Svg::parse("m 1 1 10 0 0 10", relative));

        ASSERT_EQ(absolute.size(), 3U);
        EXPECT_EQ(absolute.command_data()[1], BL_PATH_CMD_ON);
        EXPECT_EQ(absolute.command_data()[2], BL_PATH_CMD_ON);
        EXPECT_TRUE(boxed(relative, 1, 1, 11, 11));
    }

    TEST(Svg, RepeatedArgumentsRepeatTheCommand) {
        BLPath path;

        ASSERT_TRUE(ttk::Svg::parse("M 0 0 L 1 1 2 2 3 3 h 1 1 1", path));

        EXPECT_EQ(path.size(), 7U);
        EXPECT_DOUBLE_EQ(vertex(path, 6).x, 6.0);
    }

    TEST(Svg, SeparatorsMayBeCommasOrNothing) {
        BLPath spaced;
        BLPath packed;

        ASSERT_TRUE(ttk::Svg::parse("M 0 0 L 10 -5 L 3.5 0.5", spaced));
        ASSERT_TRUE(ttk::Svg::parse("M0,0L10-5L3.5.5", packed));

        ASSERT_EQ(packed.size(), spaced.size());

        for (std::size_t at = 0; at < spaced.size(); ++at) {
            EXPECT_DOUBLE_EQ(vertex(packed, at).x, vertex(spaced, at).x) << at;
            EXPECT_DOUBLE_EQ(vertex(packed, at).y, vertex(spaced, at).y) << at;
        }
    }

    TEST(Svg, NumbersMayCarryExponents) {
        BLPath path;

        ASSERT_TRUE(ttk::Svg::parse("M 1e1 0 L 2.5E-1 -1e0", path));

        EXPECT_DOUBLE_EQ(vertex(path, 0).x, 10.0);
        EXPECT_DOUBLE_EQ(vertex(path, 1).x, 0.25);
        EXPECT_DOUBLE_EQ(vertex(path, 1).y, -1.0);
    }

    TEST(Svg, CubicAndSmoothCubicReflectTheControl) {
        BLPath path;

        ASSERT_TRUE(ttk::Svg::parse("M 0 0 C 0 10 10 10 10 0 S 20 -10 20 0", path));
        ASSERT_EQ(path.size(), 7U);

        EXPECT_EQ(path.command_data()[1], BL_PATH_CMD_CUBIC);
        EXPECT_DOUBLE_EQ(vertex(path, 4).x, 10.0);
        EXPECT_DOUBLE_EQ(vertex(path, 4).y, -10.0) << "S did not reflect the last control point";
        EXPECT_DOUBLE_EQ(vertex(path, 6).x, 20.0);
    }

    TEST(Svg, SmoothCubicAfterALineUsesThePen) {
        BLPath path;

        ASSERT_TRUE(ttk::Svg::parse("M 0 0 L 5 5 S 10 10 15 5", path));

        EXPECT_DOUBLE_EQ(vertex(path, 2).x, 5.0);
        EXPECT_DOUBLE_EQ(vertex(path, 2).y, 5.0);
    }

    TEST(Svg, RelativeCubic) {
        BLPath path;

        ASSERT_TRUE(ttk::Svg::parse("M 10 10 c 0 10 10 10 10 0 s 10 -10 10 0", path));

        EXPECT_DOUBLE_EQ(vertex(path, 3).x, 20.0);
        EXPECT_DOUBLE_EQ(vertex(path, 4).y, 0.0) << "s did not reflect about the pen";
        EXPECT_DOUBLE_EQ(vertex(path, 6).x, 30.0);
    }

    TEST(Svg, QuadAndSmoothQuadReflectTheControl) {
        BLPath path;

        ASSERT_TRUE(ttk::Svg::parse("M 0 0 Q 5 10 10 0 T 20 0", path));
        ASSERT_EQ(path.size(), 5U);

        EXPECT_EQ(path.command_data()[1], BL_PATH_CMD_QUAD);
        EXPECT_DOUBLE_EQ(vertex(path, 3).x, 15.0);
        EXPECT_DOUBLE_EQ(vertex(path, 3).y, -10.0) << "T did not reflect the last control point";
    }

    TEST(Svg, SmoothQuadChainsReflectEachOther) {
        BLPath path;

        ASSERT_TRUE(ttk::Svg::parse("M 0 0 Q 5 10 10 0 t 10 0 t 10 0", path));
        ASSERT_EQ(path.size(), 7U);

        EXPECT_DOUBLE_EQ(vertex(path, 5).x, 25.0);
        EXPECT_DOUBLE_EQ(vertex(path, 5).y, 10.0);
    }

    TEST(Svg, ArcsMakeACircle) {
        BLPath path;

        ASSERT_TRUE(ttk::Svg::parse("M 0 5 A 5 5 0 1 1 10 5 A 5 5 0 1 1 0 5 Z", path));

        EXPECT_TRUE(boxed(path, 0, 0, 10, 10, 0.05));
    }

    TEST(Svg, ArcFlagsNeedNoSeparators) {
        BLPath spaced;
        BLPath packed;

        ASSERT_TRUE(ttk::Svg::parse("M 0 5 a 5 5 0 1 1 10 0", spaced));
        ASSERT_TRUE(ttk::Svg::parse("M0 5a5 5 0 1110 0", packed));

        const BLBox one = bounds(spaced);
        const BLBox two = bounds(packed);

        EXPECT_NEAR(one.x1, 10.0, 0.05);
        EXPECT_NEAR(two.x0, one.x0, 1e-9);
        EXPECT_NEAR(two.x1, one.x1, 1e-9);
        EXPECT_NEAR(two.y0, one.y0, 1e-9);
        EXPECT_NEAR(two.y1, one.y1, 1e-9);
    }

    TEST(Svg, ArcFlagMustBeADigit) {
        BLPath path;

        EXPECT_FALSE(ttk::Svg::parse("M 0 5 A 5 5 0 2 1 10 5", path));
    }

    TEST(Svg, ArcRotationIsInDegrees) {
        BLPath path;

        ASSERT_TRUE(ttk::Svg::parse("M 0 0 A 10 2 90 1 1 0 4 A 10 2 90 1 1 0 0", path));

        const BLBox box = bounds(path);

        EXPECT_NEAR(box.x1 - box.x0, 7.92, 0.05) << "a 90 degree turn did not stand the ellipse up";
        EXPECT_GT(box.y1 - box.y0, 15.0);
    }

    TEST(Svg, CloseReturnsThePenHome) {
        BLPath path;

        ASSERT_TRUE(ttk::Svg::parse("M 3 4 L 10 4 L 10 10 Z l 1 1", path));

        const BLPoint last = vertex(path, path.size() - 1);

        EXPECT_DOUBLE_EQ(last.x, 4.0);
        EXPECT_DOUBLE_EQ(last.y, 5.0);
    }

    TEST(Svg, FailureKeepsWhatParsedBeforeIt) {
        BLPath path;

        EXPECT_FALSE(ttk::Svg::parse("M 0 0 L 10 10 L 5", path));
        EXPECT_EQ(path.size(), 2U);

        EXPECT_FALSE(ttk::Svg::parse("M 0 0 L 4 4 X 1 2", path));
        EXPECT_EQ(path.size(), 2U);
    }

    TEST(Svg, NumbersBeforeAnyCommandFail) {
        BLPath path;

        EXPECT_FALSE(ttk::Svg::parse("10 10", path));
        EXPECT_EQ(path.size(), 0U);
    }

    TEST(Svg, GarbageFails) {
        BLPath path;

        EXPECT_FALSE(ttk::Svg::parse("M a b", path));
        EXPECT_FALSE(ttk::Svg::parse("M 0 0 L", path));
        EXPECT_FALSE(ttk::Svg::parse("M 0 0 C 1 2 3", path));
    }

    TEST(Svg, GlyphScalesTheViewbox) {
        const BLPath path = ttk::Svg::glyph("M 0 0 L 12 12", 12.0F, 24.0F);

        EXPECT_TRUE(boxed(path, 0, 0, 24, 24));

        const BLPath down = ttk::Svg::glyph("M 2 4 L 8 8", 8.0F, 4.0F);

        EXPECT_TRUE(boxed(down, 1, 2, 4, 4));
    }

    TEST(Svg, GlyphOfABrokenPathIsUnscaled) {
        const BLPath path = ttk::Svg::glyph("M 0 0 L 12 12 L 5", 12.0F, 24.0F);

        EXPECT_TRUE(boxed(path, 0, 0, 12, 12));
    }

    TEST(Svg, GlyphOfNullIsEmpty) {
        EXPECT_EQ(ttk::Svg::glyph(nullptr, 12.0F, 24.0F).size(), 0U);
    }

    TEST(Svg, ParsedTriangleRendersWhereItSays) {
        BLImage image;

        ASSERT_EQ(image.create(40, 40, BL_FORMAT_PRGB32), BL_SUCCESS);

        BLPath path;

        ASSERT_TRUE(ttk::Svg::parse("M4 4H36L4 36Z", path));

        {
            BLContext context(image);

            context.fill_all(BLRgba32(0xff000000));
            context.fill_path(path, BLRgba32(0xffffffff));
        }

        EXPECT_EQ(tests::pixel(image, 6, 6), 0xffffffffU);
        EXPECT_EQ(tests::pixel(image, 30, 5), 0xffffffffU);
        EXPECT_EQ(tests::pixel(image, 5, 30), 0xffffffffU);
        EXPECT_EQ(tests::pixel(image, 30, 30), 0xff000000U) << "the far side of the hypotenuse was filled";
        EXPECT_EQ(tests::pixel(image, 2, 2), 0xff000000U);
        EXPECT_EQ(tests::pixel(image, 38, 4), 0xff000000U);
    }

    TEST(Svg, ScaledGlyphRendersAtItsSize) {
        BLImage image;

        ASSERT_EQ(image.create(64, 64, BL_FORMAT_PRGB32), BL_SUCCESS);

        {
            BLContext context(image);

            context.fill_all(BLRgba32(0xff000000));
            context.fill_path(ttk::Svg::glyph("M 1 1 h 2 v 2 h -2 z", 4.0F, 64.0F), BLRgba32(0xffffffff));
        }

        const tests::Ink found = tests::ink(image, 0xff000000U);

        EXPECT_EQ(found.x0, 16);
        EXPECT_EQ(found.y0, 16);
        EXPECT_EQ(found.x1, 48);
        EXPECT_EQ(found.y1, 48);
        EXPECT_EQ(found.count, 32U * 32U);
    }
}
