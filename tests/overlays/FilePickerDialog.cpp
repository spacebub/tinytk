// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Overlays.h"
#include "Tree.h"
#include "support/Canvas.h"
#include "ttk/dialogs/DialogLayer.h"
#include "ttk/dialogs/FilePicker.h"
#include "ttk/dialogs/FilePickerDialog.h"
#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/notices/Notifier.h"
#include "ttk/toolkit/controls/Button.h"
#include "ttk/toolkit/controls/GlyphButton.h"
#include "ttk/toolkit/controls/TextBox.h"
#include "ttk/toolkit/controls/Toggle.h"
#include "ttk/toolkit/layout/Scroll.h"

namespace {
    using overlays::marked;
    using overlays::names;
    using overlays::Taker;
    using Names = std::vector<std::string>;

    constexpr double ROW = 34.0;
    constexpr double EDGE = 44.0;
    constexpr double CARD_W = 700.0;
    constexpr double CARD_H = 540.0;

    class Browsing : public overlays::Painted {
    protected:
        tests::Canvas canvas{800, 700};
        ttk::Notifier notifier;
        ttk::FilePicker picker{&notifier};
        ttk::DialogLayer *layer = nullptr;
        ttk::FilePickerDialog *dialog = nullptr;
        std::string root;
        std::vector<Names> chosen;
        std::vector<bool> options;
        bool pending = false;

        ttk::Scroll *rows = nullptr;
        ttk::Button *use = nullptr;
        ttk::GlyphButton *shut = nullptr;
        ttk::GlyphButton *up = nullptr;
        ttk::GlyphButton *typer = nullptr;
        ttk::TextBox *typed = nullptr;
        ttk::TextBox *named = nullptr;
        ttk::Toggle *hidden = nullptr;
        ttk::Toggle *option = nullptr;

        void SetUp() override {
            overlays::Painted::SetUp();

            if (IsSkipped()) {
                return;
            }

            root = overlays::tree("picker_dialog", padding());
            layer = overlays::dialog_layer(canvas);
            picker.remember_directory("here", root);
            picker.changed = [this] { pending = true; };
            layer->closed = [this](const ttk::Dialog *gone) {
                if (gone == dialog) {
                    dialog = nullptr;
                    picker.dismiss();
                }
            };
        }

        virtual int padding() { return 0; }

        // Deferred as the gallery posts it, since a picker change can come from inside the dialog it takes down.
        void pump() {
            while (pending) {
                pending = false;

                const bool wants = picker.state().open;

                if (wants && dialog == nullptr) {
                    dialog = static_cast<ttk::FilePickerDialog *>(
                            layer->show(std::make_unique<ttk::FilePickerDialog>(picker)));
                    find();
                } else if (!wants && dialog != nullptr) {
                    layer->dismiss();
                }

                if (dialog != nullptr) {
                    dialog->sync();
                }
            }

            canvas.frame();
            canvas.frame_at(canvas.root().now() + 1.0);
        }

        void find() {
            const std::vector<ttk::Scroll *> lists = overlays::all<ttk::Scroll>(dialog);
            const std::vector<ttk::Button *> buttons = overlays::all<ttk::Button>(dialog);
            const std::vector<ttk::GlyphButton *> glyphs = overlays::all<ttk::GlyphButton>(dialog);
            const std::vector<ttk::TextBox *> boxes = overlays::all<ttk::TextBox>(dialog);
            const std::vector<ttk::Toggle *> toggles = overlays::all<ttk::Toggle>(dialog);

            ASSERT_EQ(lists.size(), 1U);
            ASSERT_EQ(buttons.size(), 1U);
            ASSERT_EQ(glyphs.size(), 3U);
            ASSERT_EQ(boxes.size(), 2U);
            ASSERT_EQ(toggles.size(), 2U);

            rows = lists[0];
            use = buttons[0];
            shut = glyphs[0];
            up = glyphs[1];
            typer = glyphs[2];
            typed = boxes[0];
            named = boxes[1];
            hidden = toggles[0];
            option = toggles[1];
        }

        ttk::FilePicker::Chosen record() {
            return [this](const Names &paths, const bool ticked) {
                chosen.push_back(paths);
                options.push_back(ticked);
            };
        }

        void open(const bool directories = false, const bool folders = false, const bool multiple = false,
                  const std::string &label = {}) {
            picker.open("Pick a file", {"*.txt"}, directories, folders, multiple, "here", record(), label, "A hint");
            pump();
        }

        void save(const std::string &name) {
            picker.open_save("Save as", {"*.txt"}, "here", name, record());
            pump();
        }

        [[nodiscard]] std::string at(const std::string &name) const { return root + "/" + name; }

        [[nodiscard]] double row_y(const std::size_t index) const {
            return rows->box().y + (static_cast<double>(index) * ROW) + (ROW / 2.0) - rows->offset();
        }

        void click_row(const std::size_t index, const double x = -1.0) {
            overlays::click(canvas.root(), x < 0.0 ? rows->box().x + 120.0 : x, row_y(index));
            pump();
        }

        void click(const ttk::Widget *widget) {
            overlays::click(canvas.root(), widget);
            pump();
        }

        bool key(const int code) {
            const bool taken = canvas.root().key(ttk::Key{.code = code});

            pump();

            return taken;
        }

        [[nodiscard]] std::size_t row_of(const std::string &name) const {
            const Names listed = names(picker);

            return static_cast<std::size_t>(std::ranges::find(listed, name) - listed.begin());
        }

        [[nodiscard]] std::vector<BLRect> crumbs() {
            ttk::Typeface &type = canvas.root().type();
            const BLFont &face = type.at(ttk::Typeface::mono, ttk::Theme::fontSmall);
            const BLRect card = dialog->card()->box();
            const BLRect where{card.x + 18.0, card.y + 56.0, card.w - 36.0, ttk::Theme::control};
            const BLRect strip{where.x + 38.0, where.y, where.w - 76.0, where.h};
            const double slash = type.width(face, "/");
            double total = type.width(face, "/") + 14.0;

            for (const std::string &part : picker.state().parts) {
                total += slash + type.width(face, part) + 16.0;
            }

            double x = strip.x - std::max(0.0, total - strip.w);
            std::vector<BLRect> out{BLRect{x, strip.y, type.width(face, "/") + 14.0, strip.h}};

            x += out.back().w;

            for (const std::string &part : picker.state().parts) {
                x += slash + 1.0;
                out.emplace_back(x, strip.y, type.width(face, part) + 14.0, strip.h);
                x += out.back().w;
            }

            return out;
        }
    };

    class LongBrowsing : public Browsing {
    protected:
        int padding() override { return 30; }
    };

    TEST_F(Browsing, OpensOverThePageWhenThePickerDoes) {
        ASSERT_NO_FATAL_FAILURE(open());

        ASSERT_NE(dialog, nullptr);
        EXPECT_EQ(layer->top(), dialog);
        EXPECT_DOUBLE_EQ(dialog->card()->box().w, CARD_W);
        EXPECT_DOUBLE_EQ(dialog->card()->box().h, CARD_H);
    }

    TEST_F(Browsing, ListIsSizedToItsRows) {
        ASSERT_NO_FATAL_FAILURE(open());

        EXPECT_DOUBLE_EQ(rows->reach(), static_cast<double>(names(picker).size()) * ROW);
        EXPECT_FALSE(rows->scrollable());
    }

    TEST_F(Browsing, ClickOnAFolderRowEntersIt) {
        ASSERT_NO_FATAL_FAILURE(open());

        click_row(row_of("alpha"));

        EXPECT_EQ(picker.state().path, at("alpha"));
        EXPECT_EQ(names(picker), (Names{"deeper", "inner.txt"}));
    }

    TEST_F(Browsing, ClickOnAFileRowMarksIt) {
        ASSERT_NO_FATAL_FAILURE(open());

        click_row(row_of("b.txt"));

        EXPECT_EQ(marked(picker), (Names{"b.txt"}));
        EXPECT_EQ(picker.state().path, root);
    }

    TEST_F(Browsing, ClickBelowTheLastRowDoesNothing) {
        ASSERT_NO_FATAL_FAILURE(open());

        click_row(names(picker).size() + 1);

        EXPECT_TRUE(marked(picker).empty());
        EXPECT_EQ(picker.state().path, root);
    }

    TEST_F(Browsing, ClickOnAMarkedFileUnmarksIt) {
        ASSERT_NO_FATAL_FAILURE(open());

        click_row(row_of("b.txt"));
        click_row(row_of("b.txt"));

        EXPECT_TRUE(marked(picker).empty());
    }

    TEST_F(Browsing, UseIsDisabledUntilAFileIsMarked) {
        ASSERT_NO_FATAL_FAILURE(open());

        EXPECT_FALSE(use->enabled());

        click_row(row_of("b.txt"));

        EXPECT_TRUE(use->enabled());
    }

    TEST_F(Browsing, UseChoosesTheMarkedFileAndCloses) {
        ASSERT_NO_FATAL_FAILURE(open());

        click_row(row_of("b.txt"));
        click(use);

        EXPECT_EQ(chosen, (std::vector<Names>{{at("b.txt")}}));
        EXPECT_EQ(dialog, nullptr);
        EXPECT_FALSE(layer->covered());
    }

    TEST_F(Browsing, MultiplePickChoosesEveryMark) {
        ASSERT_NO_FATAL_FAILURE(open(false, false, true));

        click_row(row_of("b.txt"));
        click_row(row_of("file2.txt"));
        click(use);

        EXPECT_EQ(chosen, (std::vector<Names>{{at("b.txt"), at("file2.txt")}}));
    }

    TEST_F(Browsing, FolderEdgeMarksWhenFoldersCanBeTaken) {
        ASSERT_NO_FATAL_FAILURE(open(false, true, true));

        click_row(row_of("alpha"), rows->box().x + rows->box().w - (EDGE / 2.0));

        EXPECT_EQ(picker.state().path, root) << "the edge of a folder row entered it";
        EXPECT_EQ(marked(picker), (Names{"alpha"}));
        EXPECT_EQ(picker.state().markedFolders, 1);
    }

    TEST_F(Browsing, FolderBodyEntersEvenWhenFoldersCanBeTaken) {
        ASSERT_NO_FATAL_FAILURE(open(false, true, true));

        click_row(row_of("alpha"));

        EXPECT_EQ(picker.state().path, at("alpha"));
    }

    TEST_F(Browsing, FolderEdgeEntersWhenFoldersCannotBeTaken) {
        ASSERT_NO_FATAL_FAILURE(open());

        click_row(row_of("alpha"), rows->box().x + rows->box().w - (EDGE / 2.0));

        EXPECT_EQ(picker.state().path, at("alpha"));
    }

    TEST_F(Browsing, DirectoryPickUsesTheDirectoryShown) {
        ASSERT_NO_FATAL_FAILURE(open(true));

        EXPECT_TRUE(use->enabled());

        click_row(row_of("alpha"));
        click(use);

        EXPECT_EQ(chosen, (std::vector<Names>{{at("alpha")}}));
        EXPECT_EQ(picker.start_directory("here"), at("alpha"));
    }

    TEST_F(Browsing, NeighbouringMarksLeaveAGapBetweenThem) {
        ASSERT_NO_FATAL_FAILURE(open(false, false, true));

        const std::size_t first = row_of("b.txt");
        const int x = static_cast<int>(rows->box().x + 300.0);
        const int seam = static_cast<int>(rows->box().y + (static_cast<double>(first + 1) * ROW));
        const int middle = static_cast<int>(row_y(first));
        const std::uint32_t above = canvas.at(x, seam - 1);
        const std::uint32_t below = canvas.at(x, seam);
        const std::uint32_t plain = canvas.at(x, middle);

        click_row(first);
        click_row(first + 1);

        ASSERT_EQ(marked(picker), (Names{"b.txt", "file2.txt"}));
        EXPECT_NE(canvas.at(x, middle), plain) << "a marked row is not highlighted";
        EXPECT_EQ(canvas.at(x, middle), canvas.at(x, static_cast<int>(row_y(first + 1))));
        EXPECT_EQ(canvas.at(x, seam - 1), above) << "the marks of neighbouring rows run together";
        EXPECT_EQ(canvas.at(x, seam), below) << "the marks of neighbouring rows run together";
    }

    TEST_F(Browsing, MovingTheOnlyMarkRepaintsTheListWithoutALayout) {
        ASSERT_NO_FATAL_FAILURE(open());

        click_row(row_of("b.txt"));
        overlays::click(canvas.root(), rows->box().x + 120.0, row_y(row_of("A.TXT")));
        pending = false;
        dialog->sync();

        EXPECT_FALSE(canvas.root().settle()) << "moving the mark laid the whole window out again";
        EXPECT_TRUE(canvas.root().dirty());

        const std::size_t painted = canvas.frame();

        EXPECT_GT(painted, 0U);
        EXPECT_LT(painted, static_cast<std::size_t>(canvas.width() * canvas.height() / 2));
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST_F(Browsing, MovingTheOnlyMarkRepaintsBothRows) {
        ASSERT_NO_FATAL_FAILURE(open());

        click_row(row_of("b.txt"));
        click_row(row_of("A.TXT"));

        EXPECT_EQ(marked(picker), (Names{"A.TXT"}));
        EXPECT_TRUE(canvas.matches_full()) << "the old mark was left painted";
    }

    TEST_F(Browsing, NavigatingAsksForALayout) {
        ASSERT_NO_FATAL_FAILURE(open());

        picker.go(at("alpha"));
        pending = false;
        dialog->sync();

        EXPECT_TRUE(canvas.root().settle());
    }

    TEST_F(LongBrowsing, EnteringAFolderScrollsToTheTop) {
        ASSERT_NO_FATAL_FAILURE(open());
        ASSERT_TRUE(rows->scrollable());

        rows->scroll_to(200.0);
        canvas.frame();
        picker.go(at("dir2"));
        pump();

        EXPECT_EQ(picker.state().path, at("dir2"));
        EXPECT_DOUBLE_EQ(rows->offset(), 0.0);
    }

    TEST_F(LongBrowsing, MarkingLeavesTheListWhereItWas) {
        ASSERT_NO_FATAL_FAILURE(open());

        rows->scroll_to(200.0);
        canvas.frame();

        const double offset = rows->offset();

        click_row(row_of("pad5.txt"));

        EXPECT_EQ(marked(picker), (Names{"pad5.txt"}));
        EXPECT_DOUBLE_EQ(rows->offset(), offset);
    }

    TEST_F(LongBrowsing, ScrolledRowsAreHitWhereTheyAreDrawn) {
        ASSERT_NO_FATAL_FAILURE(open());

        rows->scroll_to(300.0);
        canvas.frame();
        click_row(row_of("pad7.txt"));

        EXPECT_EQ(marked(picker), (Names{"pad7.txt"}));
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST_F(Browsing, CloseButtonDismissesWithoutChoosing) {
        ASSERT_NO_FATAL_FAILURE(open());

        click_row(row_of("b.txt"));
        click(shut);

        EXPECT_FALSE(picker.state().open);
        EXPECT_EQ(dialog, nullptr);
        EXPECT_TRUE(chosen.empty());
    }

    TEST_F(Browsing, ScrimClickDismisses) {
        ASSERT_NO_FATAL_FAILURE(open());

        overlays::click(canvas.root(), 4.0, canvas.height() - 4.0);
        pump();

        EXPECT_FALSE(picker.state().open);
        EXPECT_EQ(dialog, nullptr);
    }

    TEST_F(Browsing, ClosingEndsTypingBeforeItClosesTheDialog) {
        ASSERT_NO_FATAL_FAILURE(open());

        click(typer);

        ASSERT_TRUE(picker.state().editing);

        layer->close();
        pump();

        EXPECT_FALSE(picker.state().editing);
        EXPECT_EQ(layer->top(), dialog) << "the first close should only end the typing";

        layer->close();
        pump();

        EXPECT_FALSE(picker.state().open);
        EXPECT_FALSE(layer->covered());
    }

    TEST_F(Browsing, ClosingReturnsTrueWhenNotTyping) {
        ASSERT_NO_FATAL_FAILURE(open());

        EXPECT_TRUE(dialog->closing());
        EXPECT_FALSE(picker.state().open);
    }

    TEST_F(Browsing, TyperStartsTypingWithThePathFocused) {
        ASSERT_NO_FATAL_FAILURE(open());

        EXPECT_FALSE(typed->visible());

        click(typer);

        EXPECT_TRUE(picker.state().editing);
        EXPECT_TRUE(typed->visible());
        EXPECT_EQ(typed->text(), root);
        EXPECT_EQ(canvas.root().focused(), typed);
        EXPECT_FALSE(up->visible()) << "the up button stays over the path being typed";
    }

    TEST_F(Browsing, TyperAgainGoesBackToBrowsing) {
        ASSERT_NO_FATAL_FAILURE(open());

        click(typer);
        click(typer);

        EXPECT_FALSE(picker.state().editing);
        EXPECT_FALSE(typed->visible());
        EXPECT_TRUE(up->visible());
    }

    TEST_F(Browsing, ReturnInThePathListsWhatWasTyped) {
        ASSERT_NO_FATAL_FAILURE(open());

        click(typer);
        canvas.root().wrote(at("alpha"));
        key(ttk::Code::Return);

        EXPECT_EQ(picker.state().path, at("alpha"));
        EXPECT_FALSE(picker.state().editing);
    }

    TEST_F(Browsing, ReturnOnATypedFileChoosesIt) {
        ASSERT_NO_FATAL_FAILURE(open());

        click(typer);
        canvas.root().wrote(at("b.txt"));
        key(ttk::Code::Return);

        EXPECT_EQ(chosen, (std::vector<Names>{{at("b.txt")}}));
        EXPECT_EQ(dialog, nullptr);
    }

    TEST_F(Browsing, UpButtonListsTheParent) {
        ASSERT_NO_FATAL_FAILURE(open());

        click_row(row_of("alpha"));
        click(up);

        EXPECT_EQ(picker.state().path, root);
    }

    TEST_F(Browsing, HiddenToggleListsHiddenEntries) {
        ASSERT_NO_FATAL_FAILURE(open());

        EXPECT_FALSE(hidden->checked);

        click(hidden);

        EXPECT_TRUE(picker.state().hiddenShown);
        EXPECT_TRUE(hidden->checked);
        EXPECT_EQ(names(picker).front(), ".hidden_dir");
    }

    TEST_F(Browsing, OptionToggleIsShownOnlyWithAnOption) {
        ASSERT_NO_FATAL_FAILURE(open());

        EXPECT_FALSE(option->visible());
    }

    TEST_F(Browsing, OptionTickIsHandedToTheCallback) {
        ASSERT_NO_FATAL_FAILURE(open(false, false, false, "Open read-only"));

        ASSERT_TRUE(option->visible());

        click(option);

        EXPECT_TRUE(picker.state().optionSet);
        EXPECT_TRUE(option->checked);

        click_row(row_of("b.txt"));
        click(use);

        EXPECT_EQ(options, (std::vector<bool>{true}));
    }

    TEST_F(Browsing, BreadcrumbListsThatAncestor) {
        ASSERT_NO_FATAL_FAILURE(open());

        click_row(row_of("alpha"));
        click_row(row_of("deeper"));

        const std::vector<BLRect> boxes = crumbs();
        const BLRect picked = boxes[boxes.size() - 3];

        ASSERT_GE(picked.x, dialog->card()->box().x) << "the crumb worked out lies outside the strip";

        overlays::click(canvas.root(), overlays::mid_x(picked), overlays::mid_y(picked));
        pump();

        EXPECT_EQ(picker.state().path, root);
    }

    TEST_F(Browsing, LastBreadcrumbStaysPut) {
        ASSERT_NO_FATAL_FAILURE(open());

        click_row(row_of("alpha"));

        const BLRect last = crumbs().back();

        overlays::click(canvas.root(), overlays::mid_x(last), overlays::mid_y(last));
        pump();

        EXPECT_EQ(picker.state().path, at("alpha"));
    }

    TEST_F(Browsing, BreadcrumbsShowAPointingCursor) {
        ASSERT_NO_FATAL_FAILURE(open());

        const BLRect last = crumbs().back();
        const BLRect card = dialog->card()->box();

        EXPECT_EQ(dialog->cursor_at(overlays::mid_x(last), overlays::mid_y(last)), ttk::Cursor::Pointer);
        EXPECT_EQ(dialog->cursor_at(card.x + 4.0, card.y + card.h - 4.0), ttk::Cursor::Default);
    }

    TEST_F(Browsing, HoveredBreadcrumbIsLitAndRepaintedAlone) {
        ASSERT_NO_FATAL_FAILURE(open());

        const std::vector<BLRect> boxes = crumbs();
        const BLRect picked = boxes[boxes.size() - 2];
        const std::vector<std::uint32_t> before = canvas.pixels();

        canvas.root().motion(overlays::mid_x(picked), overlays::mid_y(picked));
        canvas.frame();

        const BLRectI lit = overlays::changed(before, canvas.pixels(), canvas.width());

        ASSERT_GT(lit.w, 0) << "hovering a breadcrumb changed nothing";
        EXPECT_GE(lit.y, static_cast<int>(picked.y));
        EXPECT_LE(lit.y + lit.h, static_cast<int>(picked.y + picked.h) + 1);
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST_F(Browsing, SavingShowsTheNameFocusedAndSeeded) {
        ASSERT_NO_FATAL_FAILURE(save("report"));

        EXPECT_TRUE(named->visible());
        EXPECT_EQ(named->text(), "report");
        EXPECT_EQ(canvas.root().focused(), named);
        EXPECT_TRUE(use->enabled());
    }

    TEST_F(Browsing, NameFieldIsHiddenWhenNotSaving) {
        ASSERT_NO_FATAL_FAILURE(open());

        EXPECT_FALSE(named->visible());
    }

    TEST_F(Browsing, SaveButtonSavesTheTarget) {
        ASSERT_NO_FATAL_FAILURE(save("report"));

        click(use);

        EXPECT_EQ(chosen, (std::vector<Names>{{at("report.txt")}}));
        EXPECT_EQ(dialog, nullptr);
    }

    TEST_F(Browsing, TypingANameUpdatesTheTarget) {
        ASSERT_NO_FATAL_FAILURE(save("report"));

        canvas.root().key(ttk::Key{.code = 'a', .ctrl = true});
        canvas.root().wrote("summary");
        pump();

        EXPECT_EQ(picker.state().target, at("summary.txt"));
    }

    TEST_F(Browsing, ReturnInTheNameSavesANewFile) {
        ASSERT_NO_FATAL_FAILURE(save("report"));

        key(ttk::Code::Return);

        EXPECT_EQ(chosen, (std::vector<Names>{{at("report.txt")}}));
    }

    TEST_F(Browsing, ReturnInTheNameNeverOverwrites) {
        ASSERT_NO_FATAL_FAILURE(save("b"));

        ASSERT_TRUE(picker.state().replacing);

        key(ttk::Code::Return);

        EXPECT_TRUE(chosen.empty());
        EXPECT_NE(dialog, nullptr);
        ASSERT_FALSE(notifier.messages().empty());
        EXPECT_EQ(notifier.messages().back().severity, ttk::Severity::Warning);
    }

    TEST_F(Browsing, ReplaceButtonOverwrites) {
        ASSERT_NO_FATAL_FAILURE(save("b"));

        click(use);

        EXPECT_EQ(chosen, (std::vector<Names>{{at("b.txt")}}));
    }

    TEST_F(Browsing, ClickOnAFileWhileSavingTakesItsName) {
        ASSERT_NO_FATAL_FAILURE(save("report"));

        click_row(row_of("file2.txt"));

        EXPECT_EQ(named->text(), "file2.txt");
        EXPECT_EQ(picker.state().target, at("file2.txt"));
        EXPECT_TRUE(picker.state().replacing);
        EXPECT_TRUE(marked(picker).empty()) << "a click while saving marked the file";
    }

    TEST_F(Browsing, EmptyNameDisablesSave) {
        ASSERT_NO_FATAL_FAILURE(save(""));

        EXPECT_FALSE(use->enabled());
    }

    TEST_F(Browsing, EmptyDirectorySaysWhyAtTheBottom) {
        ASSERT_NO_FATAL_FAILURE(open());

        const BLRect card = dialog->card()->box();
        const BLRect note{card.x + 18.0, card.y + card.h - 18.0 - ttk::Theme::controlSmall, 260.0,
                          ttk::Theme::controlSmall};
        const auto sample = [&] {
            std::vector<std::uint32_t> out;

            for (int y = static_cast<int>(note.y); y < static_cast<int>(note.y + note.h); ++y) {
                for (int x = static_cast<int>(note.x); x < static_cast<int>(note.x + note.w); ++x) {
                    out.push_back(canvas.at(x, y));
                }
            }

            return out;
        };
        const std::vector<std::uint32_t> listed = sample();

        click_row(row_of("Beta"));

        ASSERT_TRUE(picker.state().entries.empty());
        EXPECT_NE(sample(), listed) << "nothing was written for the empty listing";
    }

    TEST_F(Browsing, DriveListHidesUpAndUseForADirectoryPick) {
        ASSERT_NO_FATAL_FAILURE(open(true));

        picker.show_drives();
        pump();

        EXPECT_FALSE(up->visible());
        EXPECT_FALSE(use->visible());
    }

    TEST_F(Browsing, TabStaysInsideThePicker) {
        const auto *page = static_cast<Taker *>(canvas.root().content()->add(std::make_unique<Taker>()));

        canvas.root().relayout();

        ASSERT_NO_FATAL_FAILURE(open());

        for (int step = 0; step < 20; ++step) {
            canvas.root().key(ttk::Key{.code = ttk::Code::Tab});

            ASSERT_NE(canvas.root().focused(), page) << "Tab left the dialog at step " << step;
            ASSERT_NE(canvas.root().focused(), nullptr);
        }
    }

    TEST_F(Browsing, PaintMatchesAFullRepaintThroughItsLife) {
        picker.open("Pick a file", {"*.txt"}, false, false, true, "here", record());
        pending = false;
        dialog = static_cast<ttk::FilePickerDialog *>(layer->show(std::make_unique<ttk::FilePickerDialog>(picker)));
        ASSERT_NO_FATAL_FAILURE(find());
        dialog->sync();

        for (const double now : {0.0, 0.04, 0.09, 0.5}) {
            canvas.frame_at(now);

            ASSERT_TRUE(canvas.matches_full()) << "partial/full mismatch while growing at " << now;
        }

        canvas.root().set_now(1.0);
        click_row(row_of("b.txt"));
        click_row(row_of("file2.txt"));

        EXPECT_TRUE(canvas.matches_full()) << "marked rows partial/full mismatch";

        canvas.root().motion(rows->box().x + 100.0, row_y(row_of("A.TXT")));
        canvas.frame();

        EXPECT_TRUE(canvas.matches_full()) << "hovered row partial/full mismatch";

        click_row(row_of("alpha"));

        EXPECT_TRUE(canvas.matches_full()) << "entered folder partial/full mismatch";
    }
}
