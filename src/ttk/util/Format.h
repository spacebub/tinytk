// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_UTIL_FORMAT_H
#define TTK_UTIL_FORMAT_H


#include <filesystem>
#include <string>

namespace ttk::Format {

    // The interface expects forward slashes.
    [[nodiscard]] inline std::string from_path(const std::filesystem::path &path) {
        return path.generic_string();
    }

    // Home written as ~.
    [[nodiscard]] std::string pretty_path(const std::string &path);

    // Drops whole leading directories to fit. Zero is no limit.
    [[nodiscard]] std::string fit_path(const std::string &path, int room);

    [[nodiscard]] std::string directory_of(const std::string &path);

    [[nodiscard]] std::string file_name(const std::string &path);

    [[nodiscard]] bool is_file(const std::string &path);
    [[nodiscard]] bool is_directory(const std::string &path);

    // Case and slash insensitive on Windows.
    [[nodiscard]] bool same_file(const std::string &left, const std::string &right);


    // 12.4 MB and the like.
    [[nodiscard]] std::string bytes(unsigned long long size);

}


#endif //TTK_UTIL_FORMAT_H
