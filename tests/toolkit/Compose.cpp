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
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "support/Canvas.h"
#include "toolkit/Probe.h"
#include "ttk/draw/Surface.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Compose.h"
#include "ttk/toolkit/Painter.h"
#include "ttk/toolkit/Root.h"

namespace {
    constexpr int WIDTH = 160;
    constexpr int HEIGHT = 100;
    constexpr std::size_t WINDOW = static_cast<std::size_t>(WIDTH) * HEIGHT;

    constexpr std::uint32_t RGB = 0x00ffffff;
    constexpr std::uint32_t RED = 0xffff0000;
    constexpr std::uint32_t BLUE = 0xff0000ff;

    constexpr int ROW = 10;

    // Rows of distinct colours scrolled up by `offset` pixels, so a moved pixel shows where it came from.
    class Stripes final : public ttk::Widget {
    public:
        int offset = 0;
        int marked = -1;

        void paint(const ttk::Painter &painter) override {
            const int rows = (static_cast<int>(_box.h) / ROW) + 2;
            const int first = offset / ROW;

            for (int at = first; at < first + rows; ++at) {
                const BLRect band{_box.x, _box.y + (at * ROW) - offset, _box.w, ROW};
                const auto shade = static_cast<std::uint32_t>((at * 37) % 256);

                painter.fill(band, BLRgba32(at == marked ? RED : 0xff000000U | (shade << 8U) | (255U - shade)));
            }
        }
    };

    std::uint32_t rgb(const std::uint32_t pixel) {
        return pixel & RGB;
    }

    class Composed : public ::testing::Test {
    protected:
        tests::Canvas canvas{WIDTH, HEIGHT};

        Stripes *stripes() {
            auto *made = canvas.root().content()->append(std::make_unique<Stripes>());

            canvas.root().relayout();
            canvas.frame();

            return made;
        }

        tests::Probe *probe(const BLRect &box, const std::uint32_t tone) {
            tests::Probe *made = tests::mount(canvas.root().layer(ttk::Root::POPUPS), box, tests::fonts());

            made->tone = BLRgba32(tone);

            return made;
        }
    };

    TEST_F(Composed, FirstFramePaintsTheWholeWindow) {
        EXPECT_EQ(canvas.frame(), WINDOW);
    }

    TEST_F(Composed, NothingDirtyPaintsNothing) {
        canvas.frame();

        EXPECT_EQ(canvas.frame(), 0U);
        EXPECT_EQ(ttk::compose(canvas.root(), canvas.surface()), 0U);
    }

    TEST_F(Composed, ReturnsThePixelsOfTheDamage) {
        canvas.frame();
        canvas.root().damage({10, 10, 20, 20});

        EXPECT_EQ(canvas.frame(), 400U);
    }

    TEST_F(Composed, FractionalDamageIsRoundedOutToPixels) {
        canvas.frame();
        canvas.root().damage({10.5, 10.5, 5, 5});

        EXPECT_EQ(canvas.frame(), 36U);
    }

    TEST_F(Composed, DamageOutsideTheWindowPaintsNothing) {
        canvas.frame();
        canvas.root().damage({WIDTH + 5.0, 0, 20, 20});

        EXPECT_EQ(canvas.frame(), 0U);
    }

    TEST(Compose, DistantDamageIsPaintedApart) {
        tests::Canvas canvas(400, 240);

        canvas.frame();
        canvas.root().damage({0, 0, 2, 2});
        canvas.root().damage({398, 238, 2, 2});

        EXPECT_EQ(canvas.frame(), 8U) << "two corners were merged into a larger repaint";
    }

    TEST_F(Composed, WholeWindowDamagePaintsTheWindowOnce) {
        canvas.frame();
        canvas.root().damage({0, 0, 10, 10});
        canvas.root().damage_all();
        canvas.root().damage({50, 50, 10, 10});

        EXPECT_EQ(canvas.frame(), WINDOW);
    }

    TEST_F(Composed, LeavesTheRootClean) {
        canvas.root().damage({10, 10, 20, 20});
        canvas.frame();

        EXPECT_FALSE(canvas.root().dirty());
        EXPECT_FALSE(canvas.surface().dirty()) << "presenting left damage on the surface";
    }

    TEST_F(Composed, SettlesAPendingLayout) {
        canvas.frame();

        const auto *child = canvas.root().content()->append(std::make_unique<tests::Probe>());

        canvas.root().relayout();

        EXPECT_EQ(canvas.frame(), WINDOW);
        EXPECT_TRUE(tests::same(child->box(), BLRect{0, 0, WIDTH, HEIGHT}));
    }

    TEST_F(Composed, FillsUncoveredPixelsWithTheThemeBackground) {
        canvas.frame();

        const std::uint32_t background = rgb(ttk::Theme::palette().background.value);

        EXPECT_EQ(rgb(canvas.at(0, 0)), background);
        EXPECT_EQ(rgb(canvas.at(WIDTH - 1, HEIGHT - 1)), background);
    }

    TEST_F(Composed, PaintsTheTreeOverTheBackground) {
        canvas.frame();
        probe({10, 10, 20, 20}, RED);
        canvas.root().damage({0, 0, 50, 50});
        canvas.frame();

        EXPECT_EQ(rgb(canvas.at(15, 15)), rgb(RED));
        EXPECT_EQ(rgb(canvas.at(35, 35)), rgb(ttk::Theme::palette().background.value));
    }

    TEST_F(Composed, RepaintsOnlyWhatIsDamaged) {
        tests::Probe *box = probe({10, 10, 40, 40}, RED);

        canvas.frame();
        box->tone = BLRgba32(BLUE);
        canvas.root().damage({10, 10, 20, 40});
        canvas.frame();

        EXPECT_EQ(rgb(canvas.at(15, 20)), rgb(BLUE));
        EXPECT_EQ(rgb(canvas.at(35, 20)), rgb(RED)) << "pixels outside the damage were repainted";
    }

    TEST_F(Composed, DamagedRegionIsClearedBeforePainting) {
        tests::Probe *box = probe({10, 10, 40, 40}, RED);

        canvas.frame();
        box->set_visible(false);
        canvas.frame();

        EXPECT_EQ(rgb(canvas.at(20, 20)), rgb(ttk::Theme::palette().background.value))
                << "a hidden widget's pixels stayed";
    }

    TEST_F(Composed, PartialRepaintMatchesAFullOne) {
        tests::Probe *one = probe({10, 10, 40, 40}, RED);

        probe({30, 30, 40, 40}, BLUE);
        canvas.frame();
        one->tone = BLRgba32(0xff00ff00);
        one->invalidate();
        canvas.frame();

        EXPECT_TRUE(canvas.matches_full());
    }

    TEST_F(Composed, AShiftMovesPixelsWithoutRepaintingThem) {
        Stripes *rows = stripes();
        const std::uint32_t below = canvas.at(5, 25);

        rows->offset = 5;

        ASSERT_TRUE(canvas.root().shift(rows, {0, 0, WIDTH, HEIGHT}, -5));

        EXPECT_EQ(canvas.frame(), 0U) << "a bare move repainted pixels";
        EXPECT_EQ(canvas.at(5, 20), below) << "the pixels did not move up";
    }

    TEST_F(Composed, ShiftsLandBeforeTheRegionsDamagedAfterThem) {
        Stripes *rows = stripes();

        rows->offset = 7;

        ASSERT_TRUE(canvas.root().shift(rows, {0, 0, WIDTH, HEIGHT}, -7));

        canvas.root().damage({0, HEIGHT - 7.0, WIDTH, 7});

        EXPECT_EQ(canvas.frame(), static_cast<std::size_t>(WIDTH) * 7) << "more than the uncovered strip was painted";
        EXPECT_TRUE(canvas.matches_full()) << "the strip was painted before the move carried it off";
    }

    TEST_F(Composed, DamagePendingBeforeAShiftMovesWithThePixels) {
        Stripes *rows = stripes();

        rows->marked = 4;
        canvas.root().damage({0, 40, WIDTH, ROW});
        rows->offset = 10;

        ASSERT_TRUE(canvas.root().shift(rows, {0, 0, WIDTH, HEIGHT}, -10));

        canvas.root().damage({0, HEIGHT - 10.0, WIDTH, 10});
        canvas.frame();

        EXPECT_EQ(rgb(canvas.at(5, 35)), rgb(RED)) << "the marked row was not repainted where it moved to";
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST_F(Composed, ADownwardShiftMatchesAFullRepaint) {
        Stripes *rows = stripes();

        rows->offset = 30;
        canvas.root().damage_all();
        canvas.frame();
        rows->offset = 18;

        ASSERT_TRUE(canvas.root().shift(rows, {0, 0, WIDTH, HEIGHT}, 12));

        canvas.root().damage({0, 0, WIDTH, 12});
        canvas.frame();

        EXPECT_TRUE(canvas.matches_full());
    }

    TEST_F(Composed, SeveralShiftsInOneFrameMatchAFullRepaint) {
        Stripes *rows = stripes();

        for (const int step : {3, 4, -2}) {
            rows->offset += step;

            ASSERT_TRUE(canvas.root().shift(rows, {0, 0, WIDTH, HEIGHT}, -step));

            if (step > 0) {
                canvas.root().damage({0, HEIGHT - static_cast<double>(step), WIDTH, static_cast<double>(step)});
            } else {
                canvas.root().damage({0, 0, WIDTH, static_cast<double>(-step)});
            }
        }

        canvas.frame();

        EXPECT_TRUE(canvas.matches_full());
    }

    TEST_F(Composed, WholeWindowDamageDropsTheShifts) {
        Stripes *rows = stripes();

        rows->offset = 6;

        ASSERT_TRUE(canvas.root().shift(rows, {0, 0, WIDTH, HEIGHT}, -6));

        canvas.root().damage_all();

        EXPECT_EQ(canvas.frame(), WINDOW);
        EXPECT_TRUE(canvas.matches_full()) << "a move was applied over a whole repaint";
    }

    TEST_F(Composed, AShiftOfPartOfTheWindowLeavesTheRestAlone) {
        const Stripes *rows = stripes();
        const std::uint32_t outside = canvas.at(100, 30);
        const std::uint32_t moving = canvas.at(10, 30);

        ASSERT_TRUE(canvas.root().shift(rows, {0, 0, 50, HEIGHT}, -10));

        canvas.frame();

        EXPECT_EQ(canvas.at(100, 30), outside);
        EXPECT_EQ(canvas.at(10, 20), moving);
    }

    TEST_F(Composed, ComposeIntoASecondSurfacePaintsItWhole) {
        stripes();

        ttk::Surface other;

        ASSERT_TRUE(other.attach(WIDTH, HEIGHT));

        canvas.root().damage_all();

        EXPECT_EQ(ttk::compose(canvas.root(), other), WINDOW);

        other.present();
        other.detach();
    }
}
