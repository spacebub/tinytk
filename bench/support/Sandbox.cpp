// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <string>

#include "ttk/system/Env.h"
#include "support/Sandbox.h"

namespace {
    std::filesystem::path &held() {
        static std::filesystem::path path;

        return path;
    }

    void point(const char *name, const std::filesystem::path &at) {
        std::error_code code;

        std::filesystem::create_directories(at, code);

        ttk::Env::set(name, at.string().c_str());
    }
}

namespace bench::Sandbox {
    void enter() {
        if (!held().empty()) {
            return;
        }

        std::error_code code;

        const std::filesystem::path base =
            std::filesystem::temp_directory_path(code) / "tinytk-bench";

        std::filesystem::remove_all(base, code);
        std::filesystem::create_directories(base, code);

        held() = base;

        point("HOME", base / "home");
        point("XDG_CONFIG_HOME", base / "config");
        point("XDG_DATA_HOME", base / "data");
        point("XDG_CACHE_HOME", base / "cache");
        point("APPDATA", base / "appdata");
        point("USERPROFILE", base / "home");

        ttk::Env::set("XDG_CONFIG_DIRS", (base / "etc").string().c_str());
    }

    void leave() {
        if (held().empty()) {
            return;
        }

        std::error_code code;

        std::filesystem::remove_all(held(), code);

        held().clear();
    }

    const std::filesystem::path &root() {
        return held();
    }

    std::filesystem::path scratch(const char *name) {
        const std::filesystem::path at = held() / "scratch" / name;

        std::error_code code;

        std::filesystem::remove_all(at, code);
        std::filesystem::create_directories(at, code);

        return at;
    }
}
