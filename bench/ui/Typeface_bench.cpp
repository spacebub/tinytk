// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "support/Canvas.h"
#include "support/Fixtures.h"

namespace {
    const std::vector<std::string> &labels() {
        static const std::vector<std::string> made = bench::Fixtures::lines(512);

        return made;
    }

    // Warm: the run has been shaped and its mask rasterised already.
    void Typeface_width_cached(benchmark::State &state) {
        ttk::Typeface &type = bench::fonts();
        const BLFont &font = type.at(ttk::Typeface::regular, ttk::Theme::fontBody);
        const std::string run = "Knee Deep in the Dead";

        benchmark::DoNotOptimize(type.width(font, run));

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(type.width(font, run));
        }
    }

    BENCHMARK(Typeface_width_cached);

    // A 512-run working set, as a full page of rows has. The cache holds them all.
    void Typeface_width_many_runs(benchmark::State &state) {
        ttk::Typeface &type = bench::fonts();
        const BLFont &font = type.at(ttk::Typeface::regular, ttk::Theme::fontBody);
        const std::vector<std::string> &runs = labels();
        size_t at = 0;

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(type.width(font, runs[at++ % runs.size()]));
        }
    }

    BENCHMARK(Typeface_width_many_runs);

    void Typeface_elide(benchmark::State &state) {
        ttk::Typeface &type = bench::fonts();
        const BLFont &font = type.at(ttk::Typeface::regular, ttk::Theme::fontBody);
        const std::string run = "/games/doom/addons/Eviternity II RC1.wad";

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(type.elide(font, run, 180.0F));
        }
    }

    BENCHMARK(Typeface_elide);

    void Typeface_at(benchmark::State &state) {
        ttk::Typeface &type = bench::fonts();

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(&type.at(ttk::Typeface::semibold, ttk::Theme::fontLarge));
        }
    }

    BENCHMARK(Typeface_at);

    void Typeface_line_height(benchmark::State &state) {
        ttk::Typeface &type = bench::fonts();
        const BLFont &font = type.at(ttk::Typeface::regular, ttk::Theme::fontBody);

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(type.line_height(font));
        }
    }

    BENCHMARK(Typeface_line_height);

    // The cached A8 mask laid down. This is what a resting label costs per frame.
    void Typeface_draw_cached(benchmark::State &state) {
        bench::Canvas canvas(400, 60);

        ttk::Typeface &type = canvas.type();
        const BLFont &font = type.at(ttk::Typeface::regular, ttk::Theme::fontBody);
        const std::string run = "Knee Deep in the Dead";

        type.draw(canvas.context(), font, BLPoint{8.0, 8.0}, run, ttk::Theme::of().text);

        for ([[maybe_unused]] auto step : state) {
            type.draw(canvas.context(), font, BLPoint{8.0, 8.0}, run, ttk::Theme::of().text);
        }

        canvas.context().flush(BL_CONTEXT_FLUSH_SYNC);
    }

    BENCHMARK(Typeface_draw_cached);

    void Typeface_draw_many_runs(benchmark::State &state) {
        bench::Canvas canvas(400, 60);

        ttk::Typeface &type = canvas.type();
        const BLFont &font = type.at(ttk::Typeface::regular, ttk::Theme::fontBody);
        const std::vector<std::string> &runs = labels();
        size_t at = 0;

        for ([[maybe_unused]] auto step : state) {
            type.draw(canvas.context(), font, BLPoint{8.0, 8.0}, runs[at++ % runs.size()],
                      ttk::Theme::of().text);
        }

        canvas.context().flush(BL_CONTEXT_FLUSH_SYNC);
    }

    BENCHMARK(Typeface_draw_many_runs);

    void Typeface_draw_tracked(benchmark::State &state) {
        bench::Canvas canvas(400, 60);

        ttk::Typeface &type = canvas.type();
        const BLFont &font = type.at(ttk::Typeface::semibold, ttk::Theme::fontTiny);
        const std::string run = "LIBRARY";

        for ([[maybe_unused]] auto step : state) {
            type.draw_tracked(canvas.context(), font, BLPoint{8.0, 8.0}, run, ttk::Theme::of().muted, 1.2F);
        }

        canvas.context().flush(BL_CONTEXT_FLUSH_SYNC);
    }

    BENCHMARK(Typeface_draw_tracked);

    void Typeface_width_tracked(benchmark::State &state) {
        ttk::Typeface &type = bench::fonts();
        const BLFont &font = type.at(ttk::Typeface::semibold, ttk::Theme::fontTiny);
        const std::string run = "LIBRARY";

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(type.width_tracked(font, run, 1.2F));
        }
    }

    BENCHMARK(Typeface_width_tracked);

    // A page of labels, which is the shape of a real frame's text cost.
    void Typeface_page_of_labels(benchmark::State &state) {
        bench::Canvas canvas(1280, 800);

        ttk::Typeface &type = canvas.type();
        const BLFont &font = type.at(ttk::Typeface::regular, ttk::Theme::fontBody);
        const auto count = static_cast<int>(state.range(0));
        const std::vector<std::string> &runs = labels();

        for (int at = 0; at < count; ++at) {
            type.draw(canvas.context(), font, BLPoint{8.0, 8.0}, runs[static_cast<size_t>(at)],
                      ttk::Theme::of().text);
        }

        for ([[maybe_unused]] auto step : state) {
            for (int at = 0; at < count; ++at) {
                type.draw(canvas.context(), font,
                          BLPoint{8.0, static_cast<double>((at % 40) * 20)},
                          runs[static_cast<size_t>(at)], ttk::Theme::of().text);
            }

            canvas.context().flush(BL_CONTEXT_FLUSH_SYNC);
        }

        state.SetItemsProcessed(state.iterations() * count);
    }

    BENCHMARK(Typeface_page_of_labels)->Arg(20)->Arg(80)->Arg(200);
}
