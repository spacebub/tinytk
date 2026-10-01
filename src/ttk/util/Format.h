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

//! Paths and sizes as the interface shows them.
namespace ttk::Format {

    //! Returns `path` as a string with forward slashes as separators, the form every path in the interface takes.
    [[nodiscard]] inline std::string from_path(const std::filesystem::path &path) {
        return path.generic_string();
    }

    //! Returns `path` with a leading home directory written as `~`.
    //!
    //! Only a `path` with forward slashes that starts with the home directory followed by a slash is changed.
    [[nodiscard]] std::string pretty_path(const std::string &path);

    //! Returns \ref pretty_path() of `path`, with whole leading directories replaced by `…/` until it fits in `room`
    //! characters.
    //!
    //! Lengths count bytes, so non-ASCII names are shortened more than they need to be. Returns the result of
    //! \ref pretty_path() unchanged when `room` is zero or negative, or when even the file name alone does not fit.
    [[nodiscard]] std::string fit_path(const std::string &path, int room);

    //! Returns the parent of `path` with forward slashes, or an empty string when `path` has no parent. A `path`
    //! ending in a slash yields itself without the slash.
    [[nodiscard]] std::string directory_of(const std::string &path);

    //! Returns the last component of `path`, or an empty string when `path` ends in a slash.
    [[nodiscard]] std::string file_name(const std::string &path);

    //! Tests whether `path` names a regular file, following symbolic links. Returns false on any error.
    [[nodiscard]] bool is_file(const std::string &path);

    //! Tests whether `path` names a directory, following symbolic links. Returns false on any error.
    [[nodiscard]] bool is_directory(const std::string &path);

    //! Tests whether `left` and `right` name the same file.
    //!
    //! Returns false when either is empty. Paths the file system reports as equivalent match. Otherwise the strings
    //! are compared, ignoring case and slash direction on Windows and exactly elsewhere, so missing files can match.
    [[nodiscard]] bool same_file(const std::string &left, const std::string &right);


    //! Returns `size` in bytes as a short human-readable string such as `512 B` or `12.4 MB`.
    //!
    //! Units step by 1024 up to GB. Bytes show no decimals, larger units one.
    [[nodiscard]] std::string bytes(unsigned long long size);

}


#endif //TTK_UTIL_FORMAT_H
