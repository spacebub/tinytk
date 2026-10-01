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

//! Byte string helpers: ASCII case handling, number parsing, splitting, natural ordering, UTF-8 counting and
//! command line quoting.
namespace ttk::Text {

    //! Returns `value` with the ASCII letters `A` to `Z` lowered. Every other byte is copied unchanged.
    [[nodiscard]] std::string lower(std::string_view value);

    //! Returns `value` with the ASCII letters `a` to `z` raised. Every other byte is copied unchanged.
    [[nodiscard]] std::string upper(std::string_view value);

    //! Returns `value` without leading and trailing ASCII whitespace (space, tab, CR, LF, form feed and
    //! vertical tab). Returns an empty string when `value` holds only whitespace.
    [[nodiscard]] std::string trim(std::string_view value);

    //! Tests whether `left` and `right` are equal when ASCII letters are compared without case.
    [[nodiscard]] bool iequals(std::string_view left, std::string_view right);

    //! Tests whether `value` ends with `suffix` when ASCII letters are compared without case.
    [[nodiscard]] bool iends_with(std::string_view value, std::string_view suffix);

    //! Returns `value` parsed as a decimal `int`, or `def` when it is not one.
    //!
    //! Surrounding whitespace is ignored. A leading `-` is accepted, a leading `+` is not, and the whole of the
    //! trimmed text must be the number. A value out of the range of `int` returns `def`.
    [[nodiscard]] int to_int(std::string_view value, int def = 0);

    //! Tests whether \ref to_int() would parse `value` as a number rather than fall back to its default.
    [[nodiscard]] bool is_int(std::string_view value);

    //! Returns the pieces of `value` between each `separator`.
    //!
    //! Empty pieces are kept, so `"a,,b"` gives three parts and an empty `value` gives none.
    [[nodiscard]] std::vector<std::string> split(std::string_view value, char separator);

    //! Returns `parts` concatenated with `separator` between each pair.
    [[nodiscard]] std::string join(const std::vector<std::string> &parts, std::string_view separator);

    //! Tests whether `left` sorts before `right` in natural order, so `MAP2` sorts before `MAP10`.
    //!
    //! Runs of digits compare by value, read at most nine significant digits at a time, so leading zeros are
    //! ignored. A digit sorts before any other byte, and other bytes compare by value, so case matters. A string
    //! sorts after any of its prefixes.
    //! After David Koelle's Alphanum Algorithm (MIT), http://www.davekoelle.com/alphanum.html
    [[nodiscard]] bool natural_less(std::string_view left, std::string_view right);

    //! Returns the number of UTF-8 characters in `value`, counting every byte that is not a continuation byte.
    [[nodiscard]] size_t characters(std::string_view value);

    //! Tests whether every byte of `value` takes one column, which holds when it is ASCII without a tab.
    [[nodiscard]] bool simple(std::string_view value);

    //! Returns the index of the last `letter` in `value`, or `std::string_view::npos` when there is none.
    [[nodiscard]] size_t last_of(std::string_view value, char letter);

    //! Splits a command line into arguments following shell rules.
    //!
    //! Arguments are separated by spaces, tabs and line breaks. Single quotes keep their content literally.
    //! Double quotes allow `\` escapes and expand `$NAME` and `${NAME}` from the environment, as does unquoted
    //! text, where `\` escapes the next byte. An unset variable expands to nothing and a lone `$` stays as is.
    //! An unterminated quote runs to the end of `line`.
    [[nodiscard]] std::vector<std::string> parse_arguments(std::string_view line);

    //! Returns `argument` quoted for a Windows command line.
    //!
    //! An argument that is not empty and holds no whitespace, quotes, backslashes or `$` is returned unchanged.
    //! Otherwise it is wrapped in double quotes, embedded quotes are escaped, and backslashes are doubled only
    //! where they precede a quote or the closing quote, which is how Windows splits its command lines.
    [[nodiscard]] std::string quote_argument(std::string_view argument);
}


#endif //TTK_SYSTEM_TEXT_H
