// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <cmath>

#include <gtest/gtest.h>

#include "ttk/draw/Anim.h"

namespace {
    using ttk::Anim::Curve;

    constexpr Curve CURVES[] = {Curve::Linear, Curve::CubicOut, Curve::BackOut};

    // The y of CSS's cubic-bezier(x1, y1, x2, y2) at progress `x`, by bisection on its x.
    double bezier(const double x1, const double y1, const double x2, const double y2, const double x) {
        const auto along = [](const double a, const double b, const double t) {
            const double u = 1.0 - t;

            return (3.0 * u * u * t * a) + (3.0 * u * t * t * b) + (t * t * t);
        };
        double low = 0.0;
        double high = 1.0;

        for (int step = 0; step < 60; ++step) {
            const double mid = (low + high) * 0.5;

            (along(x1, x2, mid) < x ? low : high) = mid;
        }

        return along(y1, y2, (low + high) * 0.5);
    }

    TEST(Anim, EveryCurveIsExactAtItsEnds) {
        for (const Curve curve : CURVES) {
            EXPECT_EQ(ttk::Anim::shape(curve, 0.0F), 0.0F);
            EXPECT_EQ(ttk::Anim::shape(curve, 1.0F), 1.0F);
        }
    }

    TEST(Anim, ProgressIsClampedFirst) {
        for (const Curve curve : CURVES) {
            EXPECT_EQ(ttk::Anim::shape(curve, -0.5F), 0.0F);
            EXPECT_EQ(ttk::Anim::shape(curve, -1e9F), 0.0F);
            EXPECT_EQ(ttk::Anim::shape(curve, 1.5F), 1.0F);
            EXPECT_EQ(ttk::Anim::shape(curve, 1e9F), 1.0F);
        }
    }

    TEST(Anim, LinearPassesProgressThrough) {
        for (const float at : {0.1F, 0.25F, 0.5F, 0.75F, 0.999F}) {
            EXPECT_FLOAT_EQ(ttk::Anim::shape(Curve::Linear, at), at);
        }
    }

    TEST(Anim, CubicOutAtItsMidpoint) {
        EXPECT_NEAR(ttk::Anim::shape(Curve::CubicOut, 0.5F), 0.875F, 1e-6F);
    }

    TEST(Anim, CubicOutFollowsTheCssCurve) {
        for (int step = 1; step < 100; ++step) {
            const float at = static_cast<float>(step) / 100.0F;

            EXPECT_NEAR(ttk::Anim::shape(Curve::CubicOut, at), bezier(0.33, 1.0, 0.68, 1.0, at), 0.02) << "at " << at;
        }
    }

    TEST(Anim, CubicOutRisesWithoutOvershoot) {
        float last = 0.0F;

        for (int step = 1; step <= 1000; ++step) {
            const float eased = ttk::Anim::shape(Curve::CubicOut, static_cast<float>(step) / 1000.0F);

            EXPECT_GE(eased, last);
            EXPECT_LE(eased, 1.0F);
            last = eased;
        }
    }

    TEST(Anim, CubicOutStartsFasterThanLinear) {
        for (const float at : {0.1F, 0.3F, 0.6F, 0.9F}) {
            EXPECT_GT(ttk::Anim::shape(Curve::CubicOut, at), at);
        }
    }

    TEST(Anim, BackOutAtItsMidpoint) {
        EXPECT_NEAR(ttk::Anim::shape(Curve::BackOut, 0.5F), 1.0876975F, 1e-5F);
    }

    TEST(Anim, BackOutOvershootsByAboutTenPercent) {
        float peak = 0.0F;

        for (int step = 1; step < 1000; ++step) {
            peak = std::max(peak, ttk::Anim::shape(Curve::BackOut, static_cast<float>(step) / 1000.0F));
        }

        EXPECT_GT(peak, 1.08F);
        EXPECT_LT(peak, 1.12F);
        EXPECT_NEAR(ttk::Anim::shape(Curve::BackOut, 0.999F), 1.0F, 0.01F) << "did not settle onto the target";
    }

    TEST(Tween, StartsAtZeroAndIdle) {
        const ttk::Anim::Tween tween;

        EXPECT_EQ(tween.value(), 0.0F);
        EXPECT_FALSE(tween.live());
    }

    TEST(Tween, SetJumpsAndStops) {
        ttk::Anim::Tween tween;

        tween.run(10.0F, 0.0, 1.0, Curve::Linear);
        tween.set(4.0F);

        EXPECT_EQ(tween.value(), 4.0F);
        EXPECT_FALSE(tween.live());

        tween.advance(0.5);

        EXPECT_EQ(tween.value(), 4.0F) << "a stopped run moved the value";
    }

    TEST(Tween, RunStartsWithoutMoving) {
        ttk::Anim::Tween tween;

        tween.set(2.0F);
        tween.run(10.0F, 5.0, 2.0, Curve::Linear);

        EXPECT_TRUE(tween.live());
        EXPECT_EQ(tween.value(), 2.0F) << "the value moved before any advance";
    }

    TEST(Tween, LinearRunPassesItsMidpoints) {
        ttk::Anim::Tween tween;

        tween.set(2.0F);
        tween.run(10.0F, 5.0, 2.0, Curve::Linear);
        tween.advance(5.0);

        EXPECT_FLOAT_EQ(tween.value(), 2.0F);

        tween.advance(5.5);

        EXPECT_FLOAT_EQ(tween.value(), 4.0F);

        tween.advance(6.0);

        EXPECT_FLOAT_EQ(tween.value(), 6.0F);
        EXPECT_TRUE(tween.live());
    }

    TEST(Tween, RunEndsOnItsTargetWhenItsDurationElapses) {
        ttk::Anim::Tween tween;

        tween.run(10.0F, 5.0, 2.0, Curve::BackOut);
        tween.advance(7.0);

        EXPECT_EQ(tween.value(), 10.0F);
        EXPECT_FALSE(tween.live());
    }

    TEST(Tween, AdvanceLongAfterTheEndLandsOnTheTarget) {
        ttk::Anim::Tween tween;

        tween.run(-3.0F, 0.0, 0.25, Curve::CubicOut);
        tween.advance(1000.0);

        EXPECT_EQ(tween.value(), -3.0F);
        EXPECT_FALSE(tween.live());
    }

    TEST(Tween, AdvanceBeforeTheStartHoldsTheStartValue) {
        ttk::Anim::Tween tween;

        tween.set(1.0F);
        tween.run(9.0F, 10.0, 1.0, Curve::Linear);
        tween.advance(9.0);

        EXPECT_EQ(tween.value(), 1.0F);
        EXPECT_TRUE(tween.live());
    }

    TEST(Tween, CurveShapesTheRun) {
        ttk::Anim::Tween cubic;
        ttk::Anim::Tween back;

        cubic.run(100.0F, 0.0, 1.0, Curve::CubicOut);
        back.run(100.0F, 0.0, 1.0, Curve::BackOut);
        cubic.advance(0.5);
        back.advance(0.5);

        EXPECT_NEAR(cubic.value(), 87.5F, 1e-3F);
        EXPECT_GT(back.value(), 100.0F) << "BackOut did not overshoot mid run";
    }

    TEST(Tween, RunsDownwardToo) {
        ttk::Anim::Tween tween;

        tween.set(10.0F);
        tween.run(0.0F, 0.0, 4.0, Curve::Linear);
        tween.advance(1.0);

        EXPECT_FLOAT_EQ(tween.value(), 7.5F);
    }

    TEST(Tween, NonPositiveDurationActsLikeSet) {
        for (const double seconds : {0.0, -1.0}) {
            ttk::Anim::Tween tween;

            tween.run(7.0F, 3.0, seconds, Curve::Linear);

            EXPECT_EQ(tween.value(), 7.0F) << seconds;
            EXPECT_FALSE(tween.live()) << seconds;
        }
    }

    TEST(Tween, NewRunStartsWhereTheOldOneGotTo) {
        ttk::Anim::Tween tween;

        tween.run(10.0F, 0.0, 1.0, Curve::Linear);
        tween.advance(0.5);
        tween.run(0.0F, 0.5, 1.0, Curve::Linear);

        EXPECT_FLOAT_EQ(tween.value(), 5.0F);

        tween.advance(1.0);

        EXPECT_FLOAT_EQ(tween.value(), 2.5F);
    }

    TEST(Tween, AdvanceWithoutARunDoesNothing) {
        ttk::Anim::Tween tween;

        tween.set(3.0F);
        tween.advance(100.0);

        EXPECT_EQ(tween.value(), 3.0F);
        EXPECT_FALSE(tween.live());
    }

    TEST(Tween, TowardTheSameTargetDoesNotRestart) {
        ttk::Anim::Tween tween;

        tween.toward(10.0F, 0.0, 1.0, Curve::Linear);
        tween.advance(0.5);
        tween.toward(10.0F, 0.5, 1.0, Curve::Linear);
        tween.advance(1.0);

        EXPECT_EQ(tween.value(), 10.0F) << "the repeated toward restarted the run";
        EXPECT_FALSE(tween.live());
    }

    TEST(Tween, TowardWhereItAlreadyIsDoesNothing) {
        ttk::Anim::Tween tween;

        tween.set(4.0F);
        tween.toward(4.0F, 0.0, 1.0, Curve::Linear);

        EXPECT_FALSE(tween.live());
        EXPECT_EQ(tween.value(), 4.0F);
    }

    TEST(Tween, TowardANewTargetRuns) {
        ttk::Anim::Tween tween;

        tween.toward(10.0F, 0.0, 1.0, Curve::Linear);
        tween.advance(0.5);
        tween.toward(0.0F, 0.5, 1.0, Curve::Linear);

        EXPECT_TRUE(tween.live());

        tween.advance(1.0);

        EXPECT_FLOAT_EQ(tween.value(), 2.5F);
    }

    TEST(Tween, TowardTheStartValueWhileHeadedAwayTurnsBack) {
        ttk::Anim::Tween tween;

        tween.set(0.0F);
        tween.toward(1.0F, 0.0, 1.0, Curve::Linear);
        tween.advance(0.25);
        tween.toward(0.0F, 0.25, 1.0, Curve::Linear);
        tween.advance(1.25);

        EXPECT_EQ(tween.value(), 0.0F);
        EXPECT_FALSE(tween.live());
    }
}
