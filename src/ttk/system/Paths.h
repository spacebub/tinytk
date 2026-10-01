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

//! Locations of the running executable and the application's per-user and system-wide directories.
namespace ttk::Paths {
    //! Sets the directory name the application keeps its files under. Defaults to `tinytk`.
    //!
    //! Call it before anything asks for \ref config_directory(), \ref system_config_directory() or
    //! \ref data_directory(), since they all end in this name.
    void set_application(const std::string &name);

    //! Returns the directory name set by \ref set_application(), `tinytk` when it was never set.
    [[nodiscard]] const std::string &application();

    //! Sets the path of the running executable, normally `argv[0]`. The path is made canonical when the
    //! file system allows it and stored as given otherwise.
    void set_executable(const std::filesystem::path &path);

    //! Returns the path set by \ref set_executable(), or an empty path when it was never set.
    [[nodiscard]] const std::filesystem::path &executable();

    //! Returns the directory holding \ref executable(), or the current directory when no executable was set.
    [[nodiscard]] std::filesystem::path executable_directory();

    //! Returns the user's home directory, or an empty path when it cannot be found.
    //!
    //! Reads `USERPROFILE`, then `HOMEDRIVE` with `HOMEPATH` on Windows. Elsewhere reads `HOME`, then falls back
    //! to the password database.
    [[nodiscard]] std::filesystem::path home_directory();

    //! Returns the per-user configuration directory of the application, or an empty path when it cannot be found.
    //!
    //! That is `$XDG_CONFIG_HOME/<application>`, or `~/.config/<application>` when the variable is unset. On
    //! Windows it is `%APPDATA%\<application>`, falling back to `<application>` beside the executable.
    [[nodiscard]] std::filesystem::path config_directory();

    //! Returns the system-wide configuration directory of the application, or an empty path when it cannot be
    //! found.
    //!
    //! That is the last entry of `$XDG_CONFIG_DIRS` followed by `<application>`, with `/etc/xdg` used when the
    //! variable is unset. On Windows it is `%PROGRAMDATA%\<application>`.
    [[nodiscard]] std::filesystem::path system_config_directory();

    //! Returns the per-user data directory of the application, or an empty path when it cannot be found.
    //!
    //! That is `$XDG_DATA_HOME/<application>`, or `~/.local/share/<application>` when the variable is unset. On
    //! Windows it is the same directory as \ref config_directory().
    [[nodiscard]] std::filesystem::path data_directory();
}


#endif //TTK_SYSTEM_PATHS_H
