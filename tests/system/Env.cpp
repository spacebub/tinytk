// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cstddef>
#include <string>

#include <gtest/gtest.h>

#include "ttk/system/Env.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace {
constexpr const char *NAME = "TTK_TEST_ENV_VALUE";

class Env : public ::testing::Test {
protected:
    void SetUp() override { ttk::Env::unset(NAME); }

    void TearDown() override { ttk::Env::unset(NAME); }
};
}

TEST_F(Env, UnsetVariableIsEmpty) {
    EXPECT_EQ(ttk::Env::get(NAME), "");
}

TEST_F(Env, SetThenGet) {
    ttk::Env::set(NAME, "value");

    EXPECT_EQ(ttk::Env::get(NAME), "value");
}

TEST_F(Env, SetReplacesExistingValue) {
    ttk::Env::set(NAME, "first");
    ttk::Env::set(NAME, "second");

    EXPECT_EQ(ttk::Env::get(NAME), "second");
}

TEST_F(Env, ReplacesWithShorterValue) {
    ttk::Env::set(NAME, "a long first value");
    ttk::Env::set(NAME, "x");

    EXPECT_EQ(ttk::Env::get(NAME), "x");
}

TEST_F(Env, EmptyValueReadsAsEmpty) {
    ttk::Env::set(NAME, "something");
    ttk::Env::set(NAME, "");

    EXPECT_EQ(ttk::Env::get(NAME), "");
}

TEST_F(Env, UnsetAfterSetReadsAsEmpty) {
    ttk::Env::set(NAME, "gone soon");
    ttk::Env::unset(NAME);

    EXPECT_EQ(ttk::Env::get(NAME), "");
}

TEST_F(Env, KeepsUtf8Value) {
    const std::string value = "\xE6\x97\xA5\xE6\x9C\xAC \xF0\x9F\x98\x80";

    ttk::Env::set(NAME, value.c_str());

    EXPECT_EQ(ttk::Env::get(NAME), value);
}

TEST_F(Env, KeepsEqualsSignsAndBlanks) {
    ttk::Env::set(NAME, " a=b = c ");

    EXPECT_EQ(ttk::Env::get(NAME), " a=b = c ");
}

TEST_F(Env, KeepsShellMetacharacters) {
    ttk::Env::set(NAME, R"($HOME 'q' "d" \n ;|&)");

    EXPECT_EQ(ttk::Env::get(NAME), R"($HOME 'q' "d" \n ;|&)");
}

TEST_F(Env, KeepsALongValue) {
#ifdef _WIN32
    // A Windows variable holds at most 32767 characters.
    constexpr std::size_t SIZE = 32000;
#else
    constexpr std::size_t SIZE = std::size_t{64} * 1024;
#endif
    const std::string value(SIZE, 'v');

    ttk::Env::set(NAME, value.c_str());

    EXPECT_EQ(ttk::Env::get(NAME), value);
}

TEST_F(Env, UnsetMissingVariableDoesNothing) {
    ttk::Env::unset(NAME);

    EXPECT_EQ(ttk::Env::get(NAME), "");
}

#ifdef _WIN32
TEST_F(Env, NamesIgnoreCase) {
    ttk::Env::set(NAME, "upper");

    EXPECT_EQ(ttk::Env::get("ttk_test_env_value"), "upper");
}

TEST_F(Env, SetReachesTheWin32Environment) {
    ttk::Env::set(NAME, "seen");

    std::wstring value(16, L'\0');
    const DWORD written = GetEnvironmentVariableW(L"TTK_TEST_ENV_VALUE", value.data(), 16);

    value.resize(written);

    EXPECT_EQ(value, L"seen") << "libraries reading the Win32 copy see it too";
}

TEST_F(Env, OverlongValueLeavesTheVariableAsItWas) {
    ttk::Env::set(NAME, "kept");
    ttk::Env::set(NAME, std::string(std::size_t{64} * 1024, 'v').c_str());

    EXPECT_EQ(ttk::Env::get(NAME), "kept");
}

TEST_F(Env, UnsetLeavesNeitherCopy) {
    ttk::Env::set(NAME, "gone soon");
    ttk::Env::unset(NAME);

    EXPECT_EQ(ttk::Env::get(NAME), "");
    EXPECT_EQ(GetEnvironmentVariableW(L"TTK_TEST_ENV_VALUE", nullptr, 0), 0U);
}
#else
TEST_F(Env, NamesAreCaseSensitive) {
    ttk::Env::set(NAME, "upper");

    EXPECT_EQ(ttk::Env::get("ttk_test_env_value"), "");
}
#endif

TEST_F(Env, SandboxPointsHomeAway) {
    EXPECT_FALSE(ttk::Env::get("HOME").empty());
    EXPECT_NE(ttk::Env::get("HOME").find("tinytk-tests-"), std::string::npos) << "the sandbox owns HOME";
}
