// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_SYSTEM_PATHS_H
#define TTK_SYSTEM_PATHS_H


#include <filesystem>
#include <string>

namespace ttk::Paths {
    // The directory name the application keeps its files under. Set before anything asks.
    void set_application(const std::string &name);
    [[nodiscard]] const std::string &application();

    void set_executable(const std::filesystem::path &path);
    [[nodiscard]] const std::filesystem::path &executable();
    [[nodiscard]] std::filesystem::path executable_directory();

    // Empty when the environment does not say.
    [[nodiscard]] std::filesystem::path home_directory();

    [[nodiscard]] std::filesystem::path config_directory();
    [[nodiscard]] std::filesystem::path system_config_directory();
    [[nodiscard]] std::filesystem::path data_directory();
}


#endif //TTK_SYSTEM_PATHS_H
