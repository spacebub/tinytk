// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_TESTS_OVERLAYS_TREE_H
#define TTK_TESTS_OVERLAYS_TREE_H

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "support/Sandbox.h"
#include "ttk/dialogs/FilePicker.h"

namespace overlays {
inline std::string tree(const char *name, const int padding = 0) {
    const std::filesystem::path at = std::filesystem::canonical(tests::Sandbox::scratch(name));

    for (const char *folder : {"alpha", "Beta", "dir10", "dir2", ".hidden_dir", "alpha/deeper"}) {
        std::filesystem::create_directories(at / folder);
    }

    for (const char *file :
         {"A.TXT", "b.txt", "file10.txt", "file2.txt", "image.png", "notes.md", ".secret.txt", "alpha/inner.txt"}) {
        std::ofstream(at / file) << "x";
    }

    for (int index = 0; index < padding; ++index) {
        std::ofstream(at / ("pad" + std::to_string(index) + ".txt")) << "x";
    }

    return at.generic_string();
}

inline std::vector<std::string> names(const ttk::FilePicker &picker) {
    std::vector<std::string> out;

    for (const ttk::FilePicker::Row &row : picker.state().entries) {
        out.push_back(row.name);
    }

    return out;
}

inline std::vector<std::string> marked(const ttk::FilePicker &picker) {
    std::vector<std::string> out;

    for (const ttk::FilePicker::Row &row : picker.state().entries) {
        if (row.marked) {
            out.push_back(row.name);
        }
    }

    return out;
}
}

#endif //TTK_TESTS_OVERLAYS_TREE_H
