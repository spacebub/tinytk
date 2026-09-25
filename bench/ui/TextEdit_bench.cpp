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

#include <benchmark/benchmark.h>

#include "ttk/toolkit/controls/TextEdit.h"
#include "support/Canvas.h"
#include "support/Fixtures.h"

namespace {
    std::string lines_of(const int lines) {
        std::string out;

        for (const std::string &line : bench::Fixtures::lines(lines)) {
            out += line;
            out += '\n';
        }

        return out;
    }

    // Text set whole: split into lines and measured.
    void TextEdit_set_text(benchmark::State &state) {
        const auto count = static_cast<int>(state.range(0));
        bench::Canvas canvas(1280, 800);
        ttk::TextEdit *edit = bench::mount(canvas, std::make_unique<ttk::TextEdit>(), 960.0, 640.0);
        const std::string text = lines_of(count);

        for ([[maybe_unused]] auto step : state) {
            edit->set_text(text);
        }

        state.SetItemsProcessed(state.iterations() * count);
    }

    BENCHMARK(TextEdit_set_text)->Arg(200)->Arg(2000);

    // A screenful painted again, as the caret blinking asks for twice a second.
    // With line numbers and wrapping on, then off.
    void TextEdit_paint(benchmark::State &state) {
        bench::Canvas canvas(1280, 800);
        ttk::TextEdit *edit = bench::mount(canvas, std::make_unique<ttk::TextEdit>(), 960.0, 640.0);

        edit->numbered(state.range(0) != 0)->wrapped(state.range(0) != 0);
        edit->set_text(lines_of(2000));
        bench::paint_once(canvas, *edit);

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(bench::paint_once(canvas, *edit));
        }
    }

    BENCHMARK(TextEdit_paint)->Arg(1)->Arg(0);

    // Every line broken to the width again, as a resize asks for.
    void TextEdit_rewrap(benchmark::State &state) {
        bench::Canvas canvas(1280, 800);
        ttk::TextEdit *edit = bench::mount(canvas, std::make_unique<ttk::TextEdit>(), 960.0, 640.0);

        edit->set_text(lines_of(2000));

        for ([[maybe_unused]] auto step : state) {
            edit->wrapped(true);
            edit->wrapped(false);
        }
    }

    BENCHMARK(TextEdit_rewrap);

    // A character typed and the frame that shows it: one line changes, the rest of
    // the screen is as it was.
    void TextEdit_type(benchmark::State &state) {
        bench::Canvas canvas(1280, 800);
        ttk::TextEdit *edit = bench::mount(canvas, std::make_unique<ttk::TextEdit>(), 960.0, 640.0);

        edit->set_text(lines_of(2000));
        canvas.full();

        for ([[maybe_unused]] auto step : state) {
            edit->wrote("x");
            benchmark::DoNotOptimize(canvas.frame());
        }
    }

    BENCHMARK(TextEdit_type);
}
