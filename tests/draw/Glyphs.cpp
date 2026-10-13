// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

#include <gtest/gtest.h>

#include "draw/Pixels.h"
#include "support/Sandbox.h"
#include "ttk/draw/Glyphs.h"
#include "ttk/draw/Paint.h"

namespace {
    using ttk::Glyphs::Glyph;

    constexpr std::uint32_t GROUND = 0xff000000U;

    class Sheet : public ::testing::Test {
    protected:
        static constexpr int SIDE = 96;

        void SetUp() override {
            ASSERT_EQ(image.create(SIDE, SIDE, BL_FORMAT_PRGB32), BL_SUCCESS);
            context.begin(image);
            context.fill_all(BLRgba32(GROUND));
        }

        void TearDown() override { context.end(); }

        void wipe() { context.fill_all(BLRgba32(GROUND)); }

        tests::Ink settle() {
            context.flush(BL_CONTEXT_FLUSH_SYNC);

            return tests::ink(image, GROUND);
        }

        std::vector<std::uint32_t> drawn() {
            context.flush(BL_CONTEXT_FLUSH_SYNC);

            return tests::pixels(image);
        }

        BLImage image;
        BLContext context;
    };

    std::vector<Glyph> every() {
        std::vector<Glyph> all;

        for (std::size_t at = 1; at < static_cast<std::size_t>(Glyph::Count); ++at) {
            all.push_back(static_cast<Glyph>(at));
        }

        return all;
    }

    bool near(const std::vector<std::uint32_t> &one, const std::vector<std::uint32_t> &two, const int slack) {
        if (one.size() != two.size()) {
            return false;
        }

        for (std::size_t at = 0; at < one.size(); ++at) {
            if (!tests::close(one[at], two[at], slack)) {
                return false;
            }
        }

        return true;
    }

    TEST(Glyphs, SpanIsElementTimesWeight) {
        EXPECT_FLOAT_EQ(ttk::Glyphs::element, 12.0F);
        EXPECT_FLOAT_EQ(ttk::Glyphs::span(1.0F), 12.0F);
        EXPECT_FLOAT_EQ(ttk::Glyphs::span(2.5F), 30.0F);
        EXPECT_FLOAT_EQ(ttk::Glyphs::span(0.0F), 0.0F);
    }

    TEST_F(Sheet, EmptyDrawsNothing) {
        for (int pass = 0; pass < 3; ++pass) {
            ttk::Glyphs::draw(context, Glyph::Empty, {10, 10}, 2.0F, BLRgba32(0xffffffff));
            ttk::Glyphs::draw(context, Glyph::Empty, {10, 10}, 2.0F, BLRgba32(0xffffffff), 45.0F);
        }

        EXPECT_FALSE(settle().any());
    }

    TEST_F(Sheet, EveryGlyphInksWithinItsReach) {
        for (const float weight : {1.0F, 1.5F, 2.6F}) {
            for (const Glyph glyph : every()) {
                wipe();

                const BLPoint origin{20.0, 24.0};
                const double side = ttk::Glyphs::span(weight);
                const double reach = (1.1 * weight) + 1.0;

                ttk::Glyphs::draw(context, glyph, origin, weight, BLRgba32(0xffffffff));

                const tests::Ink found = settle();

                ASSERT_TRUE(found.any()) << "glyph " << static_cast<int>(glyph) << " weight " << weight;
                EXPECT_GE(found.x0, static_cast<int>(std::floor(origin.x - reach)))
                        << "glyph " << static_cast<int>(glyph);
                EXPECT_GE(found.y0, static_cast<int>(std::floor(origin.y - reach)))
                        << "glyph " << static_cast<int>(glyph);
                EXPECT_LE(found.x1, static_cast<int>(std::ceil(origin.x + side + reach)))
                        << "glyph " << static_cast<int>(glyph);
                EXPECT_LE(found.y1, static_cast<int>(std::ceil(origin.y + side + reach)))
                        << "glyph " << static_cast<int>(glyph);
            }
        }
    }

    TEST_F(Sheet, GlyphsAreCentredInTheirSquare) {
        for (const Glyph glyph : {
                     Glyph::Close,
                     Glyph::Plus,
                     Glyph::Check,
                     Glyph::Globe,
                     Glyph::Dots,
                     Glyph::Grip,
                     Glyph::Maximize,
                     Glyph::Minimize,
             }) {
            wipe();

            const BLPoint origin{30.0, 30.0};
            constexpr float weight = 3.0F;
            const double centre = origin.x + (ttk::Glyphs::span(weight) * 0.5);

            ttk::Glyphs::draw(context, glyph, origin, weight, BLRgba32(0xffffffff));

            const tests::Ink found = settle();

            ASSERT_TRUE(found.any());
            EXPECT_NEAR((found.x0 + found.x1) * 0.5, centre, 2.0) << "glyph " << static_cast<int>(glyph);
            EXPECT_NEAR((found.y0 + found.y1) * 0.5, origin.y + (ttk::Glyphs::span(weight) * 0.5), 2.0)
                    << "glyph " << static_cast<int>(glyph);
        }
    }

    TEST_F(Sheet, InkGrowsWithWeight) {
        ttk::Glyphs::draw(context, Glyph::Close, {10, 10}, 1.0F, BLRgba32(0xffffffff));

        const tests::Ink small = settle();

        wipe();
        ttk::Glyphs::draw(context, Glyph::Close, {10, 10}, 4.0F, BLRgba32(0xffffffff));

        const tests::Ink large = settle();

        EXPECT_NEAR(large.x1 - large.x0, (small.x1 - small.x0) * 4, 6);
        EXPECT_GT(large.count, small.count * 8);
    }

    TEST_F(Sheet, DrawsInTheTone) {
        ttk::Glyphs::draw(context, Glyph::Folder, {10, 10}, 4.0F, BLRgba32(0xff00ff00));

        const tests::Ink found = settle();
        bool solid = false;

        ASSERT_TRUE(found.any());

        for (int y = found.y0; y < found.y1; ++y) {
            for (int x = found.x0; x < found.x1; ++x) {
                const std::uint32_t at = tests::pixel(image, x, y);

                ASSERT_EQ(at & 0xff00ffU, 0U) << "ink not in the tone at " << x << "," << y;
                solid = solid || at == 0xff00ff00U;
            }
        }

        EXPECT_TRUE(solid) << "a filled glyph had no fully covered pixel";
    }

    TEST_F(Sheet, RepeatedDrawsFromTheMaskMatchTheStroke) {
        for (const Glyph glyph : every()) {
            constexpr float weight = 1.73F;
            const BLPoint origin{20.0, 20.0};

            wipe();
            ttk::Glyphs::draw(context, glyph, origin, weight, BLRgba32(0xffffffff));

            const std::vector<std::uint32_t> stroked = drawn();

            for (int again = 0; again < 2; ++again) {
                wipe();
                ttk::Glyphs::draw(context, glyph, origin, weight, BLRgba32(0xffffffff));

                EXPECT_TRUE(near(drawn(), stroked, 2))
                        << "draw " << again + 2 << " of glyph " << static_cast<int>(glyph) << " differs";
            }
        }
    }

    TEST_F(Sheet, MaskKeepsInkPastTheSquare) {
        for (const float weight : {1.0F, 2.6F, 4.5F}) {
            for (const Glyph glyph : every()) {
                const BLPoint origin{20.0, 24.0};

                wipe();
                ttk::Glyphs::draw(context, glyph, origin, weight, BLRgba32(0xffffffff));

                const tests::Ink stroked = settle();

                for (int again = 0; again < 2; ++again) {
                    wipe();
                    ttk::Glyphs::draw(context, glyph, origin, weight, BLRgba32(0xffffffff));

                    const tests::Ink masked = settle();

                    EXPECT_EQ(masked.x0, stroked.x0) << "glyph " << static_cast<int>(glyph) << " weight " << weight;
                    EXPECT_EQ(masked.y0, stroked.y0) << "glyph " << static_cast<int>(glyph) << " weight " << weight;
                    EXPECT_EQ(masked.x1, stroked.x1) << "glyph " << static_cast<int>(glyph) << " weight " << weight;
                    EXPECT_EQ(masked.y1, stroked.y1) << "glyph " << static_cast<int>(glyph) << " weight " << weight;
                    EXPECT_EQ(masked.count, stroked.count)
                            << "glyph " << static_cast<int>(glyph) << " weight " << weight;
                }
            }
        }
    }

    TEST_F(Sheet, LightKeepsItsRaysFromTheMask) {
        for (int pass = 0; pass < 3; ++pass) {
            wipe();
            ttk::Glyphs::draw(context, Glyph::Light, {20, 24}, 2.6F, BLRgba32(0xffffffff));

            const tests::Ink found = settle();

            EXPECT_EQ(found.x0, 17) << "draw " << pass + 1;
            EXPECT_EQ(found.y0, 21) << "draw " << pass + 1;
            EXPECT_EQ(found.x1, 54) << "draw " << pass + 1;
            EXPECT_EQ(found.y1, 58) << "draw " << pass + 1;
        }
    }

    TEST_F(Sheet, SubPixelOriginIsKeptNotRounded) {
        ttk::Glyphs::draw(context, Glyph::Plus, {20.0, 20.0}, 2.11F, BLRgba32(0xffffffff));

        const std::vector<std::uint32_t> whole = drawn();

        for (int pass = 0; pass < 3; ++pass) {
            wipe();
            ttk::Glyphs::draw(context, Glyph::Plus, {20.5, 20.0}, 2.11F, BLRgba32(0xffffffff));

            EXPECT_NE(drawn(), whole) << "half a pixel over was drawn as a whole pixel on pass " << pass;
        }
    }

    TEST_F(Sheet, OffGridContextStillDraws) {
        context.save();
        context.translate(0.5, 0.25);

        for (int pass = 0; pass < 3; ++pass) {
            ttk::Glyphs::draw(context, Glyph::Search, {10, 10}, 2.0F, BLRgba32(0xffffffff));
        }

        context.restore();

        EXPECT_TRUE(settle().any());
    }

    TEST_F(Sheet, ScaledContextDrawsLarger) {
        context.save();
        context.scale(2.0);
        ttk::Glyphs::draw(context, Glyph::Copy, {5, 5}, 2.0F, BLRgba32(0xffffffff));
        context.restore();

        const tests::Ink found = settle();

        ASSERT_TRUE(found.any());
        EXPECT_GT(found.x1 - found.x0, static_cast<int>(ttk::Glyphs::span(2.0F)));
        EXPECT_LE(found.x1, static_cast<int>((5.0 + ttk::Glyphs::span(2.0F)) * 2.0) + 2);
    }

    TEST_F(Sheet, TurnRotatesAboutTheCentre) {
        const BLPoint origin{30.0, 30.0};
        constexpr float weight = 3.0F;
        const double centre = origin.x + (ttk::Glyphs::span(weight) * 0.5);

        ttk::Glyphs::draw(context, Glyph::Play, origin, weight, BLRgba32(0xffffffff));

        const tests::Ink right = settle();

        wipe();
        ttk::Glyphs::draw(context, Glyph::Play, origin, weight, BLRgba32(0xffffffff), 180.0F);

        const tests::Ink left = settle();

        ASSERT_TRUE(left.any());
        EXPECT_NEAR(left.x0, (2.0 * centre) - right.x1, 1.5) << "a half turn did not mirror the triangle";
        EXPECT_NEAR(left.x1, (2.0 * centre) - right.x0, 1.5);
        EXPECT_NEAR(left.y0, right.y0, 1.5);
    }

    TEST_F(Sheet, QuarterTurnSwapsWidthAndHeight) {
        ttk::Glyphs::draw(context, Glyph::Minus, {30, 30}, 3.0F, BLRgba32(0xffffffff));

        const tests::Ink flat = settle();

        wipe();
        ttk::Glyphs::draw(context, Glyph::Minus, {30, 30}, 3.0F, BLRgba32(0xffffffff), 90.0F);

        const tests::Ink upright = settle();

        EXPECT_GT(flat.x1 - flat.x0, flat.y1 - flat.y0);
        EXPECT_GT(upright.y1 - upright.y0, upright.x1 - upright.x0);
    }

    TEST_F(Sheet, DrawLeavesTheContextAsItWas) {
        context.translate(3, 4);
        context.set_stroke_width(7.0);
        context.set_comp_op(BL_COMP_OP_SRC_OVER);
        context.set_global_alpha(0.75);

        const BLMatrix2D before = context.user_transform();
        const std::uint32_t saved = context.saved_state_count();

        for (const Glyph glyph : every()) {
            ttk::Glyphs::draw(context, glyph, {10.5, 10}, 1.5F, BLRgba32(0xffffffff));
            ttk::Glyphs::draw(context, glyph, {10.5, 10}, 1.5F, BLRgba32(0xffffffff), 30.0F);
        }

        const BLMatrix2D after = context.user_transform();

        EXPECT_EQ(context.saved_state_count(), saved);
        EXPECT_DOUBLE_EQ(context.stroke_width(), 7.0);
        EXPECT_DOUBLE_EQ(context.global_alpha(), 0.75);
        EXPECT_EQ(context.comp_op(), BL_COMP_OP_SRC_OVER);
        EXPECT_DOUBLE_EQ(after.m20, before.m20);
        EXPECT_DOUBLE_EQ(after.m21, before.m21);
        EXPECT_DOUBLE_EQ(after.m00, before.m00);
    }

    TEST_F(Sheet, MinusIsTheMinimizeBar) {
        ttk::Glyphs::draw(context, Glyph::Minimize, {20, 20}, 2.0F, BLRgba32(0xffffffff));

        const std::vector<std::uint32_t> minimize = drawn();

        wipe();
        ttk::Glyphs::draw(context, Glyph::Minus, {20, 20}, 2.0F, BLRgba32(0xffffffff));

        EXPECT_EQ(drawn(), minimize);
    }

    TEST_F(Sheet, GlyphsAreTellableApart) {
        std::vector<std::vector<std::uint32_t>> seen;

        for (const Glyph glyph : every()) {
            if (glyph == Glyph::Minus) {
                continue;
            }

            wipe();
            ttk::Glyphs::draw(context, glyph, {20, 20}, 3.0F, BLRgba32(0xffffffff));

            std::vector<std::uint32_t> one = drawn();

            for (const std::vector<std::uint32_t> &other : seen) {
                EXPECT_NE(one, other) << "glyph " << static_cast<int>(glyph) << " repeats an earlier one";
            }

            seen.push_back(std::move(one));
        }
    }

    TEST(Glyphs, SheetHoldsEveryGlyphInACell) {
        const std::filesystem::path file = tests::Sandbox::scratch("glyph-sheet") / "sheet.png";

        ASSERT_TRUE(ttk::Glyphs::sheet(file.string().c_str()));

        const BLImage read = ttk::Paint::load(file.string());
        const int glyphs = static_cast<int>(Glyph::Count) - 1;
        constexpr int cell = 64;
        constexpr int columns = 8;

        ASSERT_EQ(read.width(), cell * columns);
        ASSERT_EQ(read.height(), cell * ((glyphs + columns - 1) / columns));
        EXPECT_EQ(tests::pixel(read, 0, 0), 0xff0a0d13U);

        for (int at = 0; at < columns * (read.height() / cell); ++at) {
            const int x = (at % columns) * cell;
            const int y = (at / columns) * cell;
            bool inked = false;

            for (int py = y; py < y + cell && !inked; ++py) {
                for (int px = x; px < x + cell && !inked; ++px) {
                    inked = tests::pixel(read, px, py) != 0xff0a0d13U;
                }
            }

            EXPECT_EQ(inked, at < glyphs) << "cell " << at;
        }
    }

    TEST(Glyphs, SheetFailsWhereItCannotWrite) {
        const std::filesystem::path missing = tests::Sandbox::scratch("glyph-unwritable") / "no" / "sheet.png";

        EXPECT_FALSE(ttk::Glyphs::sheet(missing.string().c_str()));
    }
}
