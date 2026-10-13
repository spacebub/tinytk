// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cstddef>
#include <cstdlib>
#include <string>

#include <gtest/gtest.h>

#include "ttk/system/Env.h"

namespace {
    constexpr const char *NAME = "TTK_TEST_ENV_VALUE";

    void unset(const char *name) {
        unsetenv(name); // NOLINT(concurrency-mt-unsafe): the tests run on one thread.
    }

    class Env : public ::testing::Test {
    protected:
        void SetUp() override { unset(NAME); }

        void TearDown() override { unset(NAME); }
    };
}

TEST_F(Env, UnsetVariableIsEmpty) {
    EXPECT_EQ(ttk::Env::get(NAME), "");
}

TEST_F(Env, SetThenGet) {
    ttk::Env::set(NAME, "value");

    EXPECT_EQ(ttk::Env::get(NAME), "value");
}

TEST_F(Env, SetIsVisibleToTheCRuntime) {
    ttk::Env::set(NAME, "seen");

    const char *raw = std::getenv(NAME); // NOLINT(concurrency-mt-unsafe): the tests run on one thread.

    ASSERT_NE(raw, nullptr);
    EXPECT_STREQ(raw, "seen");
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
    EXPECT_NE(std::getenv(NAME), nullptr) // NOLINT(concurrency-mt-unsafe): the tests run on one thread.
            << "set with an empty value still defines the variable";
}

TEST_F(Env, UnsetAfterSetReadsAsEmpty) {
    ttk::Env::set(NAME, "gone soon");
    unset(NAME);

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
    constexpr std::size_t SIZE = std::size_t{64} * 1024;
    const std::string value(SIZE, 'v');

    ttk::Env::set(NAME, value.c_str());

    EXPECT_EQ(ttk::Env::get(NAME), value);
}

TEST_F(Env, NamesAreCaseSensitive) {
    ttk::Env::set(NAME, "upper");

    EXPECT_EQ(ttk::Env::get("ttk_test_env_value"), "");
}

TEST_F(Env, SandboxPointsHomeAway) {
    EXPECT_FALSE(ttk::Env::get("HOME").empty());
    EXPECT_NE(ttk::Env::get("HOME").find("tinytk-tests-"), std::string::npos) << "the sandbox owns HOME";
}
