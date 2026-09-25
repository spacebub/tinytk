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

#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/controls/TextView.h"
#include "ttk/toolkit/layout/Scroll.h"
#include "support/Canvas.h"
#include "support/Fixtures.h"

namespace {
    // How many rows a console keeps. Past it the oldest go, as a run's output does.
    constexpr size_t CAP = 2000;

    const std::vector<std::string> &pool() {
        static const std::vector<std::string> made = bench::Fixtures::lines(4096);

        return made;
    }

    // A console as an application holds one: a view of a run's output in a scroller,
    // capped, and kept at its end as the output arrives.
    struct Console {
        bench::Canvas canvas{1280, 800};
        ttk::Scroll *scroll = nullptr;
        ttk::TextView *view = nullptr;
        size_t next = 0;

        Console() {
            auto held = std::make_unique<ttk::Scroll>();

            view = static_cast<ttk::TextView *>(held->hold(std::make_unique<ttk::TextView>()));
            view->face(ttk::Typeface::mono, ttk::Theme::fontSmall);

            scroll = bench::mount(canvas, std::move(held), 960.0, 640.0);
        }

        // One batch of output: `count` lines land, the oldest go past the cap, and the
        // view is measured again where it stands.
        void flush(const size_t count) {
            std::string text;

            for (size_t at = 0; at < count; ++at) {
                text += pool()[next++ % pool().size()];
                text += '\n';
            }

            view->append(text);

            const size_t over = view->rows() > CAP ? view->rows() - CAP : 0;

            view->drop_rows(over);
            scroll->refit(canvas.type(), static_cast<double>(over) * view->row_height(canvas.type()));
        }
    };

    // A run pouring output: each iteration is one flush of `count` lines and the frame
    // that shows it. This is what a console costs the window per batch.
    void TextView_stream(benchmark::State &state) {
        const auto count = static_cast<size_t>(state.range(0));
        Console console;

        console.flush(CAP);
        console.canvas.frame();

        for ([[maybe_unused]] auto step : state) {
            console.flush(count);
            benchmark::DoNotOptimize(console.canvas.frame());
        }

        state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(count));
    }

    BENCHMARK(TextView_stream)->Arg(20)->Arg(200)->Arg(2000);

    // The same screenful painted again, as a selection or a scroll asks for.
    void TextView_frame_again(benchmark::State &state) {
        Console console;

        console.flush(CAP);
        console.canvas.frame();

        for ([[maybe_unused]] auto step : state) {
            console.scroll->invalidate();
            benchmark::DoNotOptimize(console.canvas.frame());
        }
    }

    BENCHMARK(TextView_frame_again);

    // The rows alone: taken in, split and the oldest dropped, with nothing painted.
    void TextView_append(benchmark::State &state) {
        const auto count = static_cast<size_t>(state.range(0));
        Console console;

        console.flush(CAP);

        for ([[maybe_unused]] auto step : state) {
            console.flush(count);
        }

        state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(count));
    }

    BENCHMARK(TextView_append)->Arg(200)->Arg(2000);

    // A press lands on a character of a row, which is measured for it.
    void TextView_press(benchmark::State &state) {
        Console console;

        console.flush(CAP);
        console.canvas.frame();

        const BLRect box = console.scroll->box();
        const ttk::Pointer at{.x = box.x + 300.0, .y = box.y + (box.h / 2.0)};

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(console.view->press(at));
        }
    }

    BENCHMARK(TextView_press);

    // One row laid down from glyphs, against Typeface_draw_many_runs, which shapes and
    // rasterises each run whole.
    void Typeface_draw_once_many_runs(benchmark::State &state) {
        bench::Canvas canvas(1280, 60);

        ttk::Typeface &type = canvas.type();
        const BLFont &font = type.at(ttk::Typeface::mono, ttk::Theme::fontSmall);
        size_t at = 0;

        for ([[maybe_unused]] auto step : state) {
            type.draw_once(canvas.context(), font, BLPoint{8.0, 8.0}, pool()[at++ % pool().size()],
                           ttk::Theme::palette().text, 1200.0);
        }

        canvas.context().flush(BL_CONTEXT_FLUSH_SYNC);
    }

    BENCHMARK(Typeface_draw_once_many_runs);
}
