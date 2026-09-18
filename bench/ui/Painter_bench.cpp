// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <string>

#include <benchmark/benchmark.h>

#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Painter.h"
#include "support/Canvas.h"
#include "support/Fixtures.h"

namespace {
    class Sheet {
    public:
        Sheet() : _canvas(1280, 800),
                  _painter(_canvas.context(), _canvas.type(), BLRectI{0, 0, 1280, 800}) {}

        const ttk::Painter &painter() const { return _painter; }

        bench::Canvas &canvas() { return _canvas; }

        void flush() { _canvas.context().flush(BL_CONTEXT_FLUSH_SYNC); }

    private:
        bench::Canvas _canvas;
        ttk::Painter _painter;
    };

    const std::string &prose() {
        static const std::string made = bench::Fixtures::paragraph(12);

        return made;
    }

    void Painter_fill(benchmark::State &state) {
        Sheet sheet;

        for ([[maybe_unused]] auto step : state) {
            sheet.painter().fill(BLRect{10, 10, 240, 42}, ttk::Theme::of().surface);
        }

        sheet.flush();
    }

    BENCHMARK(Painter_fill);

    void Painter_round(benchmark::State &state) {
        Sheet sheet;

        for ([[maybe_unused]] auto step : state) {
            sheet.painter().round(BLRect{10, 10, 240, 42}, ttk::Theme::radius, ttk::Theme::of().surface);
        }

        sheet.flush();
    }

    BENCHMARK(Painter_round);

    void Painter_outline(benchmark::State &state) {
        Sheet sheet;

        for ([[maybe_unused]] auto step : state) {
            sheet.painter().outline(BLRect{10, 10, 240, 42}, ttk::Theme::radius, 1.0, ttk::Theme::of().border);
        }

        sheet.flush();
    }

    BENCHMARK(Painter_outline);

    void Painter_circle(benchmark::State &state) {
        Sheet sheet;

        for ([[maybe_unused]] auto step : state) {
            sheet.painter().circle(BLPoint{64, 64}, 9.0, ttk::Theme::of().accent);
        }

        sheet.flush();
    }

    BENCHMARK(Painter_circle);

    void Painter_needed(benchmark::State &state) {
        Sheet sheet;

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(sheet.painter().needed(BLRect{10, 10, 240, 42}));
        }
    }

    BENCHMARK(Painter_needed);

    void Painter_width(benchmark::State &state) {
        Sheet sheet;

        const BLFont &font = sheet.painter().font(ttk::Typeface::regular, ttk::Theme::fontBody);
        const std::string run = "Ultra-Violence";

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(sheet.painter().width(font, run));
        }
    }

    BENCHMARK(Painter_width);

    void Painter_elide(benchmark::State &state) {
        Sheet sheet;

        const BLFont &font = sheet.painter().font(ttk::Typeface::regular, ttk::Theme::fontBody);
        const std::string run = "/games/doom/addons/Eviternity II RC1.wad";

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(sheet.painter().elide(font, run, 180.0));
        }
    }

    BENCHMARK(Painter_elide);

    void Painter_text(benchmark::State &state) {
        Sheet sheet;

        const BLFont &font = sheet.painter().font(ttk::Typeface::regular, ttk::Theme::fontBody);
        const std::string run = "Knee Deep in the Dead";

        for ([[maybe_unused]] auto step : state) {
            sheet.painter().text(font, BLPoint{10, 10}, run, ttk::Theme::of().text);
        }

        sheet.flush();
    }

    BENCHMARK(Painter_text);

    // Measured, elided and drawn: what a row of a list costs.
    void Painter_label(benchmark::State &state) {
        Sheet sheet;

        const BLFont &font = sheet.painter().font(ttk::Typeface::regular, ttk::Theme::fontBody);
        const std::string run = "/games/doom/addons/Eviternity II RC1.wad";

        for ([[maybe_unused]] auto step : state) {
            sheet.painter().label(font, BLRect{10, 10, 240, 24}, ttk::Align::Start, run,
                                  ttk::Theme::of().text);
        }

        sheet.flush();
    }

    BENCHMARK(Painter_label);

    void Painter_tracked(benchmark::State &state) {
        Sheet sheet;

        const BLFont &font = sheet.painter().font(ttk::Typeface::semibold, ttk::Theme::fontTiny);

        for ([[maybe_unused]] auto step : state) {
            sheet.painter().tracked(font, BLPoint{10, 10}, "ENGINES", ttk::Theme::of().muted, 1.2);
        }

        sheet.flush();
    }

    BENCHMARK(Painter_tracked);

    void Painter_paragraph(benchmark::State &state) {
        Sheet sheet;

        const BLFont &font = sheet.painter().font(ttk::Typeface::regular, ttk::Theme::fontBody);

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(
                sheet.painter().paragraph(font, BLRect{10, 10, 420, 600}, prose(), ttk::Theme::of().text));
        }

        sheet.flush();
    }

    BENCHMARK(Painter_paragraph);

    void Painter_wrap_height(benchmark::State &state) {
        Sheet sheet;

        const BLFont &font = sheet.painter().font(ttk::Typeface::regular, ttk::Theme::fontBody);

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(sheet.painter().wrap_height(font, prose(), 420.0));
        }
    }

    BENCHMARK(Painter_wrap_height);

    // A fold is cached, so this is what a caller that has asked before pays.
    void Painter_fold_kept(benchmark::State &state) {
        Sheet sheet;

        ttk::Typeface &type = sheet.canvas().type();
        const BLFont &font = sheet.painter().font(ttk::Typeface::regular, ttk::Theme::fontBody);

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(ttk::fold_spans(type, font, prose(), 420.0).size());
        }
    }

    BENCHMARK(Painter_fold_kept);

    // The width is stepped, so every turn folds afresh.
    void Painter_fold_spans(benchmark::State &state) {
        Sheet sheet;

        ttk::Typeface &type = sheet.canvas().type();
        const BLFont &font = sheet.painter().font(ttk::Typeface::regular, ttk::Theme::fontBody);

        double room = 300.0;

        for ([[maybe_unused]] auto step : state) {
            room = room >= 520.0 ? 300.0 : room + 0.25;

            benchmark::DoNotOptimize(ttk::fold_spans(type, font, prose(), room).size());
        }
    }

    BENCHMARK(Painter_fold_spans);

    void Painter_push_pop(benchmark::State &state) {
        Sheet sheet;

        for ([[maybe_unused]] auto step : state) {
            sheet.painter().push(BLRect{10, 10, 400, 400});
            sheet.painter().pop();
        }
    }

    BENCHMARK(Painter_push_pop);

    // A panel as the controls draw one: fill, border and a heading.
    void Painter_panel(benchmark::State &state) {
        Sheet sheet;

        const BLFont &font = sheet.painter().font(ttk::Typeface::semibold, ttk::Theme::fontMedium);

        for ([[maybe_unused]] auto step : state) {
            sheet.painter().round(BLRect{10, 10, 560, 180}, ttk::Theme::radius, ttk::Theme::of().surface);
            sheet.painter().outline(BLRect{10, 10, 560, 180}, ttk::Theme::radius, 1.0, ttk::Theme::of().border);
            sheet.painter().label(font, BLRect{26, 22, 300, 24}, ttk::Align::Start, "Multiplayer",
                                  ttk::Theme::of().text);
        }

        sheet.flush();
    }

    BENCHMARK(Painter_panel);
}
