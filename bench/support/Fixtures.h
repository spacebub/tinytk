// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_BENCH_FIXTURES_H
#define TTK_BENCH_FIXTURES_H

#include <filesystem>
#include <string>
#include <vector>

namespace bench::Fixtures {
    // Deterministic, so a number means the same thing on the next commit.
    std::string words(int count, unsigned seed = 1);
    std::vector<std::string> lines(int count);
    std::string paragraph(int sentences);

    // An object holding `groups` groups of `items` entries each, as text and as a
    // file in the sandbox.
    std::string json(int groups, int items);
    const std::filesystem::path &json_file(int groups, int items);
}

#endif //TTK_BENCH_FIXTURES_H
