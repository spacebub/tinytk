// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <memory>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include "support/Canvas.h"
#include "ttk/draw/Damage.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/layout/Wrap.h"

namespace {
    constexpr int WIDE = 1280;
    constexpr int TALL = 800;

    std::vector<BLRect> repeated(const int count) {
        return std::vector<BLRect>(static_cast<size_t>(count), BLRect{200.5, 120.25, 48.0, 24.0});
    }

    // Small rectangles spread over the frame, none touching another.
    std::vector<BLRect> scattered(const int count) {
        std::vector<BLRect> made;

        made.reserve(static_cast<size_t>(count));

        for (int at = 0; at < count; ++at) {
            const int column = at % 20;
            const int row = ((at / 20) * 4) + (at % 4);

            made.emplace_back((column * 64.0) + (at % 5), (row * 20.0) + (at % 3), 18.0, 12.0);
        }

        return made;
    }

    // What a scroll leaves uncovered frame after frame: strips that join into one rectangle.
    std::vector<BLRect> strips(const int count) {
        std::vector<BLRect> made;

        made.reserve(static_cast<size_t>(count));

        for (int at = 0; at < count; ++at) {
            made.emplace_back(0.0, 100.0 + (at * 3.0), 1260.0, 3.0);
        }

        return made;
    }

    // A pointer crossing cards, each report lighting a rectangle a little past the last.
    std::vector<BLRect> sweep(const int count) {
        std::vector<BLRect> made;

        made.reserve(static_cast<size_t>(count));

        for (int at = 0; at < count; ++at) {
            made.emplace_back(40.0 + (at * 9.0), 60.0 + (at * 5.0), 120.0, 80.0);
        }

        return made;
    }

    void add_all(benchmark::State &state, const std::vector<BLRect> &regions) {
        ttk::Damage damage;

        damage.resize(WIDE, TALL);

        for ([[maybe_unused]] auto step : state) {
            damage.clear();

            for (const BLRect &region : regions) {
                damage.add(region);
            }

            benchmark::DoNotOptimize(damage.regions().data());
        }

        state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(regions.size()));
        state.counters["regions"] = static_cast<double>(damage.regions().size());
    }

    void Damage_add_repeated(benchmark::State &state) {
        add_all(state, repeated(100));
    }

    BENCHMARK(Damage_add_repeated);

    void Damage_add_scattered(benchmark::State &state) {
        add_all(state, scattered(100));
    }

    BENCHMARK(Damage_add_scattered);

    void Damage_add_strips(benchmark::State &state) {
        add_all(state, strips(100));
    }

    BENCHMARK(Damage_add_strips);

    // Templated so the body is only checked where Damage::shift exists.
    template <typename Set>
    void shift_all(benchmark::State &state) {
        if constexpr (requires(Set &set) { set.shift(BLRectI{}, 1); }) {
            const std::vector<BLRect> regions = scattered(24);
            Set damage;

            damage.resize(WIDE, TALL);

            for ([[maybe_unused]] auto step : state) {
                damage.clear();

                for (const BLRect &region : regions) {
                    damage.add(region);
                }

                damage.shift(BLRectI{0, 40, WIDE, TALL - 80}, -3);
                benchmark::DoNotOptimize(damage.regions().data());
            }
        } else {
            state.SkipWithMessage("Damage::shift is not available");
        }
    }

    void Damage_shift(benchmark::State &state) {
        shift_all<ttk::Damage>(state);
    }

    BENCHMARK(Damage_shift);

    void damage_root(benchmark::State &state, const std::vector<BLRect> &regions) {
        ttk::Root root(bench::fonts());

        root.resize(WIDE, TALL);

        size_t taken = 0;

        for ([[maybe_unused]] auto step : state) {
            for (const BLRect &region : regions) {
                root.damage(region);
            }

            taken = root.take().size();
            benchmark::DoNotOptimize(taken);
        }

        state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(regions.size()));
        state.counters["regions"] = static_cast<double>(taken);
    }

    void Root_damage_crowded(benchmark::State &state) {
        damage_root(state, scattered(200));
    }

    BENCHMARK(Root_damage_crowded);

    void Root_damage_take_sweep(benchmark::State &state) {
        damage_root(state, sweep(100));
    }

    BENCHMARK(Root_damage_take_sweep);

    // A page of labels with every third one invalidated, then the frame: what the damage
    // set costs against the repaint it saves.
    void Root_scattered_repaint(benchmark::State &state) {
        bench::Canvas canvas(WIDE, TALL);
        auto held = std::make_unique<ttk::Wrap>();

        held->spacing(8.0, 8.0);

        for (int at = 0; at < 240; ++at) {
            held->add(std::make_unique<ttk::Label>("Card " + std::to_string(at)));
        }

        ttk::Wrap *made = bench::mount(canvas, std::move(held), static_cast<double>(WIDE), static_cast<double>(TALL));

        canvas.full();

        for ([[maybe_unused]] auto step : state) {
            const auto &children = made->children();

            for (size_t at = 0; at < children.size(); at += 3) {
                children[at]->invalidate();
            }

            benchmark::DoNotOptimize(canvas.frame());
        }
    }

    BENCHMARK(Root_scattered_repaint);
}
