// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <array>
#include <format>
#include <utility>

#include "ttk/system/Paths.h"
#include "ttk/util/Format.h"

namespace ttk {
    namespace {

        const std::string &home_prefix() {
            static const std::string home = Format::from_path(Paths::home_directory()) + "/";

            return home;
        }

    }

    namespace Format {

        std::string pretty_path(const std::string &path) {
            const std::string &home = home_prefix();

            return home.size() > 1 && path.starts_with(home) ? "~" + path.substr(home.size() - 1) : path;
        }

        std::string fit_path(const std::string &path, const int room) {
            std::string pretty = pretty_path(path);

            if (room <= 0 || std::cmp_less_equal(pretty.size(), room)) {
                return pretty;
            }

            for (size_t at = pretty.find('/'); at != std::string::npos; at = pretty.find('/', at + 1)) {
                // The ellipsis is three bytes but one column.
                if (std::string candidate = "…/" + pretty.substr(at + 1);
                    std::cmp_less_equal(candidate.size() - 2, room - 1)) {
                    return candidate;
                }
            }

            return pretty;
        }

        std::string directory_of(const std::string &path) {
            return from_path(std::filesystem::path(path).parent_path());
        }

        std::string file_name(const std::string &path) {
            return from_path(std::filesystem::path(path).filename());
        }

        bool is_file(const std::string &path) {
            std::error_code code;

            return std::filesystem::is_regular_file(std::filesystem::path(path), code);
        }

        bool is_directory(const std::string &path) {
            std::error_code code;

            return std::filesystem::is_directory(std::filesystem::path(path), code);
        }

        bool same_file(const std::string &left, const std::string &right) {
            if (left.empty() || right.empty()) {
                return false;
            }

            if (std::error_code code; std::filesystem::equivalent(left, right, code)) {
                return true;
            }

#ifdef _WIN32
            return Text::iequals(std::filesystem::path(left).generic_string(),
                                 std::filesystem::path(right).generic_string());
#else
            return left == right;
#endif
        }

        std::string bytes(const unsigned long long size) {
            constexpr std::array<const char *, 4> UNITS = {"B", "KB", "MB", "GB"};

            auto shown = static_cast<double>(size);
            size_t unit = 0;

            while (shown >= 1024.0 && unit + 1 < UNITS.size()) {
                shown /= 1024.0;
                ++unit;
            }

            return unit == 0 ? std::format("{:.0f} {}", shown, UNITS[unit])
                             : std::format("{:.1f} {}", shown, UNITS[unit]);
        }

    }
}
