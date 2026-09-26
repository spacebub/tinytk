// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <bit>
#include <charconv>
#include <cstdint>
#include <ranges>

#ifdef __AVX2__
#include <immintrin.h>
#define TTK_TEXT_AVX2
#define TTK_TEXT_SIMD
#elif defined(__SSE2__) || defined(_M_X64)
#include <emmintrin.h>
#define TTK_TEXT_SIMD
#endif

#include "ttk/system/Env.h"
#include "ttk/system/Text.h"

namespace ttk {
    namespace {

        constexpr std::string_view WHITESPACE = " \t\r\n\f\v";

        char lower_char(const char value) {
            return value >= 'A' && value <= 'Z' ? static_cast<char>(value - 'A' + 'a') : value;
        }

        char upper_char(const char value) {
            return value >= 'a' && value <= 'z' ? static_cast<char>(value - 'a' + 'A') : value;
        }

        bool is_digit(const char value) {
            return value >= '0' && value <= '9';
        }

        bool is_identifier_char(const char value) {
            return is_digit(value) || value == '_'
                || (value >= 'a' && value <= 'z')
                || (value >= 'A' && value <= 'Z');
        }

        bool is_continuation(const char value) {
            return (static_cast<unsigned char>(value) & 0xc0U) == 0x80U;
        }

#ifdef TTK_TEXT_SIMD
        // The scans below go a vector at a time: thirty two bytes under AVX2 and
        // sixteen under SSE2, with the bits of a byte comparison read as a mask.
        struct Bytes {
#ifdef TTK_TEXT_AVX2
            using V = __m256i;

            static constexpr size_t WIDE = 32;

            static V load(const char *at) { return _mm256_loadu_si256(reinterpret_cast<const V *>(at)); }
            static V set1(const char value) { return _mm256_set1_epi8(value); }
            static V both(const V a, const V b) { return _mm256_and_si256(a, b); }
            static V either(const V a, const V b) { return _mm256_or_si256(a, b); }
            static V equal(const V a, const V b) { return _mm256_cmpeq_epi8(a, b); }
            static std::uint32_t bits(const V v) { return static_cast<std::uint32_t>(_mm256_movemask_epi8(v)); }
#else
            using V = __m128i;

            static constexpr size_t WIDE = 16;

            static V load(const char *at) { return _mm_loadu_si128(reinterpret_cast<const V *>(at)); }
            static V set1(const char value) { return _mm_set1_epi8(value); }
            static V both(const V a, const V b) { return _mm_and_si128(a, b); }
            static V either(const V a, const V b) { return _mm_or_si128(a, b); }
            static V equal(const V a, const V b) { return _mm_cmpeq_epi8(a, b); }
            static std::uint32_t bits(const V v) { return static_cast<std::uint32_t>(_mm_movemask_epi8(v)); }
#endif
        };
#endif

    }

    namespace Text {

        std::string lower(const std::string_view value) {
            std::string out(value);
            std::ranges::transform(out, out.begin(), lower_char);

            return out;
        }

        std::string upper(const std::string_view value) {
            std::string out(value);
            std::ranges::transform(out, out.begin(), upper_char);

            return out;
        }

        std::string trim(const std::string_view value) {
            const size_t first = value.find_first_not_of(WHITESPACE);

            if (first == std::string_view::npos) {
                return {};
            }

            const size_t last = value.find_last_not_of(WHITESPACE);

            return std::string(value.substr(first, last - first + 1));
        }

        bool iequals(const std::string_view left, const std::string_view right) {
            return left.size() == right.size()
                && std::ranges::equal(left, right, [](const char one, const char two) {
                       return lower_char(one) == lower_char(two);
                   });
        }

        bool iends_with(const std::string_view value, const std::string_view suffix) {
            return value.size() >= suffix.size()
                && iequals(value.substr(value.size() - suffix.size()), suffix);
        }

        int to_int(const std::string_view value, const int def) {
            const std::string trimmed = trim(value);
            int out = 0;

            const char *begin = trimmed.data();
            const char *end = begin + trimmed.size();
            const auto [stopped, code] = std::from_chars(begin, end, out);

            return code == std::errc() && stopped == end ? out : def;
        }

        bool is_int(const std::string_view value) {
            const std::string trimmed = trim(value);

            if (trimmed.empty()) {
                return false;
            }

            int out = 0;
            const char *begin = trimmed.data();
            const char *end = begin + trimmed.size();
            const auto [stopped, code] = std::from_chars(begin, end, out);

            return code == std::errc() && stopped == end;
        }

        std::vector<std::string> split(const std::string_view value, const char separator) {
            return value | std::views::split(separator)
                | std::ranges::to<std::vector<std::string>>();
        }

        std::string join(const std::vector<std::string> &parts, const std::string_view separator) {
            return parts | std::views::join_with(separator) | std::ranges::to<std::string>();
        }

        bool natural_less(const std::string_view left, const std::string_view right) {
            size_t li = 0;
            size_t ri = 0;

            while (li < left.size() && ri < right.size()) {
                const bool leftDigit = is_digit(left[li]);
                const bool rightDigit = is_digit(right[ri]);

                if (leftDigit && rightDigit) {
                    // Nine significant digits at most, so the values cannot overflow.
                    unsigned int leftValue = 0;
                    unsigned int rightValue = 0;
                    unsigned int leftDigits = 0;
                    unsigned int rightDigits = 0;

                    while (li < left.size() && is_digit(left[li]) && leftDigits < 9) {
                        leftValue = (leftValue * 10) + static_cast<unsigned int>(left[li] - '0');

                        if (leftValue != 0) {
                            leftDigits++;
                        }

                        ++li;
                    }

                    while (ri < right.size() && is_digit(right[ri]) && rightDigits < 9) {
                        rightValue = (rightValue * 10) + static_cast<unsigned int>(right[ri] - '0');

                        if (rightValue != 0) {
                            rightDigits++;
                        }

                        ++ri;
                    }

                    if (leftValue != rightValue) {
                        return leftValue < rightValue;
                    }

                    continue;
                }

                if (leftDigit) {
                    return true;
                }

                if (rightDigit) {
                    return false;
                }

                if (left[li] != right[ri]) {
                    return left[li] < right[ri];
                }

                ++li;
                ++ri;
            }

            return ri < right.size();
        }

        size_t characters(const std::string_view value) {
            size_t continuations = 0;
            size_t at = 0;

#ifdef TTK_TEXT_SIMD
            const Bytes::V high = Bytes::set1(static_cast<char>(0xc0));
            const Bytes::V lead = Bytes::set1(static_cast<char>(0x80));

            for (; at + Bytes::WIDE <= value.size(); at += Bytes::WIDE) {
                const Bytes::V v = Bytes::load(value.data() + at);

                continuations += static_cast<size_t>(std::popcount(Bytes::bits(Bytes::equal(Bytes::both(v, high), lead))));
            }

            // The rest through one vector over the end, its bits shifted past what
            // the loop already counted.
            if (at < value.size() && value.size() >= Bytes::WIDE) {
                const Bytes::V v = Bytes::load(value.data() + value.size() - Bytes::WIDE);
                const std::uint32_t found = Bytes::bits(Bytes::equal(Bytes::both(v, high), lead));

                continuations += static_cast<size_t>(std::popcount(found >> (Bytes::WIDE - (value.size() - at))));
                at = value.size();
            }
#endif

            for (; at < value.size(); ++at) {
                continuations += is_continuation(value[at]) ? 1 : 0;
            }

            return value.size() - continuations;
        }

        bool simple(const std::string_view value) {
            size_t at = 0;

#ifdef TTK_TEXT_SIMD
            const Bytes::V tab = Bytes::set1('\t');

            for (; at + Bytes::WIDE <= value.size(); at += Bytes::WIDE) {
                const Bytes::V v = Bytes::load(value.data() + at);

                if (Bytes::bits(Bytes::either(v, Bytes::equal(v, tab))) != 0) {
                    return false;
                }
            }

            // The rest through one vector over the end: a byte seen twice changes nothing.
            if (at < value.size() && value.size() >= Bytes::WIDE) {
                const Bytes::V v = Bytes::load(value.data() + value.size() - Bytes::WIDE);

                return Bytes::bits(Bytes::either(v, Bytes::equal(v, tab))) == 0;
            }
#endif

            for (; at < value.size(); ++at) {
                if (value[at] == '\t' || static_cast<unsigned char>(value[at]) >= 0x80U) {
                    return false;
                }
            }

            return true;
        }

        size_t last_of(const std::string_view value, const char letter) {
            size_t end = value.size();

#ifdef TTK_TEXT_SIMD
            const Bytes::V wanted = Bytes::set1(letter);

            for (; end >= Bytes::WIDE; end -= Bytes::WIDE) {
                const std::uint32_t found = Bytes::bits(Bytes::equal(Bytes::load(value.data() + end - Bytes::WIDE), wanted));

                if (found != 0) {
                    return end - Bytes::WIDE + static_cast<size_t>(std::bit_width(found)) - 1;
                }
            }
#endif

            for (; end > 0; --end) {
                if (value[end - 1] == letter) {
                    return end - 1;
                }
            }

            return std::string_view::npos;
        }

        std::vector<std::string> parse_arguments(const std::string_view line) {
            std::vector<std::string> arguments;
            std::string current;
            bool started = false;
            size_t index = 0;

            const auto expand = [&current](const std::string_view name) {
                if (name.empty()) {
                    current.push_back('$');

                    return;
                }

                current.append(Env::get(std::string(name).c_str()));
            };

            while (index < line.size()) {
                const char letter = line[index];

                if (letter == ' ' || letter == '\t' || letter == '\n' || letter == '\r') {
                    if (started) {
                        arguments.push_back(current);
                        current.clear();
                        started = false;
                    }

                    ++index;

                    continue;
                }

                started = true;

                if (letter == '\'') {
                    ++index;

                    while (index < line.size() && line[index] != '\'') {
                        current.push_back(line[index]);
                        ++index;
                    }

                    if (index < line.size()) {
                        ++index;
                    }

                    continue;
                }

                if (letter == '"') {
                    ++index;

                    while (index < line.size() && line[index] != '"') {
                        if (line[index] == '\\' && index + 1 < line.size()) {
                            ++index;
                            current.push_back(line[index]);
                            ++index;

                            continue;
                        }

                        if (line[index] == '$') {
                            ++index;
                            size_t start = index;

                            if (index < line.size() && line[index] == '{') {
                                start = ++index;

                                while (index < line.size() && line[index] != '}') {
                                    ++index;
                                }

                                expand(line.substr(start, index - start));

                                if (index < line.size()) {
                                    ++index;
                                }
                            } else {
                                while (index < line.size() && is_identifier_char(line[index])) {
                                    ++index;
                                }

                                expand(line.substr(start, index - start));
                            }

                            continue;
                        }

                        current.push_back(line[index]);
                        ++index;
                    }

                    if (index < line.size()) {
                        ++index;
                    }

                    continue;
                }

                if (letter == '\\' && index + 1 < line.size()) {
                    ++index;
                    current.push_back(line[index]);
                    ++index;

                    continue;
                }

                if (letter == '$') {
                    ++index;
                    size_t start = index;

                    if (index < line.size() && line[index] == '{') {
                        start = ++index;

                        while (index < line.size() && line[index] != '}') {
                            ++index;
                        }

                        expand(line.substr(start, index - start));

                        if (index < line.size()) {
                            ++index;
                        }
                    } else {
                        while (index < line.size() && is_identifier_char(line[index])) {
                            ++index;
                        }

                        expand(line.substr(start, index - start));
                    }

                    continue;
                }

                current.push_back(letter);
                ++index;
            }

            if (started) {
                arguments.push_back(current);
            }

            return arguments;
        }

        std::string quote_argument(const std::string_view argument) {
            if (!argument.empty() && argument.find_first_of(" \t\n\"'\\$") == std::string_view::npos) {
                return std::string(argument);
            }

            std::string quoted = "\"";
            size_t slashes = 0;

            // Windows argv rules: a backslash only escapes when it precedes a quote.
            for (const char letter : argument) {
                if (letter == '\\') {
                    slashes++;
                    quoted.push_back(letter);
                    continue;
                }

                if (letter == '"') {
                    quoted.append(slashes + 1, '\\');
                }

                slashes = 0;
                quoted.push_back(letter);
            }

            // Trailing backslashes would escape the closing quote.
            quoted.append(slashes, '\\');
            quoted.push_back('"');

            return quoted;
        }

    }
}
