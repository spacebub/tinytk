// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include "ttk/system/Text.h"
#include "support/Fixtures.h"

namespace {
    const std::string &sentence() {
        static const std::string made = bench::Fixtures::words(24, 7);

        return made;
    }

    std::vector<std::string> map_names(const int count) {
        std::vector<std::string> names;

        names.reserve(static_cast<size_t>(count));

        for (int at = count; at > 0; --at) {
            names.push_back("MAP" + std::to_string(at));
        }

        return names;
    }

    void Text_lower(benchmark::State &state) {
        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(ttk::Text::lower(sentence()));
        }
    }

    BENCHMARK(Text_lower);

    void Text_upper(benchmark::State &state) {
        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(ttk::Text::upper(sentence()));
        }
    }

    BENCHMARK(Text_upper);

    void Text_trim(benchmark::State &state) {
        const std::string padded = "   \t" + sentence() + " \r\n";

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(ttk::Text::trim(padded));
        }
    }

    BENCHMARK(Text_trim);

    void Text_iequals(benchmark::State &state) {
        const std::string left = "GZDoom (Vulkan)";
        const std::string right = "gzdoom (vulkan)";

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(ttk::Text::iequals(left, right));
        }
    }

    BENCHMARK(Text_iequals);

    void Text_iends_with(benchmark::State &state) {
        const std::string file = "/games/doom2/master/levels/MASTERLEV.WAD";

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(ttk::Text::iends_with(file, ".wad"));
        }
    }

    BENCHMARK(Text_iends_with);

    void Text_to_int(benchmark::State &state) {
        const std::string value = "32767";

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(ttk::Text::to_int(value));
        }
    }

    BENCHMARK(Text_to_int);

    void Text_split(benchmark::State &state) {
        const std::string list = "a.wad;b.wad;c.wad;d.pk3;e.wad;f.deh;g.bex;h.wad";

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(ttk::Text::split(list, ';'));
        }
    }

    BENCHMARK(Text_split);

    void Text_join(benchmark::State &state) {
        const std::vector<std::string> parts = ttk::Text::split(
            "a.wad;b.wad;c.wad;d.pk3;e.wad;f.deh;g.bex;h.wad", ';');

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(ttk::Text::join(parts, ";"));
        }
    }

    BENCHMARK(Text_join);

    // The map list is sorted with this on every warp push.
    void Text_natural_sort(benchmark::State &state) {
        const std::vector<std::string> source = map_names(static_cast<int>(state.range(0)));

        for ([[maybe_unused]] auto step : state) {
            std::vector<std::string> names = source;

            std::ranges::sort(names, ttk::Text::natural_less);

            benchmark::DoNotOptimize(names);
        }

        state.SetItemsProcessed(state.iterations() * state.range(0));
    }

    BENCHMARK(Text_natural_sort)->Arg(32)->Arg(256)->Arg(1024);

    void Text_natural_less(benchmark::State &state) {
        std::string left = "MAP2";
        std::string right = "MAP10";

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(left);
            benchmark::DoNotOptimize(right);
            benchmark::DoNotOptimize(ttk::Text::natural_less(left, right));
        }
    }

    BENCHMARK(Text_natural_less);

    void Text_parse_arguments(benchmark::State &state) {
        const std::string line =
            R"(-iwad "/games/DOOM2.WAD" -file "/addons/Valiant.wad" "/addons/pl2.wad" -skill 4 -warp 07 -complevel 9)";

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(ttk::Text::parse_arguments(line));
        }
    }

    BENCHMARK(Text_parse_arguments);

    void Text_quote_argument(benchmark::State &state) {
        const std::string path = R"(C:\Program Files\GZDoom\gzdoom.exe)";

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(ttk::Text::quote_argument(path));
        }
    }

    BENCHMARK(Text_quote_argument);
}
