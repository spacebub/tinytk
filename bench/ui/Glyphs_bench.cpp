// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <benchmark/benchmark.h>

#include "ttk/draw/Glyphs.h"
#include "ttk/draw/Paint.h"
#include "ttk/draw/Svg.h"
#include "ttk/draw/Theme.h"
#include "support/Canvas.h"

namespace {
    // A stand in for a decoded icon, so the blits measure the rasteriser alone.
    const BLImage &artwork() {
        static const BLImage made = [] {
            BLImage image(256, 256, BL_FORMAT_PRGB32);
            BLContext context(image);
            context.fill_all(BLRgba32(0xff3366aa));
            context.fill_circle(128.0, 128.0, 96.0, BLRgba32(0xffffcc66));
            context.end();

            return image;
        }();

        return made;
    }

    constexpr const char *COG =
        "M12 8a4 4 0 100 8 4 4 0 000-8zM12 2v3M12 19v3M2 12h3M19 12h3"
        "M4.9 4.9l2.1 2.1M17 17l2.1 2.1M19.1 4.9L17 7M7 17l-2.1 2.1";

    void Glyphs_draw(benchmark::State &state) {
        bench::Canvas canvas(128, 128);

        const auto which = static_cast<ttk::Glyphs::Glyph>(state.range(0));

        for ([[maybe_unused]] auto step : state) {
            ttk::Glyphs::draw(canvas.context(), which, BLPoint{32, 32}, 1.4F, ttk::Theme::of().text);
        }

        canvas.context().flush(BL_CONTEXT_FLUSH_SYNC);
    }

    BENCHMARK(Glyphs_draw)
        ->Arg(static_cast<int>(ttk::Glyphs::Glyph::Play))
        ->Arg(static_cast<int>(ttk::Glyphs::Glyph::Cog))
        ->Arg(static_cast<int>(ttk::Glyphs::Glyph::Search))
        ->Arg(static_cast<int>(ttk::Glyphs::Glyph::Check));

    void Glyphs_draw_turned(benchmark::State &state) {
        bench::Canvas canvas(128, 128);

        for ([[maybe_unused]] auto step : state) {
            ttk::Glyphs::draw(canvas.context(), ttk::Glyphs::Glyph::Down, BLPoint{32, 32}, 1.4F,
                         ttk::Theme::of().muted, 90.0F);
        }

        canvas.context().flush(BL_CONTEXT_FLUSH_SYNC);
    }

    BENCHMARK(Glyphs_draw_turned);

    void Glyphs_span(benchmark::State &state) {
        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(ttk::Glyphs::span(1.4F));
        }
    }

    BENCHMARK(Glyphs_span);

    void Svg_parse(benchmark::State &state) {
        for ([[maybe_unused]] auto step : state) {
            BLPath path;

            benchmark::DoNotOptimize(ttk::Svg::parse(COG, path));
            benchmark::DoNotOptimize(path);
        }
    }

    BENCHMARK(Svg_parse);

    void Svg_glyph(benchmark::State &state) {
        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(ttk::Svg::glyph(COG, 24.0F, 16.0F));
        }
    }

    BENCHMARK(Svg_glyph);

    // Cached by size and tint. A card's shadow is asked for every frame it moves.
    void Paint_shadow_cached(benchmark::State &state) {
        benchmark::DoNotOptimize(&ttk::Paint::shadow(244, 232, ttk::Theme::radius, 24.0, ttk::Theme::of().shadow));

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(&ttk::Paint::shadow(244, 232, ttk::Theme::radius, 24.0,
                                                    ttk::Theme::of().shadow));
        }
    }

    BENCHMARK(Paint_shadow_cached);

    // A size the cache has not seen, which is every frame of a window being dragged
    // wider: the sprite is filled, blurred over six passes and kept. The two blurs
    // are the ones a card asks for, at rest and lifted.
    void Paint_shadow_built(benchmark::State &state) {
        const auto blur = static_cast<double>(state.range(0));

        int wide = 200;

        for ([[maybe_unused]] auto step : state) {
            wide = wide > 400 ? 200 : wide + 1;

            benchmark::DoNotOptimize(&ttk::Paint::shadow(wide, 232, ttk::Theme::radius, blur,
                                                    ttk::Theme::of().shadow));
        }
    }

    BENCHMARK(Paint_shadow_built)->Arg(4)->Arg(10);

    void Paint_bleed(benchmark::State &state) {
        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(ttk::Paint::bleed(24.0));
        }
    }

    BENCHMARK(Paint_bleed);

    void Paint_down(benchmark::State &state) {
        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(ttk::Paint::down(BLRect{0, 0, 244, 138}));
        }
    }

    BENCHMARK(Paint_down);

    // Resampling is the expensive primitive. This is the art blit on a card.
    void Paint_cover(benchmark::State &state) {
        bench::Canvas canvas(512, 400);

        const BLImage &art = artwork();

        for ([[maybe_unused]] auto step : state) {
            ttk::Paint::cover(canvas.context(), BLRect{0, 0, 244, 138}, art, ttk::Theme::radius);
        }

        canvas.context().flush(BL_CONTEXT_FLUSH_SYNC);
    }

    BENCHMARK(Paint_cover);

    // A whole-pixel 1:1 blit against a fractional one, which costs a resample.
    void Paint_blit_aligned(benchmark::State &state) {
        bench::Canvas canvas(512, 400);

        const BLImage &art = artwork();

        for ([[maybe_unused]] auto step : state) {
            canvas.context().blit_image(BLPoint{16, 16}, art);
        }

        canvas.context().flush(BL_CONTEXT_FLUSH_SYNC);
    }

    BENCHMARK(Paint_blit_aligned);

    void Paint_blit_fractional(benchmark::State &state) {
        bench::Canvas canvas(512, 400);

        const BLImage &art = artwork();

        for ([[maybe_unused]] auto step : state) {
            canvas.context().blit_image(BLPoint{16.5, 16.5}, art);
        }

        canvas.context().flush(BL_CONTEXT_FLUSH_SYNC);
    }

    BENCHMARK(Paint_blit_fractional);
}
