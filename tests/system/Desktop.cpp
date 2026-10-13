// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

#include "ttk/system/Env.h"
#include "ttk/system/Paths.h"
#include "ttk/util/Clipboard.h"
#include "ttk/util/Clock.h"
#include "ttk/util/Desktop.h"
#include "ttk/util/Window.h"

namespace {
namespace fs = std::filesystem;

using namespace std::string_view_literals;

constexpr std::array FAMILIES = {
        "JetBrains Mono"sv,
        "Fira Code"sv,
        "DejaVu Sans Mono"sv,
        "Noto Sans Mono"sv,
#ifdef _WIN32
        "Consolas"sv,
        "Courier New"sv,
#elifdef __APPLE__
        "Menlo"sv,
        "Monaco"sv,
#endif
        "monospace"sv,
};

// Where the user installs fonts of their own.
fs::path user_fonts() {
#ifdef _WIN32
    return ttk::Paths::home_directory() / "AppData" / "Local" / "Microsoft" / "Windows" / "Fonts";
#elifdef __APPLE__
    return ttk::Paths::home_directory() / "Library" / "Fonts";
#else
    return ttk::Paths::home_directory() / ".local" / "share" / "fonts";
#endif
}

void plant(const fs::path &directory, const char *name) {
    fs::create_directories(directory);
    std::ofstream(directory / name).put('x');
}

class DesktopFonts : public ::testing::Test {
protected:
    void TearDown() override {
        std::error_code code;
        fs::remove_all(user_fonts(), code);
        fs::remove_all(ttk::Paths::home_directory() / ".fonts", code);
    }
};
}

TEST(DesktopOpen, EmptyTargetFails) {
    std::string why;

    EXPECT_FALSE(ttk::Desktop::open("", &why));
    EXPECT_FALSE(why.empty()) << "a reason is stored";
}

TEST(DesktopOpen, EmptyTargetFailsWithoutAReasonPointer) {
    EXPECT_FALSE(ttk::Desktop::open(""));
}

#ifdef _WIN32
TEST(DesktopDrives, ListTheSystemDriveInLetterOrder) {
    const std::vector<std::string> drives = ttk::Desktop::drives();

    EXPECT_NE(std::ranges::find(drives, ttk::Env::get("SystemDrive") + "/"), drives.end());
    EXPECT_TRUE(std::ranges::is_sorted(drives));
}
#else
TEST(DesktopDrives, EmptyOutsideWindows) {
    EXPECT_TRUE(ttk::Desktop::drives().empty());
}
#endif

TEST_F(DesktopFonts, ReturnsAKnownFamily) {
    const std::string family = ttk::Desktop::monospace_family();

    EXPECT_NE(std::ranges::find(FAMILIES, family), FAMILIES.end()) << family;
}

TEST_F(DesktopFonts, FindsTheFavouriteInTheUserFontDirectory) {
    plant(user_fonts() / "jetbrains", "JetBrainsMono-Regular.ttf");

    EXPECT_EQ(ttk::Desktop::monospace_family(), "JetBrains Mono");
}

#if !defined(_WIN32) && !defined(__APPLE__)
TEST_F(DesktopFonts, FindsTheFavouriteInDotFonts) {
    plant(ttk::Paths::home_directory() / ".fonts", "JetBrainsMono-Bold.otf");

    EXPECT_EQ(ttk::Desktop::monospace_family(), "JetBrains Mono");
}
#endif

TEST_F(DesktopFonts, MatchesFileNamesWithoutCase) {
    plant(user_fonts(), "JETBRAINSMONO-ITALIC.TTF");

    EXPECT_EQ(ttk::Desktop::monospace_family(), "JetBrains Mono");
}

TEST_F(DesktopFonts, WalksTheDirectoriesAgainOnEachCall) {
    const std::string before = ttk::Desktop::monospace_family();

    plant(user_fonts(), "JetBrainsMono-Regular.ttf");

    EXPECT_EQ(ttk::Desktop::monospace_family(), "JetBrains Mono");
    EXPECT_NE(std::ranges::find(FAMILIES, before), FAMILIES.end()) << before;
}

TEST(ClipboardRead, IsEmptyWithoutTheVideoSubsystem) {
    EXPECT_EQ(ttk::Clipboard::read(), "") << "SDL video is never started here, so the clipboard cannot be read";
}

TEST(ClipboardWrite, DoesNothingReadableWithoutTheVideoSubsystem) {
    ttk::Clipboard::write("not on any desktop");

    EXPECT_EQ(ttk::Clipboard::read(), "");
}

TEST(ClockInterface, IsAnAbstractNonCopyableBase) {
    EXPECT_TRUE(std::is_abstract_v<ttk::Clock>);
    EXPECT_TRUE(std::has_virtual_destructor_v<ttk::Clock>);
    EXPECT_FALSE(std::is_copy_constructible_v<ttk::Clock>);
    EXPECT_FALSE(std::is_move_constructible_v<ttk::Clock>);
    EXPECT_FALSE(std::is_copy_assignable_v<ttk::Clock>);
    EXPECT_FALSE(std::is_move_assignable_v<ttk::Clock>);
}

TEST(WindowInterface, IsAnAbstractNonCopyableBase) {
    EXPECT_TRUE(std::is_abstract_v<ttk::Window>);
    EXPECT_TRUE(std::has_virtual_destructor_v<ttk::Window>);
    EXPECT_FALSE(std::is_copy_constructible_v<ttk::Window>);
    EXPECT_FALSE(std::is_move_constructible_v<ttk::Window>);
    EXPECT_FALSE(std::is_copy_assignable_v<ttk::Window>);
    EXPECT_FALSE(std::is_move_assignable_v<ttk::Window>);
}
