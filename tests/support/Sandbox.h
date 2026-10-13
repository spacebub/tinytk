// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_TESTS_SANDBOX_H
#define TTK_TESTS_SANDBOX_H

#include <filesystem>

// A scratch tree of this process alone that the config, data and cache paths
// are pointed at, so no test reads or writes the user's own files.
namespace tests::Sandbox {
    // Before anything reads an environment variable. Paths caches what it finds.
    void enter();

    void leave();

    const std::filesystem::path &root();

    // Empty and recreated. A case that writes gets a clean one.
    std::filesystem::path scratch(const char *name);
}

#endif //TTK_TESTS_SANDBOX_H
