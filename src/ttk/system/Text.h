// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_SYSTEM_TEXT_H
#define TTK_SYSTEM_TEXT_H


#include <string>
#include <string_view>
#include <vector>

namespace ttk {
    namespace Text {

        [[nodiscard]] std::string lower(std::string_view value);

        [[nodiscard]] std::string upper(std::string_view value);

        [[nodiscard]] std::string trim(std::string_view value);

        [[nodiscard]] bool iequals(std::string_view left, std::string_view right);

        [[nodiscard]] bool iends_with(std::string_view value, std::string_view suffix);

        [[nodiscard]] int to_int(std::string_view value, int def = 0);

        [[nodiscard]] bool is_int(std::string_view value);

        [[nodiscard]] std::vector<std::string> split(std::string_view value, char separator);

        [[nodiscard]] std::string join(const std::vector<std::string> &parts, std::string_view separator);

        // Natural order, so MAP2 sorts before MAP10.
        // After David Koelle's Alphanum Algorithm (MIT), http://www.davekoelle.com/alphanum.html
        [[nodiscard]] bool natural_less(std::string_view left, std::string_view right);

        [[nodiscard]] std::vector<std::string> parse_arguments(std::string_view line);

        [[nodiscard]] std::string quote_argument(std::string_view argument);

    }
}


#endif //TTK_SYSTEM_TEXT_H
