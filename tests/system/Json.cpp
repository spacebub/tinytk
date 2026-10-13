// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "support/Sandbox.h"
#include "ttk/system/Json.h"

namespace {
    namespace fs = std::filesystem;
    namespace Json = ttk::Json;

    using Strings = std::vector<std::string>;

    constexpr std::size_t LARGE = 100000;

    Json::Doc parse(const std::string &text) {
        std::string error;
        Json::Doc doc = Json::read_data(text, &error);

        EXPECT_TRUE(doc.valid()) << text << ": " << error;

        return doc;
    }

    std::string rejection(const std::string &text) {
        std::string error;
        const Json::Doc doc = Json::read_data(text, &error);

        EXPECT_FALSE(doc.valid()) << text;

        return error;
    }

    std::string slurp(const fs::path &path) {
        std::ifstream file(path, std::ios::binary);

        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }

    void spill(const fs::path &path, const std::string_view text) {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);

        file.write(text.data(), static_cast<std::streamsize>(text.size()));
    }

    Strings field_names(yyjson_val *obj) {
        Strings out;

        Json::each_field(obj, [&out](const std::string_view key, yyjson_val *) { out.emplace_back(key); });

        return out;
    }
}

TEST(JsonDoc, DefaultIsInvalid) {
    const Json::Doc doc;

    EXPECT_FALSE(doc.valid());
    EXPECT_EQ(doc.root(), nullptr);
}

TEST(JsonDoc, NullDocumentIsInvalid) {
    const Json::Doc doc(nullptr);

    EXPECT_FALSE(doc.valid());
    EXPECT_EQ(doc.root(), nullptr);
}

TEST(JsonDoc, AdoptsARawDocument) {
    const Json::Doc doc(yyjson_read("[1]", 3, 0));

    ASSERT_TRUE(doc.valid());
    EXPECT_TRUE(yyjson_is_arr(doc.root()));
}

TEST(JsonDoc, MoveConstructionTransfersOwnership) {
    Json::Doc source = parse(R"({"a":"text"})");
    const Json::Doc moved(std::move(source));

    EXPECT_FALSE(
            source.valid()); // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move): the moved-from state is checked.
    ASSERT_TRUE(moved.valid());
    EXPECT_EQ(Json::obj_get_string(moved.root(), "a"), "text");
}

TEST(JsonDoc, MoveAssignmentReplacesTheHeldDocument) {
    Json::Doc target = parse("[1]");
    Json::Doc source = parse(R"({"b":2})");

    target = std::move(source);

    EXPECT_FALSE(
            source.valid()); // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move): the moved-from state is checked.
    ASSERT_TRUE(target.valid());
    EXPECT_EQ(Json::obj_get_int(target.root(), "b"), 2);
}

TEST(JsonDoc, MoveAssignmentFromAnEmptyDocumentEmpties) {
    Json::Doc target = parse("[1]");

    target = Json::Doc();

    EXPECT_FALSE(target.valid());
}

TEST(JsonReadData, ParsesAnObjectRoot) {
    const Json::Doc doc = parse(R"({"a":1})");

    EXPECT_TRUE(yyjson_is_obj(doc.root()));
}

TEST(JsonReadData, ParsesAnArrayRoot) {
    const Json::Doc doc = parse("[1,2,3]");

    ASSERT_TRUE(yyjson_is_arr(doc.root()));
    EXPECT_EQ(yyjson_arr_size(doc.root()), 3U);
}

TEST(JsonReadData, ParsesScalarRoots) {
    EXPECT_TRUE(yyjson_is_str(parse(R"("s")").root()));
    EXPECT_TRUE(yyjson_is_int(parse("42").root()));
    EXPECT_TRUE(yyjson_is_real(parse("4.5").root()));
    EXPECT_TRUE(yyjson_is_true(parse("true").root()));
    EXPECT_TRUE(yyjson_is_false(parse("false").root()));
    EXPECT_TRUE(yyjson_is_null(parse("null").root()));
}

TEST(JsonReadData, ParsesEmptyContainers) {
    const Json::Doc object = parse("{}");
    const Json::Doc array = parse("[]");

    EXPECT_EQ(yyjson_obj_size(object.root()), 0U);
    EXPECT_EQ(yyjson_arr_size(array.root()), 0U);
}

TEST(JsonReadData, AllowsSurroundingWhitespace) {
    EXPECT_EQ(Json::as_int(parse(" \t\r\n 7 \n").root()), 7);
}

TEST(JsonReadData, ParsesNumberForms) {
    const Json::Doc doc = parse("[0,-0,-12,1e2,2.5E-1,-3.75,9223372036854775807]");
    const yyjson_val *root = doc.root();

    EXPECT_EQ(Json::as_int(yyjson_arr_get(root, 0)), 0);
    EXPECT_EQ(Json::as_int(yyjson_arr_get(root, 2)), -12);
    EXPECT_EQ(Json::as_int(yyjson_arr_get(root, 3)), 100);
    EXPECT_DOUBLE_EQ(yyjson_get_num(yyjson_arr_get(root, 4)), 0.25);
    EXPECT_DOUBLE_EQ(yyjson_get_num(yyjson_arr_get(root, 5)), -3.75);
    EXPECT_EQ(yyjson_get_sint(yyjson_arr_get(root, 6)), INT64_MAX);
}

TEST(JsonReadData, DecodesEscapes) {
    const Json::Doc doc = parse(R"(["\"", "\\", "\/", "\b\f\n\r\t"])");
    const Strings items = Json::as_string_list(doc.root());

    EXPECT_EQ(items, (Strings{"\"", "\\", "/", "\b\f\n\r\t"}));
}

TEST(JsonReadData, DecodesUnicodeEscapes) {
    const Json::Doc doc = parse(R"(["\u0041", "\u00e4", "\u65e5", "\ud83d\ude00"])");

    EXPECT_EQ(Json::as_string_list(doc.root()), (Strings{"A", "\xC3\xA4", "\xE6\x97\xA5", "\xF0\x9F\x98\x80"}));
}

TEST(JsonReadData, KeepsRawUtf8) {
    const Json::Doc doc = parse("\"\xE6\x97\xA5\xE6\x9C\xAC \xF0\x9F\x98\x80\"");

    EXPECT_EQ(Json::as_string(doc.root()), "\xE6\x97\xA5\xE6\x9C\xAC \xF0\x9F\x98\x80");
}

TEST(JsonReadData, KeepsAnEscapedNul) {
    const Json::Doc doc = parse(R"("a\u0000b")");

    EXPECT_EQ(Json::as_string(doc.root()), std::string("a\0b", 3));
}

TEST(JsonReadData, ParsesDeepNesting) {
    constexpr std::size_t DEPTH = 5000;
    const std::string text = std::string(DEPTH, '[') + std::string(DEPTH, ']');
    const Json::Doc doc = parse(text);

    std::size_t depth = 0;

    for (const yyjson_val *at = doc.root(); at != nullptr && yyjson_is_arr(at); at = yyjson_arr_get_first(at)) {
        ++depth;
    }

    EXPECT_EQ(depth, DEPTH);
}

TEST(JsonReadData, ParsesALargeArray) {
    std::string text = "[";

    for (std::size_t i = 0; i < LARGE; ++i) {
        text += std::to_string(i);
        text += i + 1 < LARGE ? "," : "]";
    }

    const Json::Doc doc = parse(text);
    std::size_t count = 0;
    bool ordered = true;

    Json::each_item(doc.root(), [&](yyjson_val *item) {
        ordered = ordered && std::cmp_equal(Json::as_int(item), count);
        ++count;
    });

    EXPECT_EQ(count, LARGE);
    EXPECT_TRUE(ordered);
}

TEST(JsonReadData, DoesNotReferToTheDataAfterwards) {
    std::string text = R"({"name":"a value long enough to live on the heap","list":["x","y"]})";
    const Json::Doc doc = parse(text);

    text.assign(text.size(), '#');
    text.clear();
    text.shrink_to_fit();

    EXPECT_EQ(Json::obj_get_string(doc.root(), "name"), "a value long enough to live on the heap");
    EXPECT_EQ(Json::as_string_list(Json::obj_get(doc.root(), "list")), (Strings{"x", "y"}));
}

TEST(JsonReadData, RejectsEmptyText) {
    EXPECT_FALSE(rejection("").empty());
}

TEST(JsonReadData, RejectsOnlyWhitespace) {
    EXPECT_FALSE(rejection("   ").empty());
}

TEST(JsonReadData, RejectsUnclosedContainers) {
    EXPECT_FALSE(rejection("{").empty());
    EXPECT_FALSE(rejection("[1,2").empty());
    EXPECT_FALSE(rejection(R"({"a":)").empty());
}

TEST(JsonReadData, RejectsTrailingCommas) {
    EXPECT_FALSE(rejection("[1,]").empty());
    EXPECT_FALSE(rejection(R"({"a":1,})").empty());
}

TEST(JsonReadData, RejectsComments) {
    EXPECT_FALSE(rejection("// note\n1").empty());
    EXPECT_FALSE(rejection("[1 /* note */]").empty());
}

TEST(JsonReadData, RejectsSingleQuotesAndBareKeys) {
    EXPECT_FALSE(rejection("{'a':1}").empty());
    EXPECT_FALSE(rejection("{a:1}").empty());
}

TEST(JsonReadData, RejectsMisspelledLiterals) {
    EXPECT_FALSE(rejection("tru").empty());
    EXPECT_FALSE(rejection("nul").empty());
    EXPECT_FALSE(rejection("True").empty());
}

TEST(JsonReadData, RejectsNonFiniteNumbers) {
    EXPECT_FALSE(rejection("NaN").empty());
    EXPECT_FALSE(rejection("[Infinity]").empty());
}

TEST(JsonReadData, RejectsMalformedNumbers) {
    EXPECT_FALSE(rejection("01").empty());
    EXPECT_FALSE(rejection("1.").empty());
    EXPECT_FALSE(rejection("+1").empty());
    EXPECT_FALSE(rejection("0x10").empty());
}

TEST(JsonReadData, RejectsTrailingContent) {
    EXPECT_FALSE(rejection("1 2").empty());
    EXPECT_FALSE(rejection("{} x").empty());
}

TEST(JsonReadData, RejectsBadEscapes) {
    EXPECT_FALSE(rejection(R"("\x41")").empty());
    EXPECT_FALSE(rejection(R"("\u12")").empty());
}

TEST(JsonReadData, RejectsRawControlCharactersInStrings) {
    EXPECT_FALSE(rejection("\"a\nb\"").empty());
}

TEST(JsonReadData, RejectsInvalidUtf8) {
    EXPECT_FALSE(rejection("\"\xFF\"").empty());
    EXPECT_FALSE(rejection("\"\xE6\x97\"").empty()) << "a truncated sequence";
}

TEST(JsonReadData, ErrorNamesTheByteOffset) {
    const std::string error = rejection(R"({"a":1,})");

    EXPECT_NE(error.find("offset"), std::string::npos) << error;
}

TEST(JsonReadData, ErrorOffsetPointsAtTheProblem) {
    EXPECT_NE(rejection("[1,2,x]").find("offset 5"), std::string::npos);
    EXPECT_NE(rejection("    @").find("offset 4"), std::string::npos);
}

TEST(JsonReadData, ErrorIsOptional) {
    EXPECT_FALSE(Json::read_data("{").valid());
}

TEST(JsonReadData, ValidDocumentLeavesTheErrorAlone) {
    std::string error = "untouched";

    EXPECT_TRUE(Json::read_data("[]", &error).valid());
    EXPECT_EQ(error, "untouched");
}

TEST(JsonReadFile, ParsesAFile) {
    const fs::path path = tests::Sandbox::scratch("read-file") / "doc.json";

    spill(path, R"({"name":"a value long enough to live on the heap","n":3})");

    std::string error;
    const Json::Doc doc = Json::read_file(path, &error);

    ASSERT_TRUE(doc.valid()) << error;
    EXPECT_EQ(Json::obj_get_string(doc.root(), "name"), "a value long enough to live on the heap");
    EXPECT_EQ(Json::obj_get_int(doc.root(), "n"), 3);
}

TEST(JsonReadFile, StringsSurviveMovingTheDocument) {
    const fs::path path = tests::Sandbox::scratch("read-move") / "doc.json";

    spill(path, R"({"name":"a value long enough to live on the heap"})");

    Json::Doc first = Json::read_file(path);
    const Json::Doc second(std::move(first));

    ASSERT_TRUE(second.valid());
    EXPECT_EQ(Json::obj_get_string(second.root(), "name"), "a value long enough to live on the heap");
}

TEST(JsonReadFile, StringsOfATinyFileOutliveTheRead) {
    const fs::path root = tests::Sandbox::scratch("read-tiny-strings");

    spill(root / "first.json", R"("hello")");
    spill(root / "second.json", R"("world")");

    Json::Doc first = Json::read_file(root / "first.json");
    const Json::Doc second = Json::read_file(root / "second.json");
    const Json::Doc moved(std::move(first));

    EXPECT_EQ(Json::as_string(moved.root()), "hello");
    EXPECT_EQ(Json::as_string(second.root()), "world");
}

TEST(JsonReadFile, ParsesAFileWithoutStrings) {
    const fs::path path = tests::Sandbox::scratch("read-tiny") / "doc.json";

    spill(path, "[1,2]");

    const Json::Doc doc = Json::read_file(path);
    std::array<int, 2> out{};

    ASSERT_TRUE(doc.valid());
    ASSERT_TRUE(Json::as_int_array(doc.root(), out.data(), 2));
    EXPECT_EQ(out, (std::array{1, 2}));
}

TEST(JsonReadFile, ParsesALargeFile) {
    const fs::path path = tests::Sandbox::scratch("read-large") / "doc.json";
    std::string text = "[";

    for (std::size_t i = 0; i < LARGE; ++i) {
        text += "\"item" + std::to_string(i) + "\"";
        text += i + 1 < LARGE ? "," : "]";
    }

    spill(path, text);

    const Json::Doc doc = Json::read_file(path);
    const Strings items = Json::as_string_list(doc.root());

    ASSERT_EQ(items.size(), LARGE);
    EXPECT_EQ(items.front(), "item0");
    EXPECT_EQ(items.back(), "item" + std::to_string(LARGE - 1));
}

TEST(JsonReadFile, ParsesEscapesInPlace) {
    const fs::path path = tests::Sandbox::scratch("read-escapes") / "doc.json";

    spill(path, R"({"text":"line\nnext \u00e4 \ud83d\ude00 \"quoted\" and padding to the heap"})");

    const Json::Doc doc = Json::read_file(path);

    EXPECT_EQ(Json::obj_get_string(doc.root(), "text"),
              "line\nnext \xC3\xA4 \xF0\x9F\x98\x80 \"quoted\" and padding to the heap");
}

TEST(JsonReadFile, MissingFileIsInvalid) {
    const fs::path path = tests::Sandbox::scratch("read-missing") / "absent.json";
    std::string error;

    EXPECT_FALSE(Json::read_file(path, &error).valid());
    EXPECT_NE(error.find("cannot open"), std::string::npos) << error;
    EXPECT_NE(error.find(path.string()), std::string::npos) << "the message names the file: " << error;
}

TEST(JsonReadFile, MissingFileWithoutError) {
    EXPECT_FALSE(Json::read_file(tests::Sandbox::scratch("read-missing-quiet") / "absent.json").valid());
}

TEST(JsonReadFile, DirectoryIsInvalid) {
    const fs::path path = tests::Sandbox::scratch("read-directory");
    std::string error;

    EXPECT_FALSE(Json::read_file(path, &error).valid());
    EXPECT_NE(error.find(path.string()), std::string::npos) << error;
    EXPECT_FALSE(Json::read_file(fs::temp_directory_path()).valid());
    EXPECT_FALSE(Json::read_file("/").valid());
}

TEST(JsonReadFile, UnreadableFileIsInvalid) {
    const fs::path path = tests::Sandbox::scratch("read-unreadable") / "locked.json";

    spill(path, "[1]");
    fs::permissions(path, fs::perms::none);

    if (std::ifstream(path)) {
        GTEST_SKIP() << "permissions do not bind this user";
    }

    std::string error;

    EXPECT_FALSE(Json::read_file(path, &error).valid());
    EXPECT_NE(error.find("cannot open"), std::string::npos) << error;
}

TEST(JsonReadFile, EmptyFileIsInvalid) {
    const fs::path path = tests::Sandbox::scratch("read-empty") / "empty.json";

    spill(path, "");

    std::string error;

    EXPECT_FALSE(Json::read_file(path, &error).valid());
    EXPECT_FALSE(error.empty());
}

TEST(JsonReadFile, InvalidFileReportsTheOffset) {
    const fs::path path = tests::Sandbox::scratch("read-invalid") / "bad.json";

    spill(path, R"({"a": [1, 2,]})");

    std::string error;

    EXPECT_FALSE(Json::read_file(path, &error).valid());
    EXPECT_NE(error.find("offset"), std::string::npos) << error;
}

TEST(JsonAsString, ReadsAString) {
    EXPECT_EQ(Json::as_string(parse(R"("hello")").root()), "hello");
}

TEST(JsonAsString, ReadsAnEmptyString) {
    EXPECT_EQ(Json::as_string(parse(R"("")").root(), "def"), "");
}

TEST(JsonAsString, NullPointerGivesDefault) {
    EXPECT_EQ(Json::as_string(nullptr, "def"), "def");
    EXPECT_EQ(Json::as_string(nullptr), "");
}

TEST(JsonAsString, OtherTypesGiveDefault) {
    for (const std::string text : {"1", "1.5", "true", "null", "[]", "{}"}) {
        EXPECT_EQ(Json::as_string(parse(text).root(), "def"), "def") << text;
    }
}

TEST(JsonAsInt, ReadsIntegers) {
    EXPECT_EQ(Json::as_int(parse("42").root()), 42);
    EXPECT_EQ(Json::as_int(parse("-42").root()), -42);
    EXPECT_EQ(Json::as_int(parse("0").root(), 9), 0);
}

TEST(JsonAsInt, ReadsIntLimits) {
    EXPECT_EQ(Json::as_int(parse("2147483647").root()), INT_MAX);
    EXPECT_EQ(Json::as_int(parse("-2147483648").root()), INT_MIN);
}

TEST(JsonAsInt, TruncatesRealsTowardZero) {
    EXPECT_EQ(Json::as_int(parse("3.9").root()), 3);
    EXPECT_EQ(Json::as_int(parse("-3.9").root()), -3);
    EXPECT_EQ(Json::as_int(parse("0.5").root(), 7), 0);
}

TEST(JsonAsInt, ParsesNumericStrings) {
    EXPECT_EQ(Json::as_int(parse(R"("12")").root()), 12);
    EXPECT_EQ(Json::as_int(parse(R"(" -5 ")").root()), -5);
}

TEST(JsonAsInt, NonNumericStringsGiveDefault) {
    EXPECT_EQ(Json::as_int(parse(R"("twelve")").root(), 8), 8);
    EXPECT_EQ(Json::as_int(parse(R"("")").root(), 8), 8);
    EXPECT_EQ(Json::as_int(parse(R"("1.5")").root(), 8), 8);
}

TEST(JsonAsInt, OtherTypesGiveDefault) {
    for (const std::string text : {"true", "false", "null", "[1]", R"({"a":1})"}) {
        EXPECT_EQ(Json::as_int(parse(text).root(), 8), 8) << text;
    }

    EXPECT_EQ(Json::as_int(nullptr, 8), 8);
}

TEST(JsonAsBool, ReadsBooleans) {
    EXPECT_TRUE(Json::as_bool(parse("true").root(), false));
    EXPECT_FALSE(Json::as_bool(parse("false").root(), true));
}

TEST(JsonAsBool, IntegersAreTrueWhenNonZero) {
    EXPECT_TRUE(Json::as_bool(parse("1").root()));
    EXPECT_TRUE(Json::as_bool(parse("-3").root()));
    EXPECT_FALSE(Json::as_bool(parse("0").root(), true));
}

TEST(JsonAsBool, StringsOneAndTrueInAnyCaseAreTrue) {
    for (const std::string text : {R"("1")", R"("true")", R"("TRUE")", R"("True")", R"("tRuE")"}) {
        EXPECT_TRUE(Json::as_bool(parse(text).root())) << text;
    }
}

TEST(JsonAsBool, OtherStringsAreFalse) {
    for (const std::string text : {R"("0")", R"("false")", R"("yes")", R"("")", R"(" true")", R"("2")"}) {
        EXPECT_FALSE(Json::as_bool(parse(text).root(), true)) << text;
    }
}

TEST(JsonAsBool, OtherTypesGiveDefault) {
    for (const std::string text : {"null", "1.0", "[true]", R"({"a":true})"}) {
        EXPECT_TRUE(Json::as_bool(parse(text).root(), true)) << text;
        EXPECT_FALSE(Json::as_bool(parse(text).root(), false)) << text;
    }

    EXPECT_TRUE(Json::as_bool(nullptr, true));
}

TEST(JsonAsStringList, ReadsStringsInOrder) {
    EXPECT_EQ(Json::as_string_list(parse(R"(["c","a","b"])").root()), (Strings{"c", "a", "b"}));
}

TEST(JsonAsStringList, SkipsItemsThatAreNotStrings) {
    EXPECT_EQ(Json::as_string_list(parse(R"(["a",1,null,["b"],{"c":"d"},true,"e"])").root()), (Strings{"a", "e"}));
}

TEST(JsonAsStringList, KeepsEmptyAndDuplicateStrings) {
    EXPECT_EQ(Json::as_string_list(parse(R"(["", "x", "x"])").root()), (Strings{"", "x", "x"}));
}

TEST(JsonAsStringList, EmptyArrayGivesNone) {
    EXPECT_TRUE(Json::as_string_list(parse("[]").root()).empty());
}

TEST(JsonAsStringList, NonArrayGivesNone) {
    EXPECT_TRUE(Json::as_string_list(nullptr).empty());
    EXPECT_TRUE(Json::as_string_list(parse(R"("a")").root()).empty());
    EXPECT_TRUE(Json::as_string_list(parse(R"({"a":"b"})").root()).empty());
}

TEST(JsonAsIntArray, ReadsExactlyCountItems) {
    std::array<int, 3> out{};

    EXPECT_TRUE(Json::as_int_array(parse("[4,5,6]").root(), out.data(), 3));
    EXPECT_EQ(out, (std::array{4, 5, 6}));
}

TEST(JsonAsIntArray, IgnoresExtraItems) {
    std::array<int, 3> out{-1, -1, -1};

    EXPECT_TRUE(Json::as_int_array(parse("[1,2,3,4,5]").root(), out.data(), 2));
    EXPECT_EQ(out, (std::array{1, 2, -1}));
}

TEST(JsonAsIntArray, TruncatesReals) {
    std::array<int, 2> out{};

    EXPECT_TRUE(Json::as_int_array(parse("[1.9,-2.9]").root(), out.data(), 2));
    EXPECT_EQ(out, (std::array{1, -2}));
}

TEST(JsonAsIntArray, ShorterArrayFailsAndWritesNothing) {
    std::array<int, 3> out{-1, -1, -1};

    EXPECT_FALSE(Json::as_int_array(parse("[1,2]").root(), out.data(), 3));
    EXPECT_EQ(out, (std::array{-1, -1, -1}));
}

TEST(JsonAsIntArray, NonNumberItemFailsAfterWritingTheOnesBefore) {
    std::array<int, 3> out{-1, -1, -1};

    EXPECT_FALSE(Json::as_int_array(parse(R"([7,"8",9])").root(), out.data(), 3));
    EXPECT_EQ(out[0], 7) << "items before the failing one are already written";
}

TEST(JsonAsIntArray, BooleanAndNullItemsFail) {
    std::array<int, 1> out{};

    EXPECT_FALSE(Json::as_int_array(parse("[true]").root(), out.data(), 1));
    EXPECT_FALSE(Json::as_int_array(parse("[null]").root(), out.data(), 1));
}

TEST(JsonAsIntArray, NonArrayFails) {
    std::array<int, 1> out{};

    EXPECT_FALSE(Json::as_int_array(nullptr, out.data(), 1));
    EXPECT_FALSE(Json::as_int_array(parse("1").root(), out.data(), 1));
    EXPECT_FALSE(Json::as_int_array(parse(R"({"0":1})").root(), out.data(), 1));
}

TEST(JsonAsIntArray, ZeroCountSucceedsOnAnyArray) {
    std::array<int, 1> out{-1};

    EXPECT_TRUE(Json::as_int_array(parse("[]").root(), out.data(), 0));
    EXPECT_TRUE(Json::as_int_array(parse(R"(["x"])").root(), out.data(), 0));
    EXPECT_EQ(out[0], -1);
}

TEST(JsonAsIntArray, NonArrayFailsEvenForZeroCount) {
    std::array<int, 1> out{};

    EXPECT_FALSE(Json::as_int_array(parse("{}").root(), out.data(), 0));
}

TEST(JsonEachField, VisitsFieldsInDocumentOrder) {
    const Json::Doc doc = parse(R"({"z":1,"a":2,"m":3})");

    EXPECT_EQ(field_names(doc.root()), (Strings{"z", "a", "m"}));
}

TEST(JsonEachField, PassesTheValues) {
    const Json::Doc doc = parse(R"({"a":1,"b":"two"})");
    std::vector<std::pair<std::string, std::string>> seen;

    Json::each_field(doc.root(), [&](const std::string_view key, yyjson_val *val) {
        seen.emplace_back(key, yyjson_is_str(val) ? Json::as_string(val) : std::to_string(Json::as_int(val)));
    });

    EXPECT_EQ(seen, (std::vector<std::pair<std::string, std::string>>{{"a", "1"}, {"b", "two"}}));
}

TEST(JsonEachField, DecodesKeys) {
    const Json::Doc doc = parse(R"({"\u00e4":1,"a\"b":2,"":3,"x\u0000y":4})");

    EXPECT_EQ(field_names(doc.root()), (Strings{"\xC3\xA4", "a\"b", "", std::string("x\0y", 3)}));
}

TEST(JsonEachField, VisitsDuplicateKeys) {
    const Json::Doc doc = parse(R"({"a":1,"a":2})");

    EXPECT_EQ(field_names(doc.root()), (Strings{"a", "a"}));
}

TEST(JsonEachField, IgnoresNonObjects) {
    int calls = 0;
    const auto count = [&calls](std::string_view, yyjson_val *) { ++calls; };

    Json::each_field(nullptr, count);
    Json::each_field(parse("[1,2]").root(), count);
    Json::each_field(parse(R"("a")").root(), count);
    Json::each_field(parse("{}").root(), count);

    EXPECT_EQ(calls, 0);
}

TEST(JsonEachItem, VisitsItemsInOrder) {
    const Json::Doc doc = parse("[3,1,2]");
    std::vector<int> seen;

    Json::each_item(doc.root(), [&seen](yyjson_val *item) { seen.push_back(Json::as_int(item)); });

    EXPECT_EQ(seen, (std::vector{3, 1, 2}));
}

TEST(JsonEachItem, VisitsItemsOfEveryType) {
    const Json::Doc doc = parse(R"([1,"a",null,true,[],{}])");
    std::size_t calls = 0;

    Json::each_item(doc.root(), [&calls](yyjson_val *) { ++calls; });

    EXPECT_EQ(calls, 6U);
}

TEST(JsonEachItem, IgnoresNonArrays) {
    int calls = 0;
    const auto count = [&calls](yyjson_val *) { ++calls; };

    Json::each_item(nullptr, count);
    Json::each_item(parse(R"({"a":1})").root(), count);
    Json::each_item(parse("1").root(), count);
    Json::each_item(parse("[]").root(), count);

    EXPECT_EQ(calls, 0);
}

TEST(JsonObjGet, FindsAField) {
    const Json::Doc doc = parse(R"({"a":1,"b":2})");

    EXPECT_EQ(Json::as_int(Json::obj_get(doc.root(), "b")), 2);
}

TEST(JsonObjGet, MissingFieldIsNull) {
    EXPECT_EQ(Json::obj_get(parse(R"({"a":1})").root(), "b"), nullptr);
}

TEST(JsonObjGet, KeysAreCaseSensitive) {
    EXPECT_EQ(Json::obj_get(parse(R"({"Key":1})").root(), "key"), nullptr);
}

TEST(JsonObjGet, NonObjectIsNull) {
    EXPECT_EQ(Json::obj_get(nullptr, "a"), nullptr);
    EXPECT_EQ(Json::obj_get(parse(R"(["a"])").root(), "a"), nullptr);
    EXPECT_EQ(Json::obj_get(parse(R"("a")").root(), "a"), nullptr);
}

TEST(JsonObjGet, DoesNotLookIntoNestedObjects) {
    EXPECT_EQ(Json::obj_get(parse(R"({"outer":{"inner":1}})").root(), "inner"), nullptr);
}

TEST(JsonObjGet, FindsAUtf8Key) {
    const Json::Doc doc = parse(R"({"\u65e5":5})");

    EXPECT_EQ(Json::as_int(Json::obj_get(doc.root(), "\xE6\x97\xA5")), 5);
}

TEST(JsonObjGet, FindsAFieldHoldingNull) {
    const Json::Doc doc = parse(R"({"a":null})");
    yyjson_val *val = Json::obj_get(doc.root(), "a");

    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(yyjson_is_null(val));
}

TEST(JsonObjGetString, PresentMissingAndWrongType) {
    const Json::Doc doc = parse(R"({"s":"text","n":1})");

    EXPECT_EQ(Json::obj_get_string(doc.root(), "s", "def"), "text");
    EXPECT_EQ(Json::obj_get_string(doc.root(), "missing", "def"), "def");
    EXPECT_EQ(Json::obj_get_string(doc.root(), "n", "def"), "def");
    EXPECT_EQ(Json::obj_get_string(doc.root(), "missing"), "");
    EXPECT_EQ(Json::obj_get_string(nullptr, "s", "def"), "def");
}

TEST(JsonObjGetInt, PresentMissingAndWrongType) {
    const Json::Doc doc = parse(R"({"n":5,"r":2.7,"s":"9","x":"nine","b":true})");

    EXPECT_EQ(Json::obj_get_int(doc.root(), "n", -1), 5);
    EXPECT_EQ(Json::obj_get_int(doc.root(), "r", -1), 2);
    EXPECT_EQ(Json::obj_get_int(doc.root(), "s", -1), 9);
    EXPECT_EQ(Json::obj_get_int(doc.root(), "x", -1), -1);
    EXPECT_EQ(Json::obj_get_int(doc.root(), "b", -1), -1);
    EXPECT_EQ(Json::obj_get_int(doc.root(), "missing", -1), -1);
    EXPECT_EQ(Json::obj_get_int(doc.root(), "missing"), 0);
    EXPECT_EQ(Json::obj_get_int(nullptr, "n", -1), -1);
}

TEST(JsonBuilder, WritesAnObjectIndentedByTwoSpaces) {
    const fs::path path = tests::Sandbox::scratch("write-pretty") / "doc.json";
    const Json::Builder builder;
    yyjson_mut_val *root = builder.new_object();

    builder.add_string(root, "name", "x");
    builder.add_int(root, "n", 1);
    builder.set_root(root);

    ASSERT_TRUE(builder.write_file(path));
    EXPECT_EQ(slurp(path), "{\n  \"name\": \"x\",\n  \"n\": 1\n}\n");
}

TEST(JsonBuilder, WritesNestedContainersIndented) {
    const fs::path path = tests::Sandbox::scratch("write-nested") / "doc.json";
    const Json::Builder builder;
    yyjson_mut_val *root = builder.new_object();
    yyjson_mut_val *list = builder.new_array();

    builder.append_int(list, 1);
    builder.append_int(list, 2);
    builder.add_value(root, "list", list);
    builder.set_root(root);

    ASSERT_TRUE(builder.write_file(path));
    EXPECT_EQ(slurp(path), "{\n  \"list\": [\n    1,\n    2\n  ]\n}\n");
}

TEST(JsonBuilder, WritesEmptyContainers) {
    const fs::path root = tests::Sandbox::scratch("write-empty");
    const Json::Builder objects;
    const Json::Builder arrays;

    objects.set_root(objects.new_object());
    arrays.set_root(arrays.new_array());

    ASSERT_TRUE(objects.write_file(root / "object.json"));
    ASSERT_TRUE(arrays.write_file(root / "array.json"));
    EXPECT_EQ(slurp(root / "object.json"), "{}\n");
    EXPECT_EQ(slurp(root / "array.json"), "[]\n");
}

TEST(JsonBuilder, RoundTripsEveryFieldType) {
    const fs::path path = tests::Sandbox::scratch("write-types") / "doc.json";
    const Json::Builder builder;
    yyjson_mut_val *root = builder.new_object();

    builder.add_string(root, "s", "text");
    builder.add_string(root, "empty", "");
    builder.add_int(root, "max", INT_MAX);
    builder.add_int(root, "min", INT_MIN);
    builder.add_int(root, "zero", 0);
    builder.add_bool(root, "yes", true);
    builder.add_bool(root, "no", false);
    builder.set_root(root);

    ASSERT_TRUE(builder.write_file(path));

    const Json::Doc doc = Json::read_file(path);

    ASSERT_TRUE(doc.valid());
    EXPECT_EQ(Json::obj_get_string(doc.root(), "s"), "text");
    EXPECT_EQ(Json::obj_get_string(doc.root(), "empty", "def"), "");
    EXPECT_EQ(Json::obj_get_int(doc.root(), "max"), INT_MAX);
    EXPECT_EQ(Json::obj_get_int(doc.root(), "min"), INT_MIN);
    EXPECT_EQ(Json::obj_get_int(doc.root(), "zero", 5), 0);
    EXPECT_TRUE(Json::as_bool(Json::obj_get(doc.root(), "yes")));
    EXPECT_FALSE(Json::as_bool(Json::obj_get(doc.root(), "no"), true));
    EXPECT_EQ(field_names(doc.root()), (Strings{"s", "empty", "max", "min", "zero", "yes", "no"}));
}

TEST(JsonBuilder, EscapesStringValues) {
    const fs::path path = tests::Sandbox::scratch("write-escapes") / "doc.json";
    const std::string tricky = "quote \" backslash \\ newline \n tab \t bell \x07 nul " + std::string(1, '\0') + " end";
    const Json::Builder builder;
    yyjson_mut_val *root = builder.new_object();

    builder.add_string(root, "v", tricky);
    builder.set_root(root);

    ASSERT_TRUE(builder.write_file(path));
    EXPECT_EQ(Json::obj_get_string(Json::read_file(path).root(), "v"), tricky);
}

TEST(JsonBuilder, KeepsUtf8Values) {
    const fs::path path = tests::Sandbox::scratch("write-utf8") / "doc.json";
    const std::string text = "\xE6\x97\xA5\xE6\x9C\xAC \xC3\xA4 \xF0\x9F\x98\x80";
    const Json::Builder builder;
    yyjson_mut_val *root = builder.new_array();

    builder.append_string(root, text);
    builder.set_root(root);

    ASSERT_TRUE(builder.write_file(path));
    EXPECT_NE(slurp(path).find(text), std::string::npos) << "UTF-8 is written raw, not escaped";
    EXPECT_EQ(Json::as_string_list(Json::read_file(path).root()), Strings{text});
}

TEST(JsonBuilder, RoundTripsArraysOfObjects) {
    const fs::path path = tests::Sandbox::scratch("write-records") / "doc.json";
    const Json::Builder builder;
    yyjson_mut_val *root = builder.new_object();
    yyjson_mut_val *records = builder.new_array();
    const std::array<const char *, 3> names = {"one", "two", "three"};

    for (int i = 0; i < 3; ++i) {
        yyjson_mut_val *record = builder.new_object();

        builder.add_string(record, "name", names.at(static_cast<std::size_t>(i)));
        builder.add_int(record, "index", i);
        Json::Builder::append_value(records, record);
    }

    builder.add_value(root, "records", records);
    builder.set_root(root);

    ASSERT_TRUE(builder.write_file(path));

    const Json::Doc doc = Json::read_file(path);
    std::vector<std::pair<std::string, int>> seen;

    Json::each_item(Json::obj_get(doc.root(), "records"), [&seen](yyjson_val *item) {
        seen.emplace_back(Json::obj_get_string(item, "name"), Json::obj_get_int(item, "index", -1));
    });

    EXPECT_EQ(seen, (std::vector<std::pair<std::string, int>>{{"one", 0}, {"two", 1}, {"three", 2}}));
}

TEST(JsonBuilder, RoundTripsAStringList) {
    const fs::path path = tests::Sandbox::scratch("write-strings") / "doc.json";
    const Strings items = {"a", "", "c d", "\"e\""};
    const Json::Builder builder;
    yyjson_mut_val *root = builder.new_array();

    for (const std::string &item : items) {
        builder.append_string(root, item);
    }

    builder.set_root(root);

    ASSERT_TRUE(builder.write_file(path));
    EXPECT_EQ(Json::as_string_list(Json::read_file(path).root()), items);
}

TEST(JsonBuilder, RoundTripsALargeArray) {
    const fs::path path = tests::Sandbox::scratch("write-large") / "doc.json";
    const Json::Builder builder;
    yyjson_mut_val *root = builder.new_array();

    for (std::size_t i = 0; i < LARGE; ++i) {
        builder.append_int(root, static_cast<int>(i));
    }

    builder.set_root(root);

    ASSERT_TRUE(builder.write_file(path));

    const Json::Doc doc = Json::read_file(path);
    std::vector<int> back(LARGE, -1);

    ASSERT_TRUE(Json::as_int_array(doc.root(), back.data(), static_cast<int>(LARGE)));
    EXPECT_EQ(back.front(), 0);
    EXPECT_EQ(back.back(), static_cast<int>(LARGE - 1));
}

TEST(JsonBuilder, RoundTripsAParsedDocument) {
    const fs::path root = tests::Sandbox::scratch("write-reparse");
    const std::string text = R"({"title":"Doom \u00e4","size":7,"on":true,"tags":["x","y"]})";
    const Json::Doc first = parse(text);
    const Json::Builder builder;
    yyjson_mut_val *copy = builder.new_object();
    std::deque<std::string> keys;

    Json::each_field(first.root(), [&](const std::string_view key, yyjson_val *val) {
        keys.emplace_back(key);
        const char *name = keys.back().c_str();

        if (yyjson_is_str(val)) {
            builder.add_string(copy, name, std::string_view(yyjson_get_str(val), yyjson_get_len(val)));
        } else if (yyjson_is_bool(val)) {
            builder.add_bool(copy, name, Json::as_bool(val));
        } else if (yyjson_is_int(val)) {
            builder.add_int(copy, name, Json::as_int(val));
        } else if (yyjson_is_arr(val)) {
            yyjson_mut_val *list = builder.new_array();

            Json::each_item(val, [&](yyjson_val *item) {
                builder.append_string(list, std::string_view(yyjson_get_str(item), yyjson_get_len(item)));
            });
            builder.add_value(copy, name, list);
        }
    });

    builder.set_root(copy);

    ASSERT_TRUE(builder.write_file(root / "copy.json"));

    const Json::Doc second = Json::read_file(root / "copy.json");

    ASSERT_TRUE(second.valid());
    EXPECT_EQ(field_names(second.root()), (Strings{"title", "size", "on", "tags"}));
    EXPECT_EQ(Json::obj_get_string(second.root(), "title"), "Doom \xC3\xA4");
    EXPECT_EQ(Json::obj_get_int(second.root(), "size"), 7);
    EXPECT_TRUE(Json::as_bool(Json::obj_get(second.root(), "on")));
    EXPECT_EQ(Json::as_string_list(Json::obj_get(second.root(), "tags")), (Strings{"x", "y"}));
}

TEST(JsonBuilder, CreatesMissingParentDirectories) {
    const fs::path path = tests::Sandbox::scratch("write-parents") / "a" / "b" / "c" / "doc.json";
    const Json::Builder builder;

    builder.set_root(builder.new_object());

    std::string error;

    ASSERT_TRUE(builder.write_file(path, &error)) << error;
    EXPECT_TRUE(fs::is_regular_file(path));
}

TEST(JsonBuilder, WritesARelativePathWithoutParent) {
    const fs::path root = tests::Sandbox::scratch("write-relative");
    const fs::path previous = fs::current_path();
    const Json::Builder builder;

    builder.set_root(builder.new_array());
    fs::current_path(root);

    const bool wrote = builder.write_file("bare.json");

    fs::current_path(previous);

    EXPECT_TRUE(wrote);
    EXPECT_EQ(slurp(root / "bare.json"), "[]\n");
}

TEST(JsonBuilder, ReplacesAnExistingFileAndLeavesNoTemporary) {
    const fs::path root = tests::Sandbox::scratch("write-replace");
    const fs::path path = root / "doc.json";
    const Json::Builder builder;
    yyjson_mut_val *object = builder.new_object();

    spill(path, "old contents that are much longer than the new ones will be");
    builder.add_int(object, "v", 2);
    builder.set_root(object);

    ASSERT_TRUE(builder.write_file(path));
    EXPECT_EQ(Json::obj_get_int(Json::read_file(path).root(), "v"), 2);
    EXPECT_FALSE(fs::exists(root / "doc.json.new"));
}

TEST(JsonBuilder, WritesTheSameBuilderTwice) {
    const fs::path root = tests::Sandbox::scratch("write-twice");
    const Json::Builder builder;
    yyjson_mut_val *object = builder.new_object();

    builder.add_int(object, "v", 3);
    builder.set_root(object);

    ASSERT_TRUE(builder.write_file(root / "one.json"));
    ASSERT_TRUE(builder.write_file(root / "two.json"));
    EXPECT_EQ(slurp(root / "one.json"), slurp(root / "two.json"));
}

TEST(JsonBuilder, ParentThatIsAFileFails) {
    const fs::path root = tests::Sandbox::scratch("write-parent-file");

    spill(root / "blocker", "x");

    const Json::Builder builder;

    builder.set_root(builder.new_object());

    std::string error;

    EXPECT_FALSE(builder.write_file(root / "blocker" / "doc.json", &error));
    EXPECT_FALSE(error.empty());
}

TEST(JsonBuilder, TargetThatIsADirectoryFailsAndCleansUp) {
    const fs::path root = tests::Sandbox::scratch("write-target-directory");

    fs::create_directories(root / "doc.json" / "inside");

    const Json::Builder builder;

    builder.set_root(builder.new_object());

    std::string error;

    EXPECT_FALSE(builder.write_file(root / "doc.json", &error));
    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(fs::is_directory(root / "doc.json" / "inside")) << "the directory is left alone";
    EXPECT_FALSE(fs::exists(root / "doc.json.new"));
}

TEST(JsonBuilder, InvalidUtf8FailsAndKeepsTheOldFile) {
    const fs::path root = tests::Sandbox::scratch("write-invalid");
    const fs::path path = root / "doc.json";
    const Json::Builder builder;
    yyjson_mut_val *object = builder.new_object();

    spill(path, "{\"kept\": 1}\n");
    builder.add_string(object, "bad", "\xFF\xFE");
    builder.set_root(object);

    std::string error;

    EXPECT_FALSE(builder.write_file(path, &error));
    EXPECT_FALSE(error.empty());
    EXPECT_EQ(slurp(path), "{\"kept\": 1}\n");
    EXPECT_FALSE(fs::exists(root / "doc.json.new"));
}

TEST(JsonBuilder, FailureWithoutErrorPointer) {
    const fs::path root = tests::Sandbox::scratch("write-quiet");

    spill(root / "blocker", "x");

    const Json::Builder builder;

    builder.set_root(builder.new_object());

    EXPECT_FALSE(builder.write_file(root / "blocker" / "doc.json"));
}

TEST(JsonBuilder, NullContainersAreIgnored) {
    const fs::path path = tests::Sandbox::scratch("write-null") / "doc.json";
    const Json::Builder builder;
    yyjson_mut_val *root = builder.new_object();

    builder.add_string(nullptr, "a", "x");
    builder.add_int(nullptr, "b", 1);
    builder.add_bool(nullptr, "c", true);
    builder.add_value(nullptr, "d", builder.new_array());
    builder.add_value(root, "e", nullptr);
    builder.append_string(nullptr, "x");
    builder.append_int(nullptr, 1);
    Json::Builder::append_value(nullptr, builder.new_object());
    Json::Builder::append_value(builder.new_array(), nullptr);
    builder.set_root(root);

    ASSERT_TRUE(builder.write_file(path));
    EXPECT_EQ(slurp(path), "{}\n");
}

TEST(JsonBuilder, WithoutARootFailsAndWritesNothing) {
    const fs::path root = tests::Sandbox::scratch("write-no-root");
    const Json::Builder builder;
    std::string error;

    EXPECT_FALSE(builder.write_file(root / "doc.json", &error));
    EXPECT_FALSE(error.empty());
    EXPECT_FALSE(fs::exists(root / "doc.json"));
    EXPECT_FALSE(fs::exists(root / "doc.json.new"));
}

TEST(JsonReadData, RejectsALoneSurrogate) {
    EXPECT_FALSE(rejection(R"("\ud800")").empty());
    EXPECT_FALSE(rejection(R"("\ude00x")").empty());
}
