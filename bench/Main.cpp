// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include "support/Canvas.h"
#include "support/Sandbox.h"
#include "ttk/system/Paths.h"

int main(int argc, char **argv) {
    bench::Sandbox::enter();

    ttk::Paths::set_application("tinytk-bench");

    if (argc > 0) {
        ttk::Paths::set_executable(argv[0]);
    }

    if (!bench::fonts_loaded()) {
        std::fputs("No system face could be loaded, and the interface benchmarks need one.\n", stderr);

        return 1;
    }

    // The caches these share are warm in a running interface and cold in the first
    // iteration here. Warming them is what makes a number the same alone as in the run.
    std::vector<char *> args(argv, argv + argc);
    bool warmup = false;

    for (const char *arg : args) {
        warmup = warmup || std::strstr(arg, "--benchmark_min_warmup_time") != nullptr;
    }

    std::string held = "--benchmark_min_warmup_time=0.3";

    if (!warmup) {
        args.push_back(held.data());
    }

    argc = static_cast<int>(args.size());
    argv = args.data();

    benchmark::Initialize(&argc, argv);

    if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();

    bench::Sandbox::leave();

    return 0;
}
