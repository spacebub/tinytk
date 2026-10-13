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

#include <gtest/gtest.h>

#include "ttk/system/Paths.h"
#include "ttk/util/Clipboard.h"
#include "ttk/util/Clock.h"
#include "ttk/util/Desktop.h"
#include "ttk/util/Window.h"

namespace {
    namespace fs = std::filesystem;

    constexpr std::array<std::string_view, 5> FAMILIES = {
            "JetBrains Mono", "Fira Code", "DejaVu Sans Mono", "Noto Sans Mono", "monospace",
    };

    void plant(const fs::path &directory, const char *name) {
        fs::create_directories(directory);
        std::ofstream(directory / name).put('x');
    }

    class DesktopFonts : public ::testing::Test {
    protected:
        void TearDown() override {
            std::error_code code;
            const fs::path home = ttk::Paths::home_directory();

            fs::remove_all(home / ".local" / "share" / "fonts", code);
            fs::remove_all(home / ".fonts", code);
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

TEST(DesktopDrives, EmptyOutsideWindows) {
    EXPECT_TRUE(ttk::Desktop::drives().empty());
}

TEST_F(DesktopFonts, ReturnsAKnownFamily) {
    const std::string family = ttk::Desktop::monospace_family();

    EXPECT_NE(std::ranges::find(FAMILIES, family), FAMILIES.end()) << family;
}

TEST_F(DesktopFonts, FindsTheFavouriteInTheUserFontDirectory) {
    plant(ttk::Paths::home_directory() / ".local" / "share" / "fonts" / "jetbrains", "JetBrainsMono-Regular.ttf");

    EXPECT_EQ(ttk::Desktop::monospace_family(), "JetBrains Mono");
}

TEST_F(DesktopFonts, FindsTheFavouriteInDotFonts) {
    plant(ttk::Paths::home_directory() / ".fonts", "JetBrainsMono-Bold.otf");

    EXPECT_EQ(ttk::Desktop::monospace_family(), "JetBrains Mono");
}

TEST_F(DesktopFonts, MatchesFileNamesWithoutCase) {
    plant(ttk::Paths::home_directory() / ".fonts", "JETBRAINSMONO-ITALIC.TTF");

    EXPECT_EQ(ttk::Desktop::monospace_family(), "JetBrains Mono");
}

TEST_F(DesktopFonts, WalksTheDirectoriesAgainOnEachCall) {
    const std::string before = ttk::Desktop::monospace_family();

    plant(ttk::Paths::home_directory() / ".fonts", "JetBrainsMono-Regular.ttf");

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
