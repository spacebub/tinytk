// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_BENCH_SANDBOX_H
#define TTK_BENCH_SANDBOX_H

#include <filesystem>

namespace bench {
    // A scratch tree the config, data and cache paths are pointed at, so no
    // benchmark reads or writes the user's own files.
    namespace Sandbox {
        // Before anything reads an environment variable. Paths caches what it finds.
        void enter();

        void leave();

        const std::filesystem::path &root();

        // Empty and recreated. A benchmark that writes gets a clean one.
        std::filesystem::path scratch(const char *name);
    }

}

#endif //TTK_BENCH_SANDBOX_H
