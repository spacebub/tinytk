// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "ttk/system/Env.h"
#include "ttk/system/Text.h"

namespace {
using Strings = std::vector<std::string>;

constexpr std::string_view UMLAUT = "\xC3\xA4";
constexpr std::string_view UMLAUT_UPPER = "\xC3\x84";
constexpr std::string_view JAPANESE = "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E";
constexpr std::string_view GRIN = "\xF0\x9F\x98\x80";

// Longer than two AVX2 vectors, so the scans run their vector loop and their tail.
constexpr std::size_t LONG = 100;

std::string repeat(const std::string_view piece, const std::size_t times) {
    std::string out;

    for (std::size_t i = 0; i < times; ++i) {
        out += piece;
    }

    return out;
}

// Splits a Windows command line the way the C runtime does.
Strings windows_split(const std::string_view line) {
    Strings out;
    std::string current;
    bool quoted = false;
    bool started = false;
    std::size_t at = 0;

    while (at < line.size()) {
        const char letter = line[at];

        if (letter == '\\') {
            std::size_t slashes = 0;

            while (at < line.size() && line[at] == '\\') {
                ++slashes;
                ++at;
            }

            started = true;

            if (at < line.size() && line[at] == '"') {
                current.append(slashes / 2, '\\');

                if (slashes % 2 == 1) {
                    current.push_back('"');
                    ++at;
                }
            } else {
                current.append(slashes, '\\');
            }

            continue;
        }

        if (letter == '"') {
            quoted = !quoted;
            started = true;
            ++at;

            continue;
        }

        if (!quoted && (letter == ' ' || letter == '\t' || letter == '\n')) {
            if (started) {
                out.push_back(current);
                current.clear();
                started = false;
            }

            ++at;

            continue;
        }

        current.push_back(letter);
        started = true;
        ++at;
    }

    if (started) {
        out.push_back(current);
    }

    return out;
}

void unset(const char *name) {
    unsetenv(name); // NOLINT(concurrency-mt-unsafe): the tests run on one thread.
}

class TextParseArguments : public ::testing::Test {
protected:
    void SetUp() override {
        ttk::Env::set("TTK_TEST_WORD", "word");
        ttk::Env::set("TTK_TEST_SPACED", "two words");
        unset("TTK_TEST_UNSET");
    }

    void TearDown() override {
        unset("TTK_TEST_WORD");
        unset("TTK_TEST_SPACED");
    }
};
}

TEST(TextLower, LowersAsciiLetters) {
    EXPECT_EQ(ttk::Text::lower("Hello WORLD"), "hello world");
}

TEST(TextLower, KeepsDigitsAndPunctuation) {
    EXPECT_EQ(ttk::Text::lower("A1-B2_C3.@[`"), "a1-b2_c3.@[`");
}

TEST(TextLower, EmptyStaysEmpty) {
    EXPECT_EQ(ttk::Text::lower(""), "");
}

TEST(TextLower, LeavesMultiByteCharactersAlone) {
    const std::string mixed = std::string("X") + std::string(UMLAUT_UPPER) + std::string(JAPANESE);

    EXPECT_EQ(ttk::Text::lower(mixed), std::string("x") + std::string(UMLAUT_UPPER) + std::string(JAPANESE))
            << "only A to Z are lowered";
}

TEST(TextLower, KeepsEmbeddedNul) {
    const std::string value("A\0B", 3);

    EXPECT_EQ(ttk::Text::lower(value), std::string("a\0b", 3));
}

TEST(TextUpper, RaisesAsciiLetters) {
    EXPECT_EQ(ttk::Text::upper("Hello world"), "HELLO WORLD");
}

TEST(TextUpper, KeepsDigitsAndPunctuation) {
    EXPECT_EQ(ttk::Text::upper("a1-b2_c3.{~"), "A1-B2_C3.{~");
}

TEST(TextUpper, EmptyStaysEmpty) {
    EXPECT_EQ(ttk::Text::upper(""), "");
}

TEST(TextUpper, LeavesMultiByteCharactersAlone) {
    EXPECT_EQ(ttk::Text::upper(std::string("a") + std::string(UMLAUT)), std::string("A") + std::string(UMLAUT));
}

TEST(TextUpper, RoundTripsWithLowerOnAscii) {
    const std::string all = "abcdefghijklmnopqrstuvwxyz";

    EXPECT_EQ(ttk::Text::lower(ttk::Text::upper(all)), all);
}

TEST(TextTrim, RemovesSpacesAtBothEnds) {
    EXPECT_EQ(ttk::Text::trim("  value  "), "value");
}

TEST(TextTrim, RemovesEveryAsciiWhitespaceKind) {
    EXPECT_EQ(ttk::Text::trim(" \t\r\n\f\vvalue\v\f\n\r\t "), "value");
}

TEST(TextTrim, KeepsInteriorWhitespace) {
    EXPECT_EQ(ttk::Text::trim("  a b\tc  "), "a b\tc");
}

TEST(TextTrim, OnlyWhitespaceGivesEmpty) {
    EXPECT_EQ(ttk::Text::trim(" \t\r\n\f\v"), "");
}

TEST(TextTrim, EmptyStaysEmpty) {
    EXPECT_EQ(ttk::Text::trim(""), "");
}

TEST(TextTrim, SingleCharacterIsKept) {
    EXPECT_EQ(ttk::Text::trim(" x "), "x");
    EXPECT_EQ(ttk::Text::trim("x"), "x");
}

TEST(TextTrim, KeepsNonBreakingSpace) {
    const std::string nbsp = "\xC2\xA0";

    EXPECT_EQ(ttk::Text::trim(nbsp + "x" + nbsp), nbsp + "x" + nbsp) << "only ASCII whitespace is trimmed";
}

TEST(TextTrim, KeepsMultiByteContent) {
    EXPECT_EQ(ttk::Text::trim(" " + std::string(JAPANESE) + "\n"), std::string(JAPANESE));
}

TEST(TextIequals, IgnoresAsciiCase) {
    EXPECT_TRUE(ttk::Text::iequals("Hello", "hELLO"));
}

TEST(TextIequals, DifferentLengthsDiffer) {
    EXPECT_FALSE(ttk::Text::iequals("abc", "abcd"));
    EXPECT_FALSE(ttk::Text::iequals("abcd", "abc"));
}

TEST(TextIequals, EmptyEqualsEmpty) {
    EXPECT_TRUE(ttk::Text::iequals("", ""));
    EXPECT_FALSE(ttk::Text::iequals("", "a"));
}

TEST(TextIequals, DifferentLettersDiffer) {
    EXPECT_FALSE(ttk::Text::iequals("abc", "abd"));
}

TEST(TextIequals, DoesNotFoldNonAscii) {
    EXPECT_FALSE(ttk::Text::iequals(UMLAUT, UMLAUT_UPPER));
    EXPECT_TRUE(ttk::Text::iequals(JAPANESE, JAPANESE));
}

TEST(TextIequals, DoesNotMatchNeighbouringPunctuation) {
    EXPECT_FALSE(ttk::Text::iequals("@", "`")) << "@ and ` sit next to A and a";
    EXPECT_FALSE(ttk::Text::iequals("[", "{"));
}

TEST(TextIendsWith, MatchesSuffixWithoutCase) {
    EXPECT_TRUE(ttk::Text::iends_with("photo.PNG", ".png"));
    EXPECT_TRUE(ttk::Text::iends_with("photo.png", ".PnG"));
}

TEST(TextIendsWith, EmptySuffixAlwaysMatches) {
    EXPECT_TRUE(ttk::Text::iends_with("anything", ""));
    EXPECT_TRUE(ttk::Text::iends_with("", ""));
}

TEST(TextIendsWith, LongerSuffixNeverMatches) {
    EXPECT_FALSE(ttk::Text::iends_with("png", ".png"));
    EXPECT_FALSE(ttk::Text::iends_with("", "a"));
}

TEST(TextIendsWith, WholeValueMatches) {
    EXPECT_TRUE(ttk::Text::iends_with("ABC", "abc"));
}

TEST(TextIendsWith, RejectsWrongSuffix) {
    EXPECT_FALSE(ttk::Text::iends_with("photo.jpg", ".png"));
    EXPECT_FALSE(ttk::Text::iends_with("photo.png", "photo"));
}

TEST(TextIendsWith, MatchesMultiByteSuffixExactly) {
    const std::string name = "file" + std::string(JAPANESE);

    EXPECT_TRUE(ttk::Text::iends_with(name, JAPANESE));
    EXPECT_FALSE(ttk::Text::iends_with("x" + std::string(UMLAUT), UMLAUT_UPPER));
}

TEST(TextToInt, ParsesPositive) {
    EXPECT_EQ(ttk::Text::to_int("42"), 42);
}

TEST(TextToInt, ParsesNegative) {
    EXPECT_EQ(ttk::Text::to_int("-17"), -17);
}

TEST(TextToInt, ParsesZero) {
    EXPECT_EQ(ttk::Text::to_int("0", 5), 0);
    EXPECT_EQ(ttk::Text::to_int("-0", 5), 0);
}

TEST(TextToInt, IgnoresSurroundingWhitespace) {
    EXPECT_EQ(ttk::Text::to_int("  12\t\n"), 12);
    EXPECT_EQ(ttk::Text::to_int("\v-3\f"), -3);
}

TEST(TextToInt, IgnoresLeadingZeros) {
    EXPECT_EQ(ttk::Text::to_int("007"), 7);
}

TEST(TextToInt, RejectsLeadingPlus) {
    EXPECT_EQ(ttk::Text::to_int("+5", -1), -1);
}

TEST(TextToInt, RejectsTrailingText) {
    EXPECT_EQ(ttk::Text::to_int("12abc", -1), -1);
    EXPECT_EQ(ttk::Text::to_int("1 2", -1), -1) << "the whole trimmed text must be the number";
    EXPECT_EQ(ttk::Text::to_int("1.5", -1), -1);
}

TEST(TextToInt, RejectsLeadingText) {
    EXPECT_EQ(ttk::Text::to_int("abc12", -1), -1);
    EXPECT_EQ(ttk::Text::to_int("0x10", -1), -1);
}

TEST(TextToInt, EmptyAndBlankGiveDefault) {
    EXPECT_EQ(ttk::Text::to_int("", 9), 9);
    EXPECT_EQ(ttk::Text::to_int("   ", 9), 9);
}

TEST(TextToInt, DefaultDefaultIsZero) {
    EXPECT_EQ(ttk::Text::to_int("nope"), 0);
}

TEST(TextToInt, LoneSignsGiveDefault) {
    EXPECT_EQ(ttk::Text::to_int("-", 4), 4);
    EXPECT_EQ(ttk::Text::to_int("--1", 4), 4);
    EXPECT_EQ(ttk::Text::to_int("+", 4), 4);
}

TEST(TextToInt, ParsesIntLimits) {
    EXPECT_EQ(ttk::Text::to_int("2147483647"), INT_MAX);
    EXPECT_EQ(ttk::Text::to_int("-2147483648"), INT_MIN);
}

TEST(TextToInt, OutOfRangeGivesDefault) {
    EXPECT_EQ(ttk::Text::to_int("2147483648", 3), 3);
    EXPECT_EQ(ttk::Text::to_int("-2147483649", 3), 3);
    EXPECT_EQ(ttk::Text::to_int("99999999999999999999", 3), 3);
}

TEST(TextToInt, RejectsMultiByteDigits) {
    EXPECT_EQ(ttk::Text::to_int("\xEF\xBC\x91", 8), 8) << "a fullwidth one is not a decimal digit";
}

TEST(TextIsInt, AcceptsWhatToIntParses) {
    EXPECT_TRUE(ttk::Text::is_int("42"));
    EXPECT_TRUE(ttk::Text::is_int(" -7 "));
    EXPECT_TRUE(ttk::Text::is_int("0"));
    EXPECT_TRUE(ttk::Text::is_int("2147483647"));
    EXPECT_TRUE(ttk::Text::is_int("-2147483648"));
}

TEST(TextIsInt, RejectsWhatToIntFallsBackOn) {
    EXPECT_FALSE(ttk::Text::is_int(""));
    EXPECT_FALSE(ttk::Text::is_int("  "));
    EXPECT_FALSE(ttk::Text::is_int("+1"));
    EXPECT_FALSE(ttk::Text::is_int("1a"));
    EXPECT_FALSE(ttk::Text::is_int("2147483648"));
    EXPECT_FALSE(ttk::Text::is_int("-"));
}

TEST(TextIsInt, AgreesWithToIntWhenTheValueEqualsTheDefault) {
    EXPECT_TRUE(ttk::Text::is_int("0")) << "0 parses even though it equals the default";
    EXPECT_EQ(ttk::Text::to_int("0"), 0);
}

TEST(TextSplit, SplitsOnSeparator) {
    EXPECT_EQ(ttk::Text::split("a,b,c", ','), (Strings{"a", "b", "c"}));
}

TEST(TextSplit, KeepsEmptyPieces) {
    EXPECT_EQ(ttk::Text::split("a,,b", ','), (Strings{"a", "", "b"}));
}

TEST(TextSplit, EmptyValueGivesNone) {
    EXPECT_TRUE(ttk::Text::split("", ',').empty());
}

TEST(TextSplit, NoSeparatorGivesWholeValue) {
    EXPECT_EQ(ttk::Text::split("abc", ','), (Strings{"abc"}));
}

TEST(TextSplit, LoneSeparatorGivesTwoEmptyPieces) {
    EXPECT_EQ(ttk::Text::split(",", ','), (Strings{"", ""}));
}

TEST(TextSplit, LeadingSeparatorGivesLeadingEmptyPiece) {
    EXPECT_EQ(ttk::Text::split(",a", ','), (Strings{"", "a"}));
}

TEST(TextSplit, TrailingSeparatorGivesTrailingEmptyPiece) {
    EXPECT_EQ(ttk::Text::split("a,", ','), (Strings{"a", ""}));
}

TEST(TextSplit, KeepsMultiBytePieces) {
    const std::string line = std::string(JAPANESE) + ":" + std::string(GRIN);

    EXPECT_EQ(ttk::Text::split(line, ':'), (Strings{std::string(JAPANESE), std::string(GRIN)}));
}

TEST(TextSplit, SplitsOnNul) {
    const std::string value("a\0b", 3);

    EXPECT_EQ(ttk::Text::split(value, '\0'), (Strings{"a", "b"}));
}

TEST(TextSplit, ManyPieces) {
    constexpr std::size_t COUNT = 10000;
    std::string line;

    for (std::size_t i = 0; i < COUNT; ++i) {
        line += std::to_string(i);
        line += ';';
    }

    const Strings parts = ttk::Text::split(line, ';');

    ASSERT_EQ(parts.size(), COUNT + 1);
    EXPECT_EQ(parts.front(), "0");
    EXPECT_EQ(parts[COUNT - 1], std::to_string(COUNT - 1));
    EXPECT_EQ(parts.back(), "");
}

TEST(TextJoin, JoinsWithSeparator) {
    EXPECT_EQ(ttk::Text::join({"a", "b", "c"}, ", "), "a, b, c");
}

TEST(TextJoin, EmptyListGivesEmpty) {
    EXPECT_EQ(ttk::Text::join({}, ","), "");
}

TEST(TextJoin, OnePartHasNoSeparator) {
    EXPECT_EQ(ttk::Text::join({"only"}, ","), "only");
}

TEST(TextJoin, EmptySeparatorConcatenates) {
    EXPECT_EQ(ttk::Text::join({"a", "b", "c"}, ""), "abc");
}

TEST(TextJoin, KeepsEmptyParts) {
    EXPECT_EQ(ttk::Text::join({"", "a", "", ""}, "/"), "/a//");
}

TEST(TextJoin, MultiByteSeparator) {
    EXPECT_EQ(ttk::Text::join({"a", "b"}, GRIN), "a" + std::string(GRIN) + "b");
}

TEST(TextJoin, RoundTripsWithSplit) {
    for (const std::string &line : {"a,b,c", "a,,b", ",", ",a,", "x"}) {
        EXPECT_EQ(ttk::Text::join(ttk::Text::split(line, ','), ","), line) << line;
    }
}

TEST(TextNaturalLess, OrdersNumbersByValue) {
    EXPECT_TRUE(ttk::Text::natural_less("MAP2", "MAP10"));
    EXPECT_FALSE(ttk::Text::natural_less("MAP10", "MAP2"));
}

TEST(TextNaturalLess, IsIrreflexive) {
    for (const std::string_view value : {"", "a", "MAP10", "10", "a1b2"}) {
        EXPECT_FALSE(ttk::Text::natural_less(value, value)) << value;
    }
}

TEST(TextNaturalLess, PrefixSortsFirst) {
    EXPECT_TRUE(ttk::Text::natural_less("a", "ab"));
    EXPECT_FALSE(ttk::Text::natural_less("ab", "a"));
    EXPECT_TRUE(ttk::Text::natural_less("", "a"));
    EXPECT_FALSE(ttk::Text::natural_less("a", ""));
}

TEST(TextNaturalLess, EmptyIsNotLessThanEmpty) {
    EXPECT_FALSE(ttk::Text::natural_less("", ""));
}

TEST(TextNaturalLess, DigitSortsBeforeOtherBytes) {
    EXPECT_TRUE(ttk::Text::natural_less("1", "a"));
    EXPECT_TRUE(ttk::Text::natural_less("9", "!"));
    EXPECT_FALSE(ttk::Text::natural_less("a", "1"));
    EXPECT_TRUE(ttk::Text::natural_less("x1", "xa"));
}

TEST(TextNaturalLess, CaseMatters) {
    EXPECT_TRUE(ttk::Text::natural_less("B", "a")) << "B is 0x42, a is 0x61";
    EXPECT_FALSE(ttk::Text::natural_less("a", "B"));
}

TEST(TextNaturalLess, LeadingZerosAreIgnored) {
    EXPECT_FALSE(ttk::Text::natural_less("a007", "a7"));
    EXPECT_FALSE(ttk::Text::natural_less("a7", "a007"));
    EXPECT_TRUE(ttk::Text::natural_less("a007", "a8"));
}

TEST(TextNaturalLess, ManyLeadingZerosDoNotUseUpTheNineDigits) {
    EXPECT_TRUE(ttk::Text::natural_less("00000000001", "2"));
    EXPECT_FALSE(ttk::Text::natural_less("2", "00000000001"));
}

TEST(TextNaturalLess, LongNumbersCompareNineDigitsAtATime) {
    EXPECT_TRUE(ttk::Text::natural_less("123456789012", "123456789013"));
    EXPECT_FALSE(ttk::Text::natural_less("123456789013", "123456789012"));
}

TEST(TextNaturalLess, ComparesTextAfterEqualNumbers) {
    EXPECT_TRUE(ttk::Text::natural_less("1a", "1b"));
    EXPECT_TRUE(ttk::Text::natural_less("v2.1", "v2.10"));
    EXPECT_TRUE(ttk::Text::natural_less("v2.9", "v2.10"));
}

TEST(TextNaturalLess, HighBytesCompareUnsigned) {
    EXPECT_TRUE(ttk::Text::natural_less("z", UMLAUT)) << "0x7a sorts before the lead byte 0xc3";
    EXPECT_FALSE(ttk::Text::natural_less(UMLAUT, "z"));
    EXPECT_TRUE(ttk::Text::natural_less("\x7f", "\x80"));
}

TEST(TextNaturalLess, IsAsymmetricForMultiByteText) {
    const std::string umlaut(UMLAUT);

    EXPECT_NE(ttk::Text::natural_less(umlaut, "z"), ttk::Text::natural_less("z", umlaut));
    EXPECT_NE(ttk::Text::natural_less(umlaut, JAPANESE), ttk::Text::natural_less(JAPANESE, umlaut));
}

TEST(TextNaturalLess, SortsAListNaturally) {
    Strings names = {"file10", "file2", "File3", "file1", "file", "file01a"};

    std::ranges::sort(names, ttk::Text::natural_less);

    EXPECT_EQ(names, (Strings{"File3", "file", "file1", "file01a", "file2", "file10"}));
}

TEST(TextCharacters, EmptyIsZero) {
    EXPECT_EQ(ttk::Text::characters(""), 0U);
}

TEST(TextCharacters, CountsAsciiBytes) {
    EXPECT_EQ(ttk::Text::characters("hello"), 5U);
}

TEST(TextCharacters, CountsEachMultiByteCharacterOnce) {
    EXPECT_EQ(ttk::Text::characters(UMLAUT), 1U);
    EXPECT_EQ(ttk::Text::characters(JAPANESE), 3U);
    EXPECT_EQ(ttk::Text::characters(GRIN), 1U);
}

TEST(TextCharacters, CountsMixedText) {
    EXPECT_EQ(ttk::Text::characters("a" + std::string(UMLAUT) + "b" + std::string(GRIN)), 4U);
}

TEST(TextCharacters, LoneContinuationBytesCountAsNothing) {
    EXPECT_EQ(ttk::Text::characters("\x80\xBF"), 0U);
}

TEST(TextCharacters, CountsEveryLengthAcrossVectorWidths) {
    for (std::size_t n = 0; n <= LONG; ++n) {
        EXPECT_EQ(ttk::Text::characters(repeat("a" + std::string(UMLAUT), n)), 2 * n) << n << " pairs";
        EXPECT_EQ(ttk::Text::characters(repeat(GRIN, n)), n) << n << " emoji";
        EXPECT_EQ(ttk::Text::characters(std::string(n, 'x')), n) << n << " letters";
    }
}

TEST(TextCharacters, CountsAContinuationByteAtEveryOffset) {
    for (std::size_t at = 1; at < LONG; ++at) {
        std::string value(LONG, 'a');

        value[at - 1] = UMLAUT[0];
        value[at] = UMLAUT[1];

        EXPECT_EQ(ttk::Text::characters(value), LONG - 1) << "umlaut ending at " << at;
    }
}

TEST(TextSimple, EmptyIsSimple) {
    EXPECT_TRUE(ttk::Text::simple(""));
}

TEST(TextSimple, PlainAsciiIsSimple) {
    EXPECT_TRUE(ttk::Text::simple("Hello, world! 123"));
}

TEST(TextSimple, OtherControlBytesAreSimple) {
    EXPECT_TRUE(ttk::Text::simple("a\nb\rc"));
}

TEST(TextSimple, TabIsNotSimple) {
    EXPECT_FALSE(ttk::Text::simple("a\tb"));
    EXPECT_FALSE(ttk::Text::simple("\t"));
}

TEST(TextSimple, MultiByteIsNotSimple) {
    EXPECT_FALSE(ttk::Text::simple(UMLAUT));
    EXPECT_FALSE(ttk::Text::simple("abc" + std::string(GRIN)));
}

TEST(TextSimple, LongAsciiIsSimple) {
    for (std::size_t n = 0; n <= LONG; ++n) {
        EXPECT_TRUE(ttk::Text::simple(std::string(n, 'q'))) << n;
    }
}

TEST(TextSimple, FindsATabAtEveryOffset) {
    for (std::size_t at = 0; at < LONG; ++at) {
        std::string value(LONG, 'a');
        value[at] = '\t';

        EXPECT_FALSE(ttk::Text::simple(value)) << "tab at " << at;
    }
}

TEST(TextSimple, FindsAHighByteAtEveryOffset) {
    for (std::size_t at = 0; at < LONG; ++at) {
        std::string value(LONG, 'a');
        value[at] = '\x80';

        EXPECT_FALSE(ttk::Text::simple(value)) << "high byte at " << at;
    }
}

TEST(TextSimple, FindsAHighByteAtTheEndOfEveryLength) {
    for (std::size_t n = 1; n <= LONG; ++n) {
        std::string value(n, 'a');
        value.back() = '\xFF';

        EXPECT_FALSE(ttk::Text::simple(value)) << n;
    }
}

TEST(TextLastOf, EmptyGivesNpos) {
    EXPECT_EQ(ttk::Text::last_of("", 'a'), std::string_view::npos);
}

TEST(TextLastOf, MissingGivesNpos) {
    EXPECT_EQ(ttk::Text::last_of("abc", 'z'), std::string_view::npos);
}

TEST(TextLastOf, FindsTheLastOfSeveral) {
    EXPECT_EQ(ttk::Text::last_of("a/b/c", '/'), 3U);
}

TEST(TextLastOf, FindsAtFirstAndLastIndex) {
    EXPECT_EQ(ttk::Text::last_of("/abc", '/'), 0U);
    EXPECT_EQ(ttk::Text::last_of("abc/", '/'), 3U);
    EXPECT_EQ(ttk::Text::last_of("/", '/'), 0U);
}

TEST(TextLastOf, FindsHighAndNulBytes) {
    const std::string value = "a" + std::string(UMLAUT) + std::string("\0z", 2);

    EXPECT_EQ(ttk::Text::last_of(value, UMLAUT[0]), 1U);
    EXPECT_EQ(ttk::Text::last_of(value, '\0'), 3U);
}

TEST(TextLastOf, FindsALetterAtEveryOffset) {
    for (std::size_t at = 0; at < LONG; ++at) {
        std::string value(LONG, 'a');
        value[at] = '/';

        EXPECT_EQ(ttk::Text::last_of(value, '/'), at) << "slash at " << at;
    }
}

TEST(TextLastOf, PrefersTheLaterOfTwoAcrossVectors) {
    for (std::size_t at = 1; at < LONG; ++at) {
        std::string value(LONG, 'a');
        value[0] = '/';
        value[at] = '/';

        EXPECT_EQ(ttk::Text::last_of(value, '/'), at) << "second slash at " << at;
    }
}

TEST(TextLastOf, MissingInEveryLengthGivesNpos) {
    for (std::size_t n = 0; n <= LONG; ++n) {
        EXPECT_EQ(ttk::Text::last_of(std::string(n, 'a'), 'b'), std::string_view::npos) << n;
    }
}

TEST_F(TextParseArguments, EmptyLineGivesNone) {
    EXPECT_TRUE(ttk::Text::parse_arguments("").empty());
}

TEST_F(TextParseArguments, BlankLineGivesNone) {
    EXPECT_TRUE(ttk::Text::parse_arguments(" \t\r\n ").empty());
}

TEST_F(TextParseArguments, SplitsOnEveryKindOfBlank) {
    EXPECT_EQ(ttk::Text::parse_arguments("a b\tc\nd\re"), (Strings{"a", "b", "c", "d", "e"}));
}

TEST_F(TextParseArguments, CollapsesRunsOfBlanks) {
    EXPECT_EQ(ttk::Text::parse_arguments("  a   b  "), (Strings{"a", "b"}));
}

TEST_F(TextParseArguments, SingleQuotesKeepContentLiterally) {
    EXPECT_EQ(ttk::Text::parse_arguments(R"('a b' '$TTK_TEST_WORD' 'x\y' 'q"q')"),
              (Strings{"a b", "$TTK_TEST_WORD", R"(x\y)", R"(q"q)"}));
}

TEST_F(TextParseArguments, DoubleQuotesKeepBlanks) {
    EXPECT_EQ(ttk::Text::parse_arguments("\"a b\"   \"c\td\""), (Strings{"a b", "c\td"}));
}

TEST_F(TextParseArguments, DoubleQuotesAllowEscapes) {
    EXPECT_EQ(ttk::Text::parse_arguments(R"("say \"hi\"" "back\\slash" "\$TTK_TEST_WORD")"),
              (Strings{R"(say "hi")", R"(back\slash)", "$TTK_TEST_WORD"}));
}

TEST_F(TextParseArguments, DoubleQuotesKeepSingleQuotes) {
    EXPECT_EQ(ttk::Text::parse_arguments(R"("it's")"), (Strings{"it's"}));
}

TEST_F(TextParseArguments, EmptyQuotesGiveAnEmptyArgument) {
    EXPECT_EQ(ttk::Text::parse_arguments(R"(a '' "" b)"), (Strings{"a", "", "", "b"}));
}

TEST_F(TextParseArguments, AdjacentPiecesJoinIntoOneArgument) {
    EXPECT_EQ(ttk::Text::parse_arguments(R"(a'b c'"d e"f)"), (Strings{"ab cd ef"}));
}

TEST_F(TextParseArguments, BackslashEscapesTheNextByteUnquoted) {
    EXPECT_EQ(ttk::Text::parse_arguments(R"(a\ b \'c \"d \\e)"), (Strings{"a b", "'c", "\"d", "\\e"}));
}

TEST_F(TextParseArguments, TrailingBackslashIsKept) {
    EXPECT_EQ(ttk::Text::parse_arguments(R"(abc\)"), (Strings{R"(abc\)"}));
}

TEST_F(TextParseArguments, ExpandsNameUnquoted) {
    EXPECT_EQ(ttk::Text::parse_arguments("$TTK_TEST_WORD"), (Strings{"word"}));
}

TEST_F(TextParseArguments, ExpandsBracedNameUnquoted) {
    EXPECT_EQ(ttk::Text::parse_arguments("${TTK_TEST_WORD}s"), (Strings{"words"}));
}

TEST_F(TextParseArguments, ExpandsNameInsideDoubleQuotes) {
    EXPECT_EQ(ttk::Text::parse_arguments(R"("[$TTK_TEST_WORD]" "[${TTK_TEST_WORD}]")"), (Strings{"[word]", "[word]"}));
}

TEST_F(TextParseArguments, NameEndsAtTheFirstNonIdentifierByte) {
    EXPECT_EQ(ttk::Text::parse_arguments("$TTK_TEST_WORD/bin $TTK_TEST_WORD.txt"), (Strings{"word/bin", "word.txt"}));
}

TEST_F(TextParseArguments, ExpandedValueStaysOneArgument) {
    EXPECT_EQ(ttk::Text::parse_arguments(R"("$TTK_TEST_SPACED")"), (Strings{"two words"}));
}

TEST_F(TextParseArguments, UnsetVariableExpandsToNothing) {
    EXPECT_EQ(ttk::Text::parse_arguments("a$TTK_TEST_UNSET b${TTK_TEST_UNSET}c"), (Strings{"a", "bc"}));
    EXPECT_EQ(ttk::Text::parse_arguments(R"("<$TTK_TEST_UNSET>")"), (Strings{"<>"}));
}

TEST_F(TextParseArguments, LoneDollarStays) {
    EXPECT_EQ(ttk::Text::parse_arguments("$ a$ \"$\" cost$/x"), (Strings{"$", "a$", "$", "cost$/x"}));
}

TEST_F(TextParseArguments, UnterminatedSingleQuoteRunsToTheEnd) {
    EXPECT_EQ(ttk::Text::parse_arguments("a 'b c"), (Strings{"a", "b c"}));
}

TEST_F(TextParseArguments, UnterminatedDoubleQuoteRunsToTheEnd) {
    EXPECT_EQ(ttk::Text::parse_arguments("a \"b c"), (Strings{"a", "b c"}));
}

TEST_F(TextParseArguments, KeepsMultiByteArguments) {
    EXPECT_EQ(ttk::Text::parse_arguments(std::string(JAPANESE) + " '" + std::string(GRIN) + " x'"),
              (Strings{std::string(JAPANESE), std::string(GRIN) + " x"}));
}

TEST_F(TextParseArguments, ParsesATypicalCommand) {
    EXPECT_EQ(ttk::Text::parse_arguments(R"(gzdoom -iwad "doom 2.wad" -file 'mods/a b.pk3' +map MAP01)"),
              (Strings{"gzdoom", "-iwad", "doom 2.wad", "-file", "mods/a b.pk3", "+map", "MAP01"}));
}

TEST(TextQuoteArgument, PlainArgumentIsUnchanged) {
    EXPECT_EQ(ttk::Text::quote_argument("plain-arg_1.wad"), "plain-arg_1.wad");
}

TEST(TextQuoteArgument, MultiByteArgumentIsUnchanged) {
    EXPECT_EQ(ttk::Text::quote_argument(JAPANESE), std::string(JAPANESE));
}

TEST(TextQuoteArgument, EmptyArgumentIsQuoted) {
    EXPECT_EQ(ttk::Text::quote_argument(""), "\"\"");
}

TEST(TextQuoteArgument, WhitespaceIsQuoted) {
    EXPECT_EQ(ttk::Text::quote_argument("a b"), "\"a b\"");
    EXPECT_EQ(ttk::Text::quote_argument("a\tb"), "\"a\tb\"");
    EXPECT_EQ(ttk::Text::quote_argument("a\nb"), "\"a\nb\"");
}

TEST(TextQuoteArgument, SingleQuoteAndDollarAreQuoted) {
    EXPECT_EQ(ttk::Text::quote_argument("it's"), "\"it's\"");
    EXPECT_EQ(ttk::Text::quote_argument("$HOME"), "\"$HOME\"");
}

TEST(TextQuoteArgument, EmbeddedQuoteIsEscaped) {
    EXPECT_EQ(ttk::Text::quote_argument(R"(a"b)"), R"("a\"b")");
}

TEST(TextQuoteArgument, InnerBackslashIsNotDoubled) {
    EXPECT_EQ(ttk::Text::quote_argument(R"(C:\dir\file)"), R"("C:\dir\file")");
}

TEST(TextQuoteArgument, TrailingBackslashesAreDoubled) {
    EXPECT_EQ(ttk::Text::quote_argument(R"(C:\dir\)"), R"("C:\dir\\")");
    EXPECT_EQ(ttk::Text::quote_argument(R"(a\\)"), R"("a\\\\")");
}

TEST(TextQuoteArgument, BackslashesBeforeAQuoteAreDoubled) {
    EXPECT_EQ(ttk::Text::quote_argument(R"(a\"b)"), R"("a\\\"b")");
    EXPECT_EQ(ttk::Text::quote_argument(R"(a\\"b)"), R"("a\\\\\"b")");
}

TEST(TextQuoteArgument, RoundTripsThroughTheWindowsSplitter) {
    const Strings cases = {
            "",
            "plain",
            "a b",
            R"(a"b)",
            R"(a\b)",
            R"(a\)",
            R"(a\\)",
            R"(\")",
            R"(\\")",
            R"("")",
            R"(C:\Program Files\x\)",
            "tab\there",
            "it's $5",
            std::string(JAPANESE) + " " + std::string(GRIN),
    };

    for (const std::string &argument : cases) {
        EXPECT_EQ(windows_split(ttk::Text::quote_argument(argument)), Strings{argument}) << argument;
    }
}

TEST(TextQuoteArgument, JoinedArgumentsSplitBackApart) {
    const Strings arguments = {"-file", R"(C:\mods\a b\)", R"(say "hi")", "", "x"};
    Strings quoted;

    for (const std::string &argument : arguments) {
        quoted.push_back(ttk::Text::quote_argument(argument));
    }

    EXPECT_EQ(windows_split(ttk::Text::join(quoted, " ")), arguments);
}
