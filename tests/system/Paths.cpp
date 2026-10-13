// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include <gtest/gtest.h>

#ifndef _WIN32
#include <pwd.h>
#include <unistd.h>
#endif

#include "support/Sandbox.h"
#include "ttk/system/Env.h"
#include "ttk/system/Paths.h"

namespace {
namespace fs = std::filesystem;

#ifdef _WIN32
constexpr std::array<const char *, 5> TOUCHED = {
        "USERPROFILE",
        "HOMEDRIVE",
        "HOMEPATH",
        "APPDATA",
        "PROGRAMDATA",
};
#else
constexpr std::array<const char *, 5> TOUCHED = {
        "HOME",
        "XDG_CONFIG_HOME",
        "XDG_DATA_HOME",
        "XDG_CONFIG_DIRS",
        "XDG_CACHE_HOME",
};
#endif

class Paths : public ::testing::Test {
protected:
    void SetUp() override {
        for (std::size_t i = 0; i < TOUCHED.size(); ++i) {
            _saved[i] = ttk::Env::get(TOUCHED[i]);
        }

        _application = ttk::Paths::application();
        _executable = ttk::Paths::executable();
    }

    void TearDown() override {
        // Paths reads an empty variable as unset, so one that was empty comes back unset.
        for (std::size_t i = 0; i < TOUCHED.size(); ++i) {
            if (_saved[i].empty()) {
                ttk::Env::unset(TOUCHED[i]);
            } else {
                ttk::Env::set(TOUCHED[i], _saved[i].c_str());
            }
        }

        ttk::Paths::set_application(_application);
        ttk::Paths::set_executable(_executable);
    }

    static fs::path sandbox(const char *part) { return tests::Sandbox::root() / part; }

private:
    std::array<std::string, TOUCHED.size()> _saved{};
    std::string _application;
    fs::path _executable;
};
}

TEST_F(Paths, ApplicationDefaultsToTinytk) {
    EXPECT_EQ(ttk::Paths::application(), "tinytk");
}

TEST_F(Paths, SetApplicationIsReadBack) {
    ttk::Paths::set_application("demo");

    EXPECT_EQ(ttk::Paths::application(), "demo");
}

TEST_F(Paths, SetApplicationKeepsUtf8) {
    ttk::Paths::set_application("\xE6\x97\xA5\xE6\x9C\xAC");

    EXPECT_EQ(ttk::Paths::application(), "\xE6\x97\xA5\xE6\x9C\xAC");
    EXPECT_EQ(ttk::Paths::config_directory().filename(), fs::path("\xE6\x97\xA5\xE6\x9C\xAC"));
}

TEST_F(Paths, ConfigDirectoryIsNotCreated) {
    ttk::Paths::set_application("never-created");

    const fs::path directory = ttk::Paths::config_directory();

    EXPECT_FALSE(fs::exists(directory));
}

#ifndef _WIN32
TEST_F(Paths, ConfigDirectoryIsUnderXdgConfigHome) {
    EXPECT_EQ(ttk::Paths::config_directory(), sandbox("config") / "tinytk");
}

TEST_F(Paths, ConfigDirectoryEndsInTheApplication) {
    ttk::Paths::set_application("demo");

    EXPECT_EQ(ttk::Paths::config_directory(), sandbox("config") / "demo");
}

TEST_F(Paths, ConfigDirectoryFallsBackToHomeDotConfig) {
    ttk::Env::unset("XDG_CONFIG_HOME");

    EXPECT_EQ(ttk::Paths::config_directory(), sandbox("home") / ".config" / "tinytk");
}

TEST_F(Paths, EmptyXdgConfigHomeCountsAsUnset) {
    ttk::Env::set("XDG_CONFIG_HOME", "");

    EXPECT_EQ(ttk::Paths::config_directory(), sandbox("home") / ".config" / "tinytk");
}

TEST_F(Paths, ConfigDirectoryFollowsAChangedVariable) {
    const fs::path elsewhere = tests::Sandbox::scratch("config-elsewhere");

    ttk::Env::set("XDG_CONFIG_HOME", elsewhere.c_str());

    EXPECT_EQ(ttk::Paths::config_directory(), elsewhere / "tinytk") << "nothing is cached between calls";
}

TEST_F(Paths, DataDirectoryIsUnderXdgDataHome) {
    EXPECT_EQ(ttk::Paths::data_directory(), sandbox("data") / "tinytk");
}

TEST_F(Paths, DataDirectoryEndsInTheApplication) {
    ttk::Paths::set_application("demo");

    EXPECT_EQ(ttk::Paths::data_directory(), sandbox("data") / "demo");
}

TEST_F(Paths, DataDirectoryFallsBackToHomeLocalShare) {
    ttk::Env::unset("XDG_DATA_HOME");

    EXPECT_EQ(ttk::Paths::data_directory(), sandbox("home") / ".local" / "share" / "tinytk");
}

TEST_F(Paths, EmptyXdgDataHomeCountsAsUnset) {
    ttk::Env::set("XDG_DATA_HOME", "");

    EXPECT_EQ(ttk::Paths::data_directory(), sandbox("home") / ".local" / "share" / "tinytk");
}

TEST_F(Paths, SystemConfigDirectoryIsUnderXdgConfigDirs) {
    EXPECT_EQ(ttk::Paths::system_config_directory(), sandbox("etc") / "tinytk");
}

TEST_F(Paths, SystemConfigDirectoryUsesTheLastEntry) {
    ttk::Env::set("XDG_CONFIG_DIRS", "/first/xdg:/second/xdg:/last/xdg");

    EXPECT_EQ(ttk::Paths::system_config_directory(), fs::path("/last/xdg") / "tinytk");
}

TEST_F(Paths, SystemConfigDirectoryWithOneEntry) {
    ttk::Env::set("XDG_CONFIG_DIRS", "/only/xdg");

    EXPECT_EQ(ttk::Paths::system_config_directory(), fs::path("/only/xdg") / "tinytk");
}

TEST_F(Paths, SystemConfigDirectoryDefaultsToEtcXdg) {
    ttk::Env::unset("XDG_CONFIG_DIRS");

    EXPECT_EQ(ttk::Paths::system_config_directory(), fs::path("/etc/xdg") / "tinytk");
}

TEST_F(Paths, EmptyXdgConfigDirsCountsAsUnset) {
    ttk::Env::set("XDG_CONFIG_DIRS", "");

    EXPECT_EQ(ttk::Paths::system_config_directory(), fs::path("/etc/xdg") / "tinytk");
}

TEST_F(Paths, SystemConfigDirectoryEndsInTheApplication) {
    ttk::Paths::set_application("demo");
    ttk::Env::set("XDG_CONFIG_DIRS", "/a:/b");

    EXPECT_EQ(ttk::Paths::system_config_directory(), fs::path("/b") / "demo");
}

TEST_F(Paths, HomeDirectoryIsHome) {
    EXPECT_EQ(ttk::Paths::home_directory(), sandbox("home"));
}

TEST_F(Paths, HomeDirectoryFallsBackToThePasswordDatabase) {
    ttk::Env::unset("HOME");

    const passwd *entry = getpwuid(getuid()); // NOLINT(concurrency-mt-unsafe): the tests run on one thread.

    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(ttk::Paths::home_directory(), fs::path(entry->pw_dir));
}

TEST_F(Paths, EmptyHomeFallsBackToThePasswordDatabase) {
    ttk::Env::set("HOME", "");

    const passwd *entry = getpwuid(getuid()); // NOLINT(concurrency-mt-unsafe): the tests run on one thread.

    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(ttk::Paths::home_directory(), fs::path(entry->pw_dir));
}

TEST_F(Paths, FallbacksFollowAChangedHome) {
    const fs::path home = tests::Sandbox::scratch("other-home");

    ttk::Env::set("HOME", home.c_str());
    ttk::Env::unset("XDG_CONFIG_HOME");
    ttk::Env::unset("XDG_DATA_HOME");

    EXPECT_EQ(ttk::Paths::config_directory(), home / ".config" / "tinytk");
    EXPECT_EQ(ttk::Paths::data_directory(), home / ".local" / "share" / "tinytk");
}
#else
TEST_F(Paths, ConfigDirectoryIsUnderAppData) {
    EXPECT_EQ(ttk::Paths::config_directory(), sandbox("appdata") / "tinytk");
}

TEST_F(Paths, ConfigDirectoryEndsInTheApplication) {
    ttk::Paths::set_application("demo");

    EXPECT_EQ(ttk::Paths::config_directory(), sandbox("appdata") / "demo");
}

TEST_F(Paths, ConfigDirectoryFallsBackBesideTheExecutable) {
    ttk::Env::unset("APPDATA");

    EXPECT_EQ(ttk::Paths::config_directory(), ttk::Paths::executable_directory() / "tinytk");
}

TEST_F(Paths, EmptyAppDataCountsAsUnset) {
    ttk::Env::set("APPDATA", "");

    EXPECT_EQ(ttk::Paths::config_directory(), ttk::Paths::executable_directory() / "tinytk");
}

TEST_F(Paths, ConfigDirectoryFollowsAChangedVariable) {
    const fs::path elsewhere = tests::Sandbox::scratch("config-elsewhere");

    ttk::Env::set("APPDATA", elsewhere.string().c_str());

    EXPECT_EQ(ttk::Paths::config_directory(), elsewhere / "tinytk") << "nothing is cached between calls";
}

TEST_F(Paths, DataDirectoryIsTheConfigDirectory) {
    EXPECT_EQ(ttk::Paths::data_directory(), sandbox("appdata") / "tinytk");

    ttk::Paths::set_application("demo");

    EXPECT_EQ(ttk::Paths::data_directory(), ttk::Paths::config_directory());
}

TEST_F(Paths, SystemConfigDirectoryIsUnderProgramData) {
    ttk::Env::set("PROGRAMDATA", sandbox("programdata").string().c_str());

    EXPECT_EQ(ttk::Paths::system_config_directory(), sandbox("programdata") / "tinytk");
}

TEST_F(Paths, SystemConfigDirectoryWithoutProgramDataIsEmpty) {
    ttk::Env::unset("PROGRAMDATA");

    EXPECT_TRUE(ttk::Paths::system_config_directory().empty());
}

TEST_F(Paths, HomeDirectoryIsTheUserProfile) {
    EXPECT_EQ(ttk::Paths::home_directory(), sandbox("home"));
}

TEST_F(Paths, HomeDirectoryFallsBackToHomeDriveAndPath) {
    ttk::Env::unset("USERPROFILE");
    ttk::Env::set("HOMEDRIVE", "C:");
    ttk::Env::set("HOMEPATH", R"(\Users\someone)");

    EXPECT_EQ(ttk::Paths::home_directory(), fs::path(R"(C:\Users\someone)"));
}

TEST_F(Paths, HomeDirectoryNeedsBothHomeDriveAndPath) {
    ttk::Env::unset("USERPROFILE");
    ttk::Env::set("HOMEDRIVE", "C:");
    ttk::Env::unset("HOMEPATH");

    EXPECT_TRUE(ttk::Paths::home_directory().empty());
}
#endif

TEST_F(Paths, SetExecutableMakesThePathCanonical) {
    const fs::path bin = tests::Sandbox::scratch("exe") / "bin";

    fs::create_directories(bin);
    std::ofstream(bin / "program").put('x');

    ttk::Paths::set_executable(bin / ".." / "bin" / "." / "program");

    EXPECT_EQ(ttk::Paths::executable(), fs::canonical(bin / "program"));
}

TEST_F(Paths, SetExecutableResolvesSymbolicLinks) {
    const fs::path root = tests::Sandbox::scratch("exe-link");

    std::ofstream(root / "real").put('x');

    std::error_code code;

    // Windows only makes them with developer mode or elevation.
    fs::create_symlink(root / "real", root / "link", code);

    if (code) {
        GTEST_SKIP() << "no symbolic links here: " << code.message();
    }

    ttk::Paths::set_executable(root / "link");

    EXPECT_EQ(ttk::Paths::executable(), fs::canonical(root / "real"));
}

TEST_F(Paths, SetExecutableResolvesARelativePath) {
    const fs::path root = tests::Sandbox::scratch("exe-relative");
    const fs::path previous = fs::current_path();

    std::ofstream(root / "program").put('x');
    fs::current_path(root);

    ttk::Paths::set_executable("program");

    fs::current_path(previous);

    EXPECT_EQ(ttk::Paths::executable(), fs::canonical(root / "program"));
}

TEST_F(Paths, ExecutableDirectoryHoldsTheExecutable) {
    const fs::path root = tests::Sandbox::scratch("exe-directory");

    std::ofstream(root / "program").put('x');

    ttk::Paths::set_executable(root / "program");

    EXPECT_EQ(ttk::Paths::executable_directory(), fs::canonical(root));
}

TEST_F(Paths, ExecutableDirectoryWithoutAnExecutableIsTheCurrentDirectory) {
    ttk::Paths::set_executable({});

    ASSERT_TRUE(ttk::Paths::executable().empty());
    EXPECT_EQ(ttk::Paths::executable_directory(), fs::current_path());
}
