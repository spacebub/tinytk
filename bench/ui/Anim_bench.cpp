// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <benchmark/benchmark.h>

#include "ttk/draw/Anim.h"

namespace {
    void Anim_shape(benchmark::State &state) {
        const auto curve = static_cast<ttk::Anim::Curve>(state.range(0));
        float at = 0.0F;

        for ([[maybe_unused]] auto step : state) {
            at += 0.01F;

            if (at > 1.0F) {
                at = 0.0F;
            }

            benchmark::DoNotOptimize(ttk::Anim::shape(curve, at));
        }
    }

    BENCHMARK(Anim_shape)
        ->Arg(static_cast<int>(ttk::Anim::Curve::Linear))
        ->Arg(static_cast<int>(ttk::Anim::Curve::CubicOut))
        ->Arg(static_cast<int>(ttk::Anim::Curve::BackOut));

    void Anim_run(benchmark::State &state) {
        ttk::Anim::Tween tween;
        double now = 0.0;

        for ([[maybe_unused]] auto step : state) {
            now += 1.0 / 280.0;

            tween.run(1.0F, now, 0.18, ttk::Anim::Curve::CubicOut);

            benchmark::DoNotOptimize(tween);
        }
    }

    BENCHMARK(Anim_run);

    void Anim_toward(benchmark::State &state) {
        ttk::Anim::Tween tween;
        double now = 0.0;
        float goal = 1.0F;

        for ([[maybe_unused]] auto step : state) {
            now += 1.0 / 280.0;
            goal = goal > 0.5F ? 0.0F : 1.0F;

            tween.toward(goal, now, 0.18, ttk::Anim::Curve::CubicOut);

            benchmark::DoNotOptimize(tween);
        }
    }

    BENCHMARK(Anim_toward);

    // One tween stepped at the refresh rate, which every live widget does per frame.
    void Anim_advance(benchmark::State &state) {
        ttk::Anim::Tween tween;
        double now = 0.0;

        tween.run(1.0F, now, 1e9, ttk::Anim::Curve::CubicOut);

        for ([[maybe_unused]] auto step : state) {
            now += 1.0 / 280.0;

            tween.advance(now);

            benchmark::DoNotOptimize(tween.value());
        }
    }

    BENCHMARK(Anim_advance);

    void Anim_advance_still(benchmark::State &state) {
        ttk::Anim::Tween tween;
        double now = 0.0;

        tween.set(1.0F);

        for ([[maybe_unused]] auto step : state) {
            now += 1.0 / 280.0;

            tween.advance(now);

            benchmark::DoNotOptimize(tween.live());
        }
    }

    BENCHMARK(Anim_advance_still);
}
