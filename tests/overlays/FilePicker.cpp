// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Tree.h"
#include "support/Sandbox.h"
#include "ttk/dialogs/FilePicker.h"
#include "ttk/notices/Notifier.h"
#include "ttk/system/Paths.h"

namespace {
    using overlays::marked;
    using overlays::names;
    using Names = std::vector<std::string>;

    Names texts() {
        return {"alpha", "Beta", "dir2", "dir10", "A.TXT", "b.txt", "file2.txt", "file10.txt"};
    }

    std::string home() {
        return ttk::Paths::home_directory().generic_string();
    }

    class Picker : public ::testing::Test {
    protected:
        ttk::Notifier notifier;
        ttk::FilePicker picker{&notifier};
        std::string root;
        std::vector<Names> chosen;
        std::vector<bool> options;
        int changes = 0;

        void SetUp() override {
            root = overlays::tree("picker");
            picker.changed = [this] { ++changes; };
            picker.remember_directory("here", root);
        }

        ttk::FilePicker::Chosen record() {
            return [this](const Names &paths, const bool option) {
                chosen.push_back(paths);
                options.push_back(option);
            };
        }

        void open(const Names &filters = {"*.txt"}, const bool directories = false, const bool folders = false,
                  const bool multiple = false, const std::string &option = {}) {
            picker.open("Pick", filters, directories, folders, multiple, "here", record(), option, "A hint");
        }

        void save(const std::string &name = "report", const Names &filters = {"*.txt"}) {
            picker.open_save("Save", filters, "here", name, record());
        }

        [[nodiscard]] std::string at(const std::string &name) const { return root + "/" + name; }

        [[nodiscard]] std::string warned() const {
            return notifier.messages().empty() ? std::string() : notifier.messages().back().body;
        }

        [[nodiscard]] const ttk::FilePicker::State &state() const { return picker.state(); }
    };

    TEST(PickerStart, FallsBackToTheSandboxedHome) {
        ttk::Notifier notifier;
        const ttk::FilePicker picker(&notifier);
        const std::string start = picker.start_directory("never remembered");

        EXPECT_EQ(start, home());
        EXPECT_TRUE(start.starts_with(tests::Sandbox::root().generic_string())) << "the picker reached the real home";
    }

    TEST_F(Picker, StartsInTheRememberedDirectory) {
        EXPECT_EQ(picker.start_directory("here"), root);
    }

    TEST_F(Picker, RememberedDirectoryThatIsGoneFallsBackToHome) {
        picker.remember_directory("gone", at("missing"));

        EXPECT_EQ(picker.start_directory("gone"), home());
    }

    TEST_F(Picker, RememberingAnEmptyPathKeepsTheOldOne) {
        picker.remember_directory("here", "");

        EXPECT_EQ(picker.start_directory("here"), root);
    }

    TEST_F(Picker, KeysAreRememberedApart) {
        picker.remember_directory("other", at("alpha"));

        EXPECT_EQ(picker.start_directory("here"), root);
        EXPECT_EQ(picker.start_directory("other"), at("alpha"));
    }

    TEST_F(Picker, MemoryHooksReplaceTheOwnStore) {
        std::map<std::string, std::string> kept{{"here", at("alpha")}};

        picker.set_memory(ttk::FilePicker::Memory{
                .directory = [&kept](const std::string &key) { return kept[key]; },
                .remember = [&kept](const std::string &key, const std::string &path) { kept[key] = path; },
                .hidden = {},
                .showHidden = {},
        });

        EXPECT_EQ(picker.start_directory("here"), at("alpha")) << "the hook was not asked";

        picker.remember_directory("next", at("Beta"));

        EXPECT_EQ(kept["next"], at("Beta"));
    }

    TEST_F(Picker, OpenFillsTheState) {
        open({"*.txt"}, false, true, true, "Recurse");

        EXPECT_TRUE(state().open);
        EXPECT_EQ(state().title, "Pick");
        EXPECT_FALSE(state().directories);
        EXPECT_TRUE(state().folders);
        EXPECT_TRUE(state().multiple);
        EXPECT_EQ(state().option, "Recurse");
        EXPECT_EQ(state().optionHint, "A hint");
        EXPECT_FALSE(state().optionSet);
        EXPECT_FALSE(state().saving);
        EXPECT_FALSE(state().editing);
        EXPECT_FALSE(state().drives);
        EXPECT_FALSE(state().hiddenShown);
        EXPECT_TRUE(state().rooted);
        EXPECT_EQ(state().path, root);
        EXPECT_EQ(state().marked, 0);
        EXPECT_GT(changes, 0);
    }

    TEST_F(Picker, PartsAreTheNonEmptyComponentsOfThePath) {
        open();

        std::string joined;

        for (const std::string &part : state().parts) {
            EXPECT_FALSE(part.empty());

            joined += "/" + part;
        }

        EXPECT_EQ(joined, root);
        EXPECT_EQ(state().parts.back(), "picker");
    }

    TEST_F(Picker, OpenClearsTheOptionTick) {
        open();
        picker.state().optionSet = true;
        picker.dismiss();
        open();

        EXPECT_FALSE(state().optionSet);
    }

    TEST_F(Picker, ListsFoldersFirstThenMatchingFilesInNaturalOrder) {
        open();

        EXPECT_EQ(names(picker), texts());
    }

    TEST_F(Picker, RowsCarryTheirPathAndKind) {
        open();

        const ttk::FilePicker::Row &folder = state().entries.front();
        const ttk::FilePicker::Row &file = state().entries.back();

        EXPECT_EQ(folder.path, at("alpha"));
        EXPECT_TRUE(folder.directory);
        EXPECT_FALSE(folder.hidden);
        EXPECT_FALSE(folder.marked);
        EXPECT_EQ(file.path, at("file10.txt"));
        EXPECT_FALSE(file.directory);
    }

    TEST_F(Picker, NoFiltersListsEveryFile) {
        open({});

        EXPECT_EQ(names(picker), (Names{"alpha", "Beta", "dir2", "dir10", "A.TXT", "b.txt", "file2.txt", "file10.txt",
                                        "image.png", "notes.md"}));
    }

    TEST_F(Picker, StarDotStarListsEveryFile) {
        open({"*.*"});

        EXPECT_EQ(names(picker).size(), 10U);
    }

    TEST_F(Picker, FiltersIgnoreCase) {
        open({"*.TXT"});

        EXPECT_EQ(names(picker), texts());
    }

    TEST_F(Picker, QuestionMarkMatchesOneCharacter) {
        open({"?.txt"});

        EXPECT_EQ(names(picker), (Names{"alpha", "Beta", "dir2", "dir10", "A.TXT", "b.txt"}));
    }

    TEST_F(Picker, StarMatchesAnyRunAnywhere) {
        open({"f*1*"});

        EXPECT_EQ(names(picker), (Names{"alpha", "Beta", "dir2", "dir10", "file10.txt"}));
    }

    TEST_F(Picker, AnyFilterOfSeveralLetsAFileIn) {
        open({"*.png", "*.md"});

        EXPECT_EQ(names(picker), (Names{"alpha", "Beta", "dir2", "dir10", "image.png", "notes.md"}));
    }

    TEST_F(Picker, HiddenEntriesAreLeftOutByDefault) {
        open({});

        for (const std::string &name : names(picker)) {
            EXPECT_FALSE(name.starts_with('.')) << name;
        }
    }

    TEST_F(Picker, ShowHiddenListsThemFlaggedAsHidden) {
        open();
        picker.show_hidden(true);

        EXPECT_TRUE(state().hiddenShown);
        EXPECT_EQ(names(picker), (Names{".hidden_dir", "alpha", "Beta", "dir2", "dir10", ".secret.txt", "A.TXT",
                                        "b.txt", "file2.txt", "file10.txt"}));
        EXPECT_TRUE(state().entries.front().hidden);
        EXPECT_FALSE(state().entries[1].hidden);

        picker.show_hidden(false);

        EXPECT_EQ(names(picker), texts());
    }

    TEST_F(Picker, ShowHiddenCarriesOverToTheNextOpen) {
        open();
        picker.show_hidden(true);
        picker.dismiss();
        open();

        EXPECT_TRUE(state().hiddenShown);
        EXPECT_EQ(names(picker).front(), ".hidden_dir");
    }

    TEST_F(Picker, ShowHiddenGoesThroughTheMemoryHooks) {
        bool shown = true;
        std::vector<bool> stored;

        picker.set_memory(ttk::FilePicker::Memory{
                .directory = {},
                .remember = {},
                .hidden = [&shown] { return shown; },
                .showHidden = [&stored](const bool value) { stored.push_back(value); },
        });
        open();

        EXPECT_TRUE(state().hiddenShown) << "the hidden hook was not asked on open";
        EXPECT_EQ(names(picker).front(), ".hidden_dir");

        picker.show_hidden(false);

        EXPECT_EQ(stored, (std::vector<bool>{false}));
    }

    TEST_F(Picker, DirectoryPickLeavesFilesOut) {
        open({"*.txt"}, true);

        EXPECT_TRUE(state().directories);
        EXPECT_EQ(names(picker), (Names{"alpha", "Beta", "dir2", "dir10"}));
    }

    TEST_F(Picker, EmptyDirectoryPickSaysSo) {
        open({}, true);
        picker.go(at("Beta"));

        EXPECT_TRUE(state().entries.empty());
        EXPECT_EQ(state().nothing, "No directories here");
    }

    TEST_F(Picker, EmptyListingNamesTheFilters) {
        open({"*.png", "*.md"});
        picker.go(at("Beta"));

        EXPECT_TRUE(state().entries.empty());
        EXPECT_EQ(state().nothing, "Nothing here matches *.png, *.md");
    }

    TEST_F(Picker, EmptyFilterListIsShownAsAStar) {
        open({});
        picker.go(at("Beta"));

        EXPECT_EQ(state().nothing, "Nothing here matches *");
    }

    TEST_F(Picker, GoListsTheDirectory) {
        open();
        picker.go(at("alpha"));

        EXPECT_EQ(state().path, at("alpha"));
        EXPECT_EQ(state().parts.back(), "alpha");
        EXPECT_EQ(names(picker), (Names{"deeper", "inner.txt"}));
    }

    TEST_F(Picker, GoToAMissingDirectoryListsNothing) {
        open();
        picker.go(at("missing"));

        EXPECT_EQ(state().path, at("missing"));
        EXPECT_TRUE(state().entries.empty());
    }

    TEST_F(Picker, GoToAFileListsNothing) {
        open();
        picker.go(at("b.txt"));

        EXPECT_TRUE(state().entries.empty());
    }

    TEST_F(Picker, UpListsTheParent) {
        open();
        picker.go(at("alpha/deeper"));
        picker.up();

        EXPECT_EQ(state().path, at("alpha"));

        picker.up();

        EXPECT_EQ(state().path, root);
        EXPECT_EQ(names(picker), texts());
    }

    TEST_F(Picker, UpToListsTheAncestorAtThatIndex) {
        open();
        picker.go(at("alpha/deeper"));

        const auto picked = static_cast<int>(state().parts.size()) - 3;

        picker.up_to(picked);

        EXPECT_EQ(state().path, root);

        picker.up_to(0);

        EXPECT_EQ(state().path, "/" + state().parts.front());
        EXPECT_EQ(state().parts.size(), 1U);
    }

    TEST_F(Picker, UpToTheLastPartStaysPut) {
        open();
        picker.go(at("alpha"));
        picker.up_to(static_cast<int>(state().parts.size()) - 1);

        EXPECT_EQ(state().path, at("alpha"));
    }

    TEST_F(Picker, DriveListIsEmptyOutsideWindows) {
        open();
        picker.show_drives();

        EXPECT_TRUE(state().drives);
        EXPECT_EQ(state().path, root) << "the drive list moved the path";
        EXPECT_EQ(state().nothing, "No drives found");
#ifndef _WIN32
        EXPECT_TRUE(state().entries.empty());
#endif
    }

    TEST_F(Picker, GoingSomewhereLeavesTheDriveList) {
        open();
        picker.show_drives();
        picker.go(at("alpha"));

        EXPECT_FALSE(state().drives);
        EXPECT_EQ(names(picker), (Names{"deeper", "inner.txt"}));
    }

    TEST_F(Picker, ShowHiddenOnTheDriveListKeepsIt) {
        open();
        picker.show_drives();
        picker.show_hidden(true);

        EXPECT_TRUE(state().drives);
        EXPECT_TRUE(state().hiddenShown);
    }

    TEST_F(Picker, MarkingAFileMarksItsRow) {
        open();
        picker.mark(at("b.txt"));

        EXPECT_EQ(marked(picker), (Names{"b.txt"}));
        EXPECT_EQ(state().marked, 1);
        EXPECT_EQ(state().markedFolders, 0);
    }

    TEST_F(Picker, SingleMarkMovesToTheNextFile) {
        open();
        picker.mark(at("b.txt"));
        picker.mark(at("A.TXT"));

        EXPECT_EQ(marked(picker), (Names{"A.TXT"}));
        EXPECT_EQ(state().marked, 1);
    }

    TEST_F(Picker, MarkingAgainClearsTheMark) {
        open();
        picker.mark(at("b.txt"));
        picker.mark(at("b.txt"));

        EXPECT_TRUE(marked(picker).empty());
        EXPECT_EQ(state().marked, 0);
    }

    TEST_F(Picker, MultipleMarksToggleOnTheirOwn) {
        open({"*.txt"}, false, false, true);
        picker.mark(at("b.txt"));
        picker.mark(at("A.TXT"));
        picker.mark(at("file2.txt"));
        picker.mark(at("b.txt"));

        EXPECT_EQ(marked(picker), (Names{"A.TXT", "file2.txt"}));
        EXPECT_EQ(state().marked, 2);
    }

    TEST_F(Picker, MarkedFoldersAreCounted) {
        open({"*.txt"}, false, true, true);
        picker.mark(at("alpha"));
        picker.mark(at("b.txt"));

        EXPECT_EQ(state().marked, 2);
        EXPECT_EQ(state().markedFolders, 1);
    }

    TEST_F(Picker, MarkIsDroppedOnNavigationInASinglePick) {
        open();
        picker.mark(at("b.txt"));
        picker.go(at("alpha"));
        picker.up();

        EXPECT_TRUE(marked(picker).empty());
        EXPECT_EQ(state().marked, 0);
    }

    TEST_F(Picker, MarksSurviveNavigationInAMultiplePick) {
        open({"*.txt"}, false, false, true);
        picker.mark(at("b.txt"));
        picker.go(at("alpha"));
        picker.mark(at("alpha/inner.txt"));

        EXPECT_EQ(state().marked, 2) << "a mark made in another directory was lost";
        EXPECT_EQ(marked(picker), (Names{"inner.txt"}));

        picker.up();

        EXPECT_EQ(marked(picker), (Names{"b.txt"}));
    }

    TEST_F(Picker, ChooseCallsBackWithThePathsAndTheOption) {
        open({"*.txt"}, false, false, false, "Recurse");
        picker.state().optionSet = true;
        picker.touch();
        picker.choose({at("b.txt")});

        ASSERT_EQ(chosen.size(), 1U);
        EXPECT_EQ(chosen[0], (Names{at("b.txt")}));
        EXPECT_EQ(options, (std::vector<bool>{true}));
        EXPECT_FALSE(state().open);
    }

    TEST_F(Picker, ChooseRemembersTheDirectoryForTheNextOpen) {
        open();
        picker.go(at("alpha"));
        picker.choose({at("alpha/inner.txt")});

        EXPECT_EQ(picker.start_directory("here"), at("alpha"));

        open();

        EXPECT_EQ(state().path, at("alpha"));
    }

    TEST_F(Picker, ChoosingNothingClosesWithoutCallingBack) {
        open();
        picker.choose({});

        EXPECT_TRUE(chosen.empty());
        EXPECT_FALSE(state().open);
    }

    TEST_F(Picker, ChooseMarkedPassesTheMarksInTheOrderMade) {
        open({"*.txt"}, false, false, true);
        picker.mark(at("file2.txt"));
        picker.mark(at("A.TXT"));
        picker.choose_marked();

        ASSERT_EQ(chosen.size(), 1U);
        EXPECT_EQ(chosen[0], (Names{at("file2.txt"), at("A.TXT")}));
    }

    TEST_F(Picker, ChooseMarkedWithNoMarksCallsNothing) {
        open();
        picker.choose_marked();

        EXPECT_TRUE(chosen.empty());
    }

    TEST_F(Picker, CallbackRunsOnce) {
        open();
        picker.choose({at("b.txt")});
        picker.choose({at("A.TXT")});

        EXPECT_EQ(chosen.size(), 2U) << "choose is not one-shot by itself";

        chosen.clear();
        open();
        picker.mark(at("b.txt"));
        picker.dismiss();
        picker.choose_marked();

        EXPECT_TRUE(chosen.empty()) << "dismiss left the mark to be chosen";
    }

    TEST_F(Picker, DismissClosesWithoutCallingBackAndClearsMarks) {
        open({"*.txt"}, false, false, true);
        picker.mark(at("b.txt"));
        picker.dismiss();

        EXPECT_FALSE(state().open);
        EXPECT_TRUE(chosen.empty());

        picker.choose_marked();

        EXPECT_TRUE(chosen.empty()) << "the mark outlived the dismissal";
    }

    TEST_F(Picker, DismissLeavesTheStateUnmarked) {
        open({"*.txt"}, false, true, true);
        picker.mark(at("b.txt"));
        picker.mark(at("alpha"));
        picker.dismiss();

        EXPECT_TRUE(marked(picker).empty()) << "a row still reads as marked";
        EXPECT_EQ(state().marked, 0);
        EXPECT_EQ(state().markedFolders, 0);
        EXPECT_FALSE(state().multiple);
    }

    TEST_F(Picker, DismissWhileTypingOnlyEndsTheTyping) {
        open();
        picker.state().editing = true;
        picker.dismiss();

        EXPECT_FALSE(state().editing);
        EXPECT_TRUE(state().open);

        picker.dismiss();

        EXPECT_FALSE(state().open);
    }

    TEST_F(Picker, EveryChangeCallsChanged) {
        open();

        const int before = changes;

        picker.mark(at("b.txt"));

        EXPECT_GT(changes, before);

        const int marked = changes;

        picker.touch();

        EXPECT_EQ(changes, marked + 1);
    }

    TEST(PickerTouch, WithoutAChangedCallbackIsSafe) {
        ttk::Notifier notifier;
        const ttk::FilePicker picker(&notifier);

        picker.touch();

        SUCCEED();
    }

    TEST_F(Picker, TypedDirectoryIsListed) {
        open();
        picker.state().editing = true;
        picker.typed(at("alpha"));

        EXPECT_EQ(state().path, at("alpha"));
        EXPECT_FALSE(state().editing);
    }

    TEST_F(Picker, TypedPathIsTrimmed) {
        open();
        picker.typed("  " + at("alpha") + " \t");

        EXPECT_EQ(state().path, at("alpha"));
    }

    TEST_F(Picker, TypedTrailingSlashIsDropped) {
        open();
        picker.typed(at("alpha") + "/");

        EXPECT_EQ(state().path, at("alpha"));
    }

    TEST_F(Picker, TypedDotsAreResolved) {
        open();
        picker.typed(at("alpha/deeper/../.."));

        EXPECT_EQ(state().path, root);
    }

    TEST_F(Picker, TypedTildeIsHome) {
        open();
        picker.typed("~");

        EXPECT_EQ(state().path, std::filesystem::canonical(home()).generic_string());
    }

    TEST_F(Picker, TypedRelativePathStartsAtTheWorkingDirectory) {
        const std::filesystem::path was = std::filesystem::current_path();

        std::filesystem::current_path(root);
        open();
        picker.typed("alpha/deeper");
        std::filesystem::current_path(was);

        EXPECT_EQ(state().path, at("alpha/deeper"));
    }

    TEST_F(Picker, TypedFileIsChosen) {
        open();
        picker.typed(at("b.txt"));

        ASSERT_EQ(chosen.size(), 1U);
        EXPECT_EQ(chosen[0], (Names{at("b.txt")}));
        EXPECT_FALSE(state().open);
    }

    TEST_F(Picker, TypedMissingPathIsWarnedAbout) {
        open();
        picker.typed(at("missing"));

        ASSERT_FALSE(notifier.messages().empty());
        EXPECT_EQ(notifier.messages().back().severity, ttk::Severity::Warning);
        EXPECT_EQ(warned(), at("missing") + " is not there.");
        EXPECT_EQ(state().path, root);
        EXPECT_TRUE(chosen.empty());
    }

    TEST_F(Picker, TypedFileWhilePickingADirectoryIsWarnedAbout) {
        open({}, true);
        picker.typed(at("b.txt"));

        EXPECT_TRUE(chosen.empty());
        EXPECT_EQ(warned(), at("b.txt") + " is not there.");
    }

    TEST_F(Picker, TypedNothingOnlyEndsTheTyping) {
        open();
        picker.state().editing = true;
        picker.typed("   ");

        EXPECT_FALSE(state().editing);
        EXPECT_EQ(state().path, root);
        EXPECT_TRUE(notifier.messages().empty());
    }

    TEST_F(Picker, OpenSaveOffersTheNameAndItsTarget) {
        const int seed = state().nameSeed;

        save("report");

        EXPECT_TRUE(state().open);
        EXPECT_TRUE(state().saving);
        EXPECT_FALSE(state().directories);
        EXPECT_FALSE(state().multiple);
        EXPECT_EQ(state().title, "Save");
        EXPECT_EQ(state().name, "report");
        EXPECT_GT(state().nameSeed, seed);
        EXPECT_EQ(state().target, at("report.txt"));
        EXPECT_FALSE(state().replacing);
    }

    TEST_F(Picker, OfferingTheSameNameAgainBumpsTheSeed) {
        save("report");

        const int first = state().nameSeed;

        save("report");

        EXPECT_GT(state().nameSeed, first);
    }

    TEST_F(Picker, OpenAfterSavingIsNoLongerSaving) {
        save();
        picker.dismiss();
        open();

        EXPECT_FALSE(state().saving);
    }

    TEST_F(Picker, NameWithAFilterSuffixKeepsIt) {
        save();
        picker.named("REPORT.TXT");

        EXPECT_EQ(state().target, at("REPORT.TXT"));
    }

    TEST_F(Picker, FirstFilterSuffixIsAddedToABareName) {
        save("x", {"*.md", "*.txt"});

        EXPECT_EQ(state().target, at("x.md"));

        picker.named("x.txt");

        EXPECT_EQ(state().target, at("x.txt"));
    }

    TEST_F(Picker, NameWithoutFiltersGetsNoSuffix) {
        save("plain", {});

        EXPECT_EQ(state().target, at("plain"));
    }

    TEST_F(Picker, NameIsTidiedOfTrailingDotsAndSpaces) {
        save();
        picker.named("  notes. . ");

        EXPECT_EQ(state().target, at("notes.txt"));
    }

    TEST_F(Picker, NameKeepsOnlyItsLastComponent) {
        save();
        picker.named("elsewhere/x");

        EXPECT_EQ(state().target, at("x.txt"));
    }

    TEST_F(Picker, EmptyNameHasNoTarget) {
        save("");

        EXPECT_TRUE(state().target.empty());
        EXPECT_FALSE(state().replacing);
    }

    TEST_F(Picker, ExistingFileIsMarkedAsReplacing) {
        save();
        picker.named("b");

        EXPECT_EQ(state().target, at("b.txt"));
        EXPECT_TRUE(state().replacing);

        picker.named("alpha");

        EXPECT_FALSE(state().replacing) << "a name that would land on a folder is no replace";
    }

    TEST_F(Picker, TargetFollowsTheDirectory) {
        save("report");
        picker.go(at("alpha"));

        EXPECT_EQ(state().target, at("alpha/report.txt"));
    }

    TEST_F(Picker, SavingWithoutANameWarns) {
        save("");
        picker.save("  ", true);

        EXPECT_EQ(warned(), "Give the file a name first.");
        EXPECT_TRUE(chosen.empty());
        EXPECT_TRUE(state().open);
    }

    TEST_F(Picker, SavingOverAFileNeedsReplacing) {
        save();
        picker.save("b", false);

        EXPECT_EQ(warned(), "b.txt is already there. Use Replace to write over it.");
        EXPECT_TRUE(chosen.empty());

        picker.save("b", true);

        ASSERT_EQ(chosen.size(), 1U);
        EXPECT_EQ(chosen[0], (Names{at("b.txt")}));
    }

    TEST_F(Picker, SavingANewFileChoosesItsTarget) {
        save();
        picker.save("fresh", false);

        ASSERT_EQ(chosen.size(), 1U);
        EXPECT_EQ(chosen[0], (Names{at("fresh.txt")}));
        EXPECT_FALSE(state().open);
        EXPECT_TRUE(notifier.messages().empty());
    }

    TEST_F(Picker, TypedFileWhileSavingOffersItsName) {
        save();

        const int seed = state().nameSeed;

        picker.typed(at("alpha/inner.txt"));

        EXPECT_TRUE(chosen.empty());
        EXPECT_EQ(state().path, at("alpha"));
        EXPECT_EQ(state().name, "inner.txt");
        EXPECT_GT(state().nameSeed, seed);
        EXPECT_TRUE(state().replacing);
    }
}
