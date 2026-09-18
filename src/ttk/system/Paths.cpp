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
#include "ttk/system/Paths.h"

#ifndef _WIN32
#include <pwd.h>
#include <unistd.h>
#endif

namespace ttk::Paths {
    namespace {
        // Function local, so nothing depends on static initialisation order.
        std::string &application_name() {
            static std::string name = "tinytk";

            return name;
        }

        std::filesystem::path &executable_path() {
            static std::filesystem::path path;

            return path;
        }

        std::filesystem::path from_environment(const char *name) {
            const std::string value = Env::get(name);

            return value.empty() ? std::filesystem::path() : std::filesystem::path(value);
        }

#ifndef _WIN32
        // XDG_CONFIG_HOME and the like, or the default under home when unset.
        std::filesystem::path xdg_directory(const char *variable, const std::filesystem::path &fallback) {
            if (std::filesystem::path base = from_environment(variable); !base.empty()) {
                return base;
            }

            const std::filesystem::path home = home_directory();

            return home.empty() ? std::filesystem::path() : home / fallback;
        }
#endif
    }

    void set_application(const std::string &name) {
        application_name() = name;
    }

    const std::string &application() {
        return application_name();
    }

    void set_executable(const std::filesystem::path &path) {
        std::error_code code;
        const std::filesystem::path resolved = std::filesystem::weakly_canonical(path, code);

        executable_path() = code ? path : resolved;
    }

    const std::filesystem::path &executable() {
        return executable_path();
    }

    std::filesystem::path executable_directory() {
        if (executable_path().empty()) {
            std::error_code code;
            const std::filesystem::path here = std::filesystem::current_path(code);

            return code ? std::filesystem::path(".") : here;
        }

        return executable_path().parent_path();
    }

    std::filesystem::path home_directory() {
#ifdef _WIN32
        if (std::filesystem::path profile = from_environment("USERPROFILE"); !profile.empty()) {
            return profile;
        }

        const std::filesystem::path drive = from_environment("HOMEDRIVE");
        const std::filesystem::path rest = from_environment("HOMEPATH");

        return drive.empty() || rest.empty() ? std::filesystem::path() : drive / rest;
#else
        if (std::filesystem::path home = from_environment("HOME"); !home.empty()) {
            return home;
        }

        // NOLINTNEXTLINE(concurrency-mt-unsafe): asked once, before any thread starts.
        const passwd *entry = getpwuid(getuid());

        return entry != nullptr && entry->pw_dir != nullptr
            ? std::filesystem::path(entry->pw_dir)
            : std::filesystem::path();
#endif
    }

    std::filesystem::path config_directory() {
#ifdef _WIN32
        if (const std::filesystem::path appData = from_environment("APPDATA"); !appData.empty()) {
            return appData / application();
        }

        return executable_directory() / application();
#else
        const std::filesystem::path base = xdg_directory("XDG_CONFIG_HOME", ".config");

        return base.empty() ? base : base / application();
#endif
    }

    std::filesystem::path system_config_directory() {
#ifdef _WIN32
        const std::filesystem::path programData = from_environment("PROGRAMDATA");

        return programData.empty() ? programData : programData / application();
#else
        // The last entry of XDG_CONFIG_DIRS, /etc/xdg by default.
        const std::string dirs = Env::get("XDG_CONFIG_DIRS");
        std::string base = dirs.empty() ? "/etc/xdg" : dirs;

        if (const size_t last = base.find_last_of(':'); last != std::string::npos) {
            base = base.substr(last + 1);
        }

        return base.empty() ? std::filesystem::path() : std::filesystem::path(base) / application();
#endif
    }

    std::filesystem::path data_directory() {
#ifdef _WIN32
        return config_directory();
#else
        const std::filesystem::path base = xdg_directory("XDG_DATA_HOME", std::filesystem::path(".local") / "share");

        return base.empty() ? base : base / application();
#endif
    }
}
