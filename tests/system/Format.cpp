// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <climits>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include "support/Sandbox.h"
#include "ttk/system/Paths.h"
#include "ttk/util/Format.h"

namespace {
    namespace fs = std::filesystem;

    constexpr const char *ELLIPSIS = "\xE2\x80\xA6";

    constexpr unsigned long long KB = 1024ULL;
    constexpr unsigned long long MB = KB * KB;
    constexpr unsigned long long GB = MB * KB;

    std::string home() {
        return ttk::Format::from_path(ttk::Paths::home_directory());
    }

    std::string touch(const fs::path &path) {
        std::ofstream(path).put('x');

        return ttk::Format::from_path(path);
    }
}

TEST(FormatFromPath, KeepsForwardSlashes) {
    EXPECT_EQ(ttk::Format::from_path("/usr/share/doom"), "/usr/share/doom");
}

TEST(FormatFromPath, EmptyStaysEmpty) {
    EXPECT_EQ(ttk::Format::from_path({}), "");
}

TEST(FormatFromPath, KeepsUtf8) {
    EXPECT_EQ(ttk::Format::from_path("/a/\xE6\x97\xA5\xE6\x9C\xAC.wad"), "/a/\xE6\x97\xA5\xE6\x9C\xAC.wad");
}

TEST(FormatFromPath, JoinsWithForwardSlashes) {
    EXPECT_EQ(ttk::Format::from_path(fs::path("a") / "b" / "c.txt"), "a/b/c.txt");
}

TEST(FormatPrettyPath, HomeBecomesTilde) {
    EXPECT_EQ(ttk::Format::pretty_path(home() + "/games/doom.wad"), "~/games/doom.wad");
}

TEST(FormatPrettyPath, HomeWithTrailingSlashBecomesTildeSlash) {
    EXPECT_EQ(ttk::Format::pretty_path(home() + "/"), "~/");
}

TEST(FormatPrettyPath, HomeWithoutSlashIsUnchanged) {
    EXPECT_EQ(ttk::Format::pretty_path(home()), home());
}

TEST(FormatPrettyPath, SiblingSharingTheHomePrefixIsUnchanged) {
    const std::string sibling = home() + "other/file";

    EXPECT_EQ(ttk::Format::pretty_path(sibling), sibling) << "home must be followed by a slash";
}

TEST(FormatPrettyPath, HomeInTheMiddleIsUnchanged) {
    const std::string nested = "/mnt" + home() + "/file";

    EXPECT_EQ(ttk::Format::pretty_path(nested), nested);
}

TEST(FormatPrettyPath, OtherPathsAreUnchanged) {
    EXPECT_EQ(ttk::Format::pretty_path("/usr/share/doom"), "/usr/share/doom");
    EXPECT_EQ(ttk::Format::pretty_path("relative/path"), "relative/path");
    EXPECT_EQ(ttk::Format::pretty_path(""), "");
}

TEST(FormatPrettyPath, BackslashesAreUnchanged) {
    std::string backslashed = home() + "/x";

    for (char &letter : backslashed) {
        letter = letter == '/' ? '\\' : letter;
    }

    EXPECT_EQ(ttk::Format::pretty_path(backslashed), backslashed) << "only forward slashes are recognised";
}

TEST(FormatPrettyPath, KeepsUtf8AfterHome) {
    EXPECT_EQ(ttk::Format::pretty_path(home() + "/\xC3\xA4"), "~/\xC3\xA4");
}

TEST(FormatFitPath, FittingPathIsOnlyPrettied) {
    EXPECT_EQ(ttk::Format::fit_path("/usr/share/doom", 15), "/usr/share/doom");
    EXPECT_EQ(ttk::Format::fit_path(home() + "/doom", 6), "~/doom");
}

TEST(FormatFitPath, ZeroOrNegativeRoomReturnsThePrettyPath) {
    const std::string longer = "/a/very/long/path/to/a/file.wad";

    EXPECT_EQ(ttk::Format::fit_path(longer, 0), longer);
    EXPECT_EQ(ttk::Format::fit_path(longer, -5), longer);
    EXPECT_EQ(ttk::Format::fit_path(home() + "/x/y", 0), "~/x/y");
}

TEST(FormatFitPath, DropsWholeLeadingDirectories) {
    EXPECT_EQ(ttk::Format::fit_path("/usr/share/games/doom/doom2.wad", 17), std::string(ELLIPSIS) + "/doom/doom2.wad");
}

TEST(FormatFitPath, DropsOnlyAsManyDirectoriesAsNeeded) {
    EXPECT_EQ(ttk::Format::fit_path("/aaaa/bbbb/cccc/dddd/file", 20), std::string(ELLIPSIS) + "/cccc/dddd/file");
}

TEST(FormatFitPath, ResultFitsAndEndsInTheFileName) {
    const std::string path = "/one/two/three/four/five/six/seven/name.txt";

    for (int room = 12; std::cmp_less(room, path.size()); ++room) {
        const std::string fitted = ttk::Format::fit_path(path, room);

        ASSERT_TRUE(fitted.starts_with(ELLIPSIS)) << room;
        EXPECT_LE(static_cast<int>(fitted.size()) - 2, room) << "the ellipsis is one column: " << fitted;
        EXPECT_TRUE(fitted.ends_with("/name.txt")) << fitted;
    }
}

TEST(FormatFitPath, ShortensAfterPrettying) {
    EXPECT_EQ(ttk::Format::fit_path(home() + "/games/doom/doom2.wad", 17), std::string(ELLIPSIS) + "/doom/doom2.wad");
}

TEST(FormatFitPath, FileNameThatDoesNotFitReturnsThePrettyPath) {
    const std::string path = "/dir/a-file-name-that-is-far-too-long.wad";

    EXPECT_EQ(ttk::Format::fit_path(path, 10), path);
    EXPECT_EQ(ttk::Format::fit_path(home() + "/dir/a-file-name-that-is-far-too-long.wad", 10),
              "~/dir/a-file-name-that-is-far-too-long.wad");
}

TEST(FormatFitPath, PathWithoutSlashesIsUnchanged) {
    EXPECT_EQ(ttk::Format::fit_path("averyveryverylongname", 5), "averyveryverylongname");
}

TEST(FormatFitPath, CountsBytesForUtf8Names) {
    const std::string wide = "\xE6\x97\xA5\xE6\x9C\xAC";
    const std::string path = "/" + wide + "/" + wide + "/" + wide + ".wad";

    EXPECT_EQ(ttk::Format::fit_path(path, 13), std::string(ELLIPSIS) + "/" + wide + ".wad")
            << "six bytes of name count as six columns";
}

TEST(FormatDirectoryOf, ReturnsTheParent) {
    EXPECT_EQ(ttk::Format::directory_of("/usr/share/doom.wad"), "/usr/share");
}

TEST(FormatDirectoryOf, RelativeParent) {
    EXPECT_EQ(ttk::Format::directory_of("mods/a.pk3"), "mods");
}

TEST(FormatDirectoryOf, NoParentIsEmpty) {
    EXPECT_EQ(ttk::Format::directory_of("doom.wad"), "");
    EXPECT_EQ(ttk::Format::directory_of(""), "");
}

TEST(FormatDirectoryOf, TrailingSlashYieldsItselfWithoutTheSlash) {
    EXPECT_EQ(ttk::Format::directory_of("/usr/share/"), "/usr/share");
    EXPECT_EQ(ttk::Format::directory_of("mods/"), "mods");
}

TEST(FormatDirectoryOf, FileAtTheRoot) {
    EXPECT_EQ(ttk::Format::directory_of("/doom.wad"), "/");
}

TEST(FormatDirectoryOf, KeepsUtf8) {
    EXPECT_EQ(ttk::Format::directory_of("/\xC3\xA4/\xE6\x97\xA5.wad"), "/\xC3\xA4");
}

TEST(FormatFileName, ReturnsTheLastComponent) {
    EXPECT_EQ(ttk::Format::file_name("/usr/share/doom.wad"), "doom.wad");
    EXPECT_EQ(ttk::Format::file_name("doom.wad"), "doom.wad");
}

TEST(FormatFileName, TrailingSlashIsEmpty) {
    EXPECT_EQ(ttk::Format::file_name("/usr/share/"), "");
}

TEST(FormatFileName, EmptyIsEmpty) {
    EXPECT_EQ(ttk::Format::file_name(""), "");
}

TEST(FormatFileName, KeepsDotsAndUtf8) {
    EXPECT_EQ(ttk::Format::file_name("/a/.hidden"), ".hidden");
    EXPECT_EQ(ttk::Format::file_name("/a/archive.tar.gz"), "archive.tar.gz");
    EXPECT_EQ(ttk::Format::file_name("/a/\xE6\x97\xA5\xE6\x9C\xAC"), "\xE6\x97\xA5\xE6\x9C\xAC");
}

TEST(FormatFileName, JoinsBackWithDirectoryOf) {
    const std::string path = "/usr/share/games/doom2.wad";

    EXPECT_EQ(ttk::Format::directory_of(path) + "/" + ttk::Format::file_name(path), path);
}

TEST(FormatIsFile, RegularFileIsAFile) {
    const fs::path root = tests::Sandbox::scratch("is-file");

    EXPECT_TRUE(ttk::Format::is_file(touch(root / "f.txt")));
}

TEST(FormatIsFile, DirectoryIsNotAFile) {
    EXPECT_FALSE(ttk::Format::is_file(ttk::Format::from_path(tests::Sandbox::scratch("is-file-directory"))));
}

TEST(FormatIsFile, MissingAndEmptyAreNotFiles) {
    EXPECT_FALSE(ttk::Format::is_file(ttk::Format::from_path(tests::Sandbox::scratch("is-file-missing") / "nope")));
    EXPECT_FALSE(ttk::Format::is_file(""));
}

TEST(FormatIsFile, FollowsSymbolicLinks) {
    const fs::path root = tests::Sandbox::scratch("is-file-link");

    touch(root / "real");
    fs::create_symlink(root / "real", root / "link");
    fs::create_symlink(root / "absent", root / "dangling");

    EXPECT_TRUE(ttk::Format::is_file(ttk::Format::from_path(root / "link")));
    EXPECT_FALSE(ttk::Format::is_file(ttk::Format::from_path(root / "dangling")));
}

TEST(FormatIsFile, Utf8Name) {
    const fs::path root = tests::Sandbox::scratch("is-file-utf8");

    EXPECT_TRUE(ttk::Format::is_file(touch(root / "\xE6\x97\xA5\xE6\x9C\xAC.txt")));
}

TEST(FormatIsDirectory, DirectoryIsADirectory) {
    EXPECT_TRUE(ttk::Format::is_directory(ttk::Format::from_path(tests::Sandbox::scratch("is-directory"))));
}

TEST(FormatIsDirectory, FileIsNotADirectory) {
    const fs::path root = tests::Sandbox::scratch("is-directory-file");

    EXPECT_FALSE(ttk::Format::is_directory(touch(root / "f")));
}

TEST(FormatIsDirectory, MissingAndEmptyAreNotDirectories) {
    EXPECT_FALSE(ttk::Format::is_directory(
            ttk::Format::from_path(tests::Sandbox::scratch("is-directory-missing") / "nope")));
    EXPECT_FALSE(ttk::Format::is_directory(""));
}

TEST(FormatIsDirectory, FollowsSymbolicLinks) {
    const fs::path root = tests::Sandbox::scratch("is-directory-link");

    fs::create_directory(root / "real");
    fs::create_directory_symlink(root / "real", root / "link");

    EXPECT_TRUE(ttk::Format::is_directory(ttk::Format::from_path(root / "link")));
}

TEST(FormatIsDirectory, TrailingSlashStillNamesTheDirectory) {
    EXPECT_TRUE(ttk::Format::is_directory(ttk::Format::from_path(tests::Sandbox::scratch("is-directory-slash")) + "/"));
}

TEST(FormatSameFile, EmptyNeverMatches) {
    EXPECT_FALSE(ttk::Format::same_file("", ""));
    EXPECT_FALSE(ttk::Format::same_file("", "/tmp"));
    EXPECT_FALSE(ttk::Format::same_file("/tmp", ""));
}

TEST(FormatSameFile, SamePathMatches) {
    const fs::path root = tests::Sandbox::scratch("same-file");
    const std::string file = touch(root / "f");

    EXPECT_TRUE(ttk::Format::same_file(file, file));
}

TEST(FormatSameFile, DifferentSpellingsOfOneFileMatch) {
    const fs::path root = tests::Sandbox::scratch("same-file-spelling");
    const std::string file = touch(root / "f");

    EXPECT_TRUE(ttk::Format::same_file(file, ttk::Format::from_path(root / "." / "f")));
    EXPECT_TRUE(ttk::Format::same_file(file, ttk::Format::from_path(root / ".." / root.filename() / "f")));
}

TEST(FormatSameFile, SymbolicAndHardLinksMatch) {
    const fs::path root = tests::Sandbox::scratch("same-file-links");
    const std::string file = touch(root / "f");

    fs::create_symlink(root / "f", root / "soft");
    fs::create_hard_link(root / "f", root / "hard");

    EXPECT_TRUE(ttk::Format::same_file(file, ttk::Format::from_path(root / "soft")));
    EXPECT_TRUE(ttk::Format::same_file(file, ttk::Format::from_path(root / "hard")));
}

TEST(FormatSameFile, DifferentFilesDoNotMatch) {
    const fs::path root = tests::Sandbox::scratch("same-file-different");

    EXPECT_FALSE(ttk::Format::same_file(touch(root / "a"), touch(root / "b")));
}

TEST(FormatSameFile, IdenticalMissingPathsMatch) {
    const std::string missing = ttk::Format::from_path(tests::Sandbox::scratch("same-file-missing") / "nope");

    EXPECT_TRUE(ttk::Format::same_file(missing, missing)) << "missing files compare as strings";
}

TEST(FormatSameFile, DifferentMissingPathsDoNotMatch) {
    const fs::path root = tests::Sandbox::scratch("same-file-missing-pair");

    EXPECT_FALSE(ttk::Format::same_file(ttk::Format::from_path(root / "a"), ttk::Format::from_path(root / "b")));
}

TEST(FormatSameFile, CaseMattersOutsideWindows) {
    const fs::path root = tests::Sandbox::scratch("same-file-case");

    EXPECT_FALSE(ttk::Format::same_file(ttk::Format::from_path(root / "Name"), ttk::Format::from_path(root / "name")));
}

TEST(FormatBytes, ZeroBytes) {
    EXPECT_EQ(ttk::Format::bytes(0), "0 B");
}

TEST(FormatBytes, OneByte) {
    EXPECT_EQ(ttk::Format::bytes(1), "1 B");
}

TEST(FormatBytes, BytesShowNoDecimals) {
    EXPECT_EQ(ttk::Format::bytes(512), "512 B");
    EXPECT_EQ(ttk::Format::bytes(KB - 1), "1023 B");
}

TEST(FormatBytes, StepsToKilobytesAt1024) {
    EXPECT_EQ(ttk::Format::bytes(KB), "1.0 KB");
    EXPECT_EQ(ttk::Format::bytes(KB + (KB / 2)), "1.5 KB");
}

TEST(FormatBytes, StepsToMegabytes) {
    EXPECT_EQ(ttk::Format::bytes(MB), "1.0 MB");
    EXPECT_EQ(ttk::Format::bytes(13002342), "12.4 MB");
}

TEST(FormatBytes, StepsToGigabytes) {
    EXPECT_EQ(ttk::Format::bytes(GB), "1.0 GB");
    EXPECT_EQ(ttk::Format::bytes(5 * GB / 2), "2.5 GB");
}

TEST(FormatBytes, StopsAtGigabytes) {
    EXPECT_EQ(ttk::Format::bytes(GB * KB), "1024.0 GB");
}

TEST(FormatBytes, LargestValue) {
    EXPECT_EQ(ttk::Format::bytes(ULLONG_MAX), "17179869184.0 GB");
}

TEST(FormatBytes, RoundsToOneDecimal) {
    EXPECT_EQ(ttk::Format::bytes(1126), "1.1 KB");
    EXPECT_EQ(ttk::Format::bytes(2047), "2.0 KB");
}
