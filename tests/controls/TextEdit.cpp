// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Mount.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Event.h"
#include "ttk/toolkit/controls/TextEdit.h"

namespace {
    using controls::Calls;
    using controls::key;
    namespace Code = ttk::Code;

    constexpr BLRect BOX{20, 20, 360, 200};
    constexpr double PAD_X = 10.0;
    constexpr double PAD_Y = 8.0;

    std::string probe(ttk::TextEdit *edit) {
        edit->wrote("|");

        return edit->text();
    }

    std::string numbered_lines(const int count) {
        std::string out;

        for (int line = 0; line < count; ++line) {
            out += "L" + std::string(line < 10 ? "0" : "") + std::to_string(line);

            if (line + 1 < count) {
                out += '\n';
            }
        }

        return out;
    }

    class TextEditTest : public controls::Painted {
    protected:
        tests::Canvas canvas;
        Calls calls;
        double cw = 0.0;
        double lh = 0.0;

        void SetUp() override {
            Painted::SetUp();

            if (IsSkipped()) {
                return;
            }

            ttk::Typeface &type = tests::fonts();
            const BLFont &font = type.at(ttk::Typeface::mono, ttk::Theme::fontSmall);

            cw = type.width(font, "0000000000") / 10.0;
            lh = std::ceil(type.line_height(font) + 4.0);
        }

        ttk::TextEdit *make(const std::string &text = {}, const BLRect &spot = BOX,
                            const ttk::TextEdit::Cog cog = ttk::TextEdit::Cog::Hidden) {
            auto made = std::make_unique<ttk::TextEdit>(calls.recorder(), cog);

            made->set_text(text);

            return controls::mount(canvas, std::move(made), spot);
        }

        ttk::TextEdit *focused(const std::string &text = {}, const BLRect &spot = BOX) {
            ttk::TextEdit *edit = make(text, spot);

            canvas.root().focus(edit);
            canvas.frame();

            return edit;
        }

        [[nodiscard]] double gutter(const std::size_t lines) const {
            return (static_cast<double>(std::max<std::size_t>(2, std::to_string(lines).size())) * cw) + (2.0 * PAD_X);
        }

        [[nodiscard]] static double view_height(const BLRect &spot = BOX) { return spot.h - (2.0 * PAD_Y); }

        [[nodiscard]] static double text_width(const BLRect &spot = BOX, const double gutterWidth = 0.0) {
            return spot.w - ttk::Theme::lane - gutterWidth - PAD_X;
        }

        // The box whose text area is `columns` wide, so wrapped rows hold that many.
        [[nodiscard]] BLRect fitting(const double columns) const {
            return BLRect{20, 20, ttk::Theme::lane + PAD_X + ((columns + 0.5) * cw), 200};
        }

        void press_cell(const ttk::TextEdit *edit, const double row, const double column,
                        const double gutterWidth = 0.0, const bool shift = false) {
            const double x = edit->box().x + gutterWidth + PAD_X + (column * cw) + 0.1;
            const double y = edit->box().y + PAD_Y + (row * lh) + (lh / 2.0);

            controls::click(canvas.root(), x, y, shift);
        }

        static void type_keys(ttk::TextEdit *edit, const int code, const int times, const bool ctrl = false,
                              const bool shift = false) {
            for (int at = 0; at < times; ++at) {
                edit->key(key(code, ctrl, shift));
            }
        }
    };

    TEST_F(TextEditTest, StartsEmptyAndEditable) {
        const ttk::TextEdit *edit = make();

        EXPECT_TRUE(edit->text().empty());
        EXPECT_TRUE(edit->editable());
        EXPECT_TRUE(edit->selection().empty());
    }

    TEST_F(TextEditTest, SetTextRemovesCarriageReturns) {
        ttk::TextEdit *edit = make();

        edit->set_text("one\r\ntwo\rthree\n");

        EXPECT_EQ(edit->text(), "one\ntwothree\n");
    }

    TEST_F(TextEditTest, SetTextDoesNotCallEdited) {
        ttk::TextEdit *edit = make();

        edit->set_text("quiet");

        EXPECT_TRUE(calls.texts.empty());
    }

    TEST_F(TextEditTest, SetTextMovesTheCaretToTheStart) {
        ttk::TextEdit *edit = focused("abc\ndef");

        edit->key(key(Code::End, true));
        edit->set_text("new\ntext");

        EXPECT_EQ(probe(edit), "|new\ntext");
    }

    TEST_F(TextEditTest, SetTextClearsTheHistory) {
        ttk::TextEdit *edit = focused();

        edit->wrote("typed");
        edit->set_text("fresh");
        edit->key(key('z', true));

        EXPECT_EQ(edit->text(), "fresh") << "undo reached past set_text";
    }

    TEST_F(TextEditTest, SetTextScrollsBackToTheTop) {
        ttk::TextEdit *edit = focused(numbered_lines(40));

        edit->key(key(Code::End, true));
        edit->set_text(numbered_lines(40));
        canvas.frame();
        press_cell(edit, 0, 0);

        EXPECT_EQ(probe(edit).find('|'), 0U);
    }

    TEST_F(TextEditTest, SetTextRepaints) {
        ttk::TextEdit *edit = make("a");

        canvas.frame();
        edit->set_text("b");

        EXPECT_TRUE(canvas.root().dirty());
    }

    TEST_F(TextEditTest, WroteInsertsAndReportsTheWholeText) {
        ttk::TextEdit *edit = focused("ac");

        edit->key(key(Code::Right));
        edit->wrote("b");

        EXPECT_EQ(edit->text(), "abc");
        EXPECT_EQ(calls.texts, (std::vector<std::string>{"abc"}));
    }

    TEST_F(TextEditTest, WroteRemovesCarriageReturns) {
        ttk::TextEdit *edit = focused();

        edit->wrote("a\r\nb");

        EXPECT_EQ(edit->text(), "a\nb");
    }

    TEST_F(TextEditTest, WroteReplacesTheSelection) {
        ttk::TextEdit *edit = focused("one\ntwo");

        edit->select_all();
        edit->wrote("x");

        EXPECT_EQ(edit->text(), "x");
    }

    TEST_F(TextEditTest, WroteMultiLineTextSplitsLines) {
        ttk::TextEdit *edit = focused();

        edit->wrote("a\nb\nc");
        edit->key(key(Code::Up));

        EXPECT_EQ(probe(edit), "a\nb|\nc");
    }

    TEST_F(TextEditTest, SelectionIsEmptyWithoutOne) {
        ttk::TextEdit *edit = focused("text");

        edit->key(key(Code::End));

        EXPECT_EQ(edit->selection(), "");
    }

    TEST_F(TextEditTest, SelectAllSelectsEveryLine) {
        ttk::TextEdit *edit = make("one\ntwo\n");

        edit->select_all();

        EXPECT_EQ(edit->selection(), "one\ntwo\n");
    }

    TEST_F(TextEditTest, SelectAllLeavesTheCaretAtTheEnd) {
        ttk::TextEdit *edit = focused("one\ntwo");

        edit->select_all();
        edit->key(key(Code::Left, false, true));

        EXPECT_EQ(edit->selection(), "one\ntw");
    }

    TEST_F(TextEditTest, CtrlASelectsAll) {
        ttk::TextEdit *edit = focused("one\ntwo");

        EXPECT_TRUE(edit->key(key('a', true)));
        EXPECT_EQ(edit->selection(), "one\ntwo");
    }

    TEST_F(TextEditTest, RightCrossesALineBreak) {
        ttk::TextEdit *edit = focused("ab\ncd");

        type_keys(edit, Code::Right, 3);

        EXPECT_EQ(probe(edit), "ab\n|cd");
    }

    TEST_F(TextEditTest, LeftCrossesALineBreak) {
        ttk::TextEdit *edit = focused("ab\ncd");

        edit->key(key(Code::End, true));
        type_keys(edit, Code::Left, 3);

        EXPECT_EQ(probe(edit), "ab|\ncd");
    }

    TEST_F(TextEditTest, LeftAndRightStepOverWholeCodePoints) {
        ttk::TextEdit *edit = focused("aé😀b");

        edit->key(key(Code::Right));
        edit->key(key(Code::Right));

        EXPECT_EQ(probe(edit), "aé|😀b");

        edit->set_text("aé😀b");
        edit->key(key(Code::End));
        edit->key(key(Code::Left));
        edit->key(key(Code::Left));

        EXPECT_EQ(probe(edit), "aé|😀b") << "Left did not step back over the whole emoji";
    }

    TEST_F(TextEditTest, LeftAtTheStartAndRightAtTheEndStay) {
        ttk::TextEdit *edit = focused("ab");

        EXPECT_TRUE(edit->key(key(Code::Left)));
        EXPECT_EQ(probe(edit), "|ab");

        edit->key(key(Code::End, true));

        EXPECT_TRUE(edit->key(key(Code::Right)));
        EXPECT_EQ(probe(edit), "|ab|");
    }

    TEST_F(TextEditTest, LeftCollapsesASelectionToItsStart) {
        ttk::TextEdit *edit = focused("abc\ndef");

        edit->select_all();
        edit->key(key(Code::Left));

        EXPECT_EQ(probe(edit), "|abc\ndef");
    }

    TEST_F(TextEditTest, RightCollapsesASelectionToItsEnd) {
        ttk::TextEdit *edit = focused("abcdef");

        type_keys(edit, Code::Right, 2, false, true);
        edit->key(key(Code::Right));

        EXPECT_EQ(probe(edit), "ab|cdef");
    }

    TEST_F(TextEditTest, CtrlRightMovesPastTheNextWord) {
        ttk::TextEdit *edit = focused("one two three");

        edit->key(key(Code::Right, true));
        edit->key(key(Code::Right, true));

        EXPECT_EQ(probe(edit), "one two| three");
    }

    TEST_F(TextEditTest, CtrlRightFromSpacesBeforeABreakStopsAfterIt) {
        ttk::TextEdit *edit = focused("ab  \ncd");

        type_keys(edit, Code::Right, 2);
        edit->key(key(Code::Right, true));

        EXPECT_EQ(probe(edit), "ab  \n|cd");
    }

    TEST_F(TextEditTest, CtrlLeftMovesToTheStartOfTheWord) {
        ttk::TextEdit *edit = focused("one two");

        edit->key(key(Code::End));
        edit->key(key(Code::Left, true));

        EXPECT_EQ(probe(edit), "one |two");
    }

    TEST_F(TextEditTest, CtrlLeftAtALineStartStopsAtTheEndOfTheLineAbove) {
        ttk::TextEdit *edit = focused("ab\ncd");

        edit->key(key(Code::Down));
        edit->key(key(Code::Home));
        edit->key(key(Code::Left, true));

        EXPECT_EQ(probe(edit), "ab|\ncd");
    }

    TEST_F(TextEditTest, CtrlShiftRightSelectsAWord) {
        ttk::TextEdit *edit = focused("naïve café");

        edit->key(key(Code::Right, true, true));

        EXPECT_EQ(edit->selection(), "naïve");
    }

    TEST_F(TextEditTest, DownKeepsTheGoalColumnAcrossAShortLine) {
        ttk::TextEdit *edit = focused("abcdef\nab\nabcdef");

        type_keys(edit, Code::Right, 5);
        edit->key(key(Code::Down));

        EXPECT_EQ(probe(edit), "abcdef\nab|\nabcdef");

        edit->set_text("abcdef\nab\nabcdef");
        type_keys(edit, Code::Right, 5);
        edit->key(key(Code::Down));
        edit->key(key(Code::Down));

        EXPECT_EQ(probe(edit), "abcdef\nab\nabcde|f");
    }

    TEST_F(TextEditTest, UpKeepsTheGoalColumnAcrossAShortLine) {
        ttk::TextEdit *edit = focused("abcdef\nab\nabcdef");

        edit->key(key(Code::End, true));
        edit->key(key(Code::Left));
        edit->key(key(Code::Up));
        edit->key(key(Code::Up));

        EXPECT_EQ(probe(edit), "abcde|f\nab\nabcdef");
    }

    TEST_F(TextEditTest, AHorizontalMoveResetsTheGoalColumn) {
        ttk::TextEdit *edit = focused("abcdef\nab\nabcdef");

        type_keys(edit, Code::Right, 5);
        edit->key(key(Code::Down));
        edit->key(key(Code::Left));
        edit->key(key(Code::Down));

        EXPECT_EQ(probe(edit), "abcdef\nab\na|bcdef");
    }

    TEST_F(TextEditTest, UpOnTheFirstRowGoesToTheStart) {
        ttk::TextEdit *edit = focused("abc\ndef");

        type_keys(edit, Code::Right, 2);
        edit->key(key(Code::Up));

        EXPECT_EQ(probe(edit), "|abc\ndef");
    }

    TEST_F(TextEditTest, DownOnTheLastRowGoesToTheEnd) {
        ttk::TextEdit *edit = focused("abc\ndef");

        edit->key(key(Code::Down));
        edit->key(key(Code::Down));

        EXPECT_EQ(probe(edit), "abc\ndef|");
    }

    TEST_F(TextEditTest, UpAndDownCountColumnsInCodePoints) {
        ttk::TextEdit *edit = focused("éé\nabc");

        edit->key(key(Code::End));
        edit->key(key(Code::Down));

        EXPECT_EQ(probe(edit), "éé\nab|c");
    }

    TEST_F(TextEditTest, UpAndDownExpandTabs) {
        ttk::TextEdit *edit = focused("\tx\nabcdef");

        edit->key(key(Code::End));
        edit->key(key(Code::Down));

        EXPECT_EQ(probe(edit), "\tx\nabcde|f") << "a tab did not count as four columns";
    }

    TEST_F(TextEditTest, HomeGoesToTheFirstNonBlankThenToTheStart) {
        ttk::TextEdit *edit = focused("    foo");

        edit->key(key(Code::End));
        edit->key(key(Code::Home));

        EXPECT_EQ(probe(edit), "    |foo");

        edit->key(key(Code::Left));
        edit->key(key(Code::Home));

        EXPECT_EQ(probe(edit), "|    |foo");
    }

    TEST_F(TextEditTest, HomeInsideTheIndentGoesToTheFirstNonBlank) {
        ttk::TextEdit *edit = focused("\t  foo");

        edit->key(key(Code::Right));
        edit->key(key(Code::Home));

        EXPECT_EQ(probe(edit), "\t  |foo");
    }

    TEST_F(TextEditTest, EndGoesToTheEndOfTheLine) {
        ttk::TextEdit *edit = focused("ab\ncd");

        edit->key(key(Code::End));

        EXPECT_EQ(probe(edit), "ab|\ncd");
    }

    TEST_F(TextEditTest, CtrlHomeAndCtrlEndGoToEitherEndOfTheText) {
        ttk::TextEdit *edit = focused("ab\ncd\nef");

        edit->key(key(Code::Down));
        edit->key(key(Code::End, true));

        EXPECT_EQ(probe(edit), "ab\ncd\nef|");

        edit->key(key(Code::Home, true));

        EXPECT_EQ(probe(edit), "|ab\ncd\nef|");
    }

    TEST_F(TextEditTest, ShiftDownSelectsAcrossLines) {
        ttk::TextEdit *edit = focused("line one\nline two\nline three");

        edit->key(key(Code::Down, false, true));
        edit->key(key(Code::End, false, true));

        EXPECT_EQ(edit->selection(), "line one\nline two");
    }

    TEST_F(TextEditTest, ShiftUpSelectsBackwards) {
        ttk::TextEdit *edit = focused("ab\ncd");

        edit->key(key(Code::End, true));
        edit->key(key(Code::Up, false, true));

        EXPECT_EQ(edit->selection(), "\ncd");
    }

    TEST_F(TextEditTest, CtrlShiftEndSelectsToTheEnd) {
        ttk::TextEdit *edit = focused("ab\ncd");

        edit->key(key(Code::Right));
        edit->key(key(Code::End, true, true));

        EXPECT_EQ(edit->selection(), "b\ncd");
    }

    TEST_F(TextEditTest, PageDownMovesAViewLessOneRow) {
        ttk::TextEdit *edit = focused(numbered_lines(40));
        const auto page = static_cast<std::size_t>(std::floor(view_height() / lh) - 1.0);

        edit->key(key(Code::Right));
        edit->key(key(Code::PageDown));

        EXPECT_EQ(probe(edit).find('|'), (page * 4) + 1) << "page of " << page << " rows";
    }

    TEST_F(TextEditTest, PageUpMovesBackAPage) {
        ttk::TextEdit *edit = focused(numbered_lines(40));

        edit->key(key(Code::PageDown));
        edit->key(key(Code::PageDown));
        edit->key(key(Code::PageUp));
        edit->key(key(Code::PageUp));

        EXPECT_EQ(probe(edit).find('|'), 0U);
    }

    TEST_F(TextEditTest, PageDownStopsAtTheLastRow) {
        ttk::TextEdit *edit = focused(numbered_lines(12));

        type_keys(edit, Code::PageDown, 4);

        EXPECT_EQ(probe(edit).find('|'), 11U * 4U);
    }

    TEST_F(TextEditTest, ShiftPageDownSelects) {
        ttk::TextEdit *edit = focused(numbered_lines(40));

        edit->key(key(Code::PageDown, false, true));

        EXPECT_FALSE(edit->selection().empty());
        EXPECT_TRUE(edit->selection().starts_with("L00\nL01\n"));
    }

    TEST_F(TextEditTest, ReturnInsertsALineBreak) {
        ttk::TextEdit *edit = focused("abcd");

        type_keys(edit, Code::Right, 2);

        EXPECT_TRUE(edit->key(key(Code::Return)));
        EXPECT_EQ(edit->text(), "ab\ncd");
        EXPECT_EQ(calls.texts.back(), "ab\ncd");
        EXPECT_EQ(probe(edit), "ab\n|cd");
    }

    TEST_F(TextEditTest, ReturnKeepsTheIndent) {
        ttk::TextEdit *edit = focused("  \tfoo");

        edit->key(key(Code::End));
        edit->key(key(Code::Return));

        EXPECT_EQ(edit->text(), "  \tfoo\n  \t");
    }

    TEST_F(TextEditTest, ReturnInsideTheIndentKeepsOnlyWhatIsBeforeTheCaret) {
        ttk::TextEdit *edit = focused("    foo");

        type_keys(edit, Code::Right, 2);
        edit->key(key(Code::Return));

        EXPECT_EQ(edit->text(), "  \n    foo");
    }

    TEST_F(TextEditTest, ReturnReplacesTheSelection) {
        ttk::TextEdit *edit = focused("abXYcd");

        type_keys(edit, Code::Right, 2);
        type_keys(edit, Code::Right, 2, false, true);
        edit->key(key(Code::Return));

        EXPECT_EQ(edit->text(), "ab\ncd");
    }

    TEST_F(TextEditTest, TabInsertsSpacesToTheNextStop) {
        ttk::TextEdit *edit = focused("ab");

        edit->key(key(Code::Tab));

        EXPECT_EQ(edit->text(), "    ab");

        edit->key(key(Code::End));

        EXPECT_TRUE(edit->key(key(Code::Tab)));
        EXPECT_EQ(edit->text(), "    ab  ");

        edit->key(key(Code::Tab));

        EXPECT_EQ(edit->text(), "    ab      ");
    }

    TEST_F(TextEditTest, TabCountsColumnsInCodePoints) {
        ttk::TextEdit *edit = focused("ééé");

        edit->key(key(Code::End));
        edit->key(key(Code::Tab));

        EXPECT_EQ(edit->text(), "ééé ");
    }

    TEST_F(TextEditTest, TabWithAModifierIsNotConsumed) {
        ttk::TextEdit *edit = focused("ab");

        EXPECT_FALSE(edit->key(ttk::Key{.code = Code::Tab, .shift = true}));
        EXPECT_FALSE(edit->key(ttk::Key{.code = Code::Tab, .ctrl = true}));
        EXPECT_FALSE(edit->key(ttk::Key{.code = Code::Tab, .alt = true}));
        EXPECT_EQ(edit->text(), "ab");
    }

    TEST_F(TextEditTest, ShiftTabLeavesTheEditor) {
        auto other = std::make_unique<ttk::TextEdit>();
        ttk::TextEdit *first = controls::mount(canvas, std::move(other), BLRect{20, 20, 100, 60});
        ttk::TextEdit *edit = make("ab", BLRect{20, 100, 200, 100});

        canvas.root().focus(edit);

        EXPECT_TRUE(canvas.root().key(ttk::Key{.code = Code::Tab, .shift = true}));
        EXPECT_EQ(canvas.root().focused(), first);
        EXPECT_EQ(edit->text(), "ab");
    }

    TEST_F(TextEditTest, BackspaceJoinsALineWithTheOneAbove) {
        ttk::TextEdit *edit = focused("ab\ncd");

        edit->key(key(Code::Down));
        edit->key(key(Code::Home));
        edit->key(key(Code::Backspace));

        EXPECT_EQ(edit->text(), "abcd");
        EXPECT_EQ(probe(edit), "ab|cd");
    }

    TEST_F(TextEditTest, DeleteJoinsALineWithTheOneBelow) {
        ttk::TextEdit *edit = focused("ab\ncd");

        edit->key(key(Code::End));
        edit->key(key(Code::Delete));

        EXPECT_EQ(edit->text(), "abcd");
        EXPECT_EQ(calls.texts.back(), "abcd");
    }

    TEST_F(TextEditTest, BackspaceAtTheStartDoesNothing) {
        ttk::TextEdit *edit = focused("ab");

        EXPECT_TRUE(edit->key(key(Code::Backspace)));
        EXPECT_TRUE(edit->key(key(Code::Backspace, true)));
        EXPECT_EQ(edit->text(), "ab");
        EXPECT_TRUE(calls.texts.empty());
    }

    TEST_F(TextEditTest, DeleteAtTheEndDoesNothing) {
        ttk::TextEdit *edit = focused("ab");

        edit->key(key(Code::End, true));

        EXPECT_TRUE(edit->key(key(Code::Delete)));
        EXPECT_TRUE(edit->key(key(Code::Delete, true)));
        EXPECT_EQ(edit->text(), "ab");
        EXPECT_TRUE(calls.texts.empty());
    }

    TEST_F(TextEditTest, BackspaceAndDeleteRemoveWholeCodePoints) {
        ttk::TextEdit *edit = focused("é😀x");

        edit->key(key(Code::Delete));

        EXPECT_EQ(edit->text(), "😀x");

        edit->key(key(Code::Right));
        edit->key(key(Code::Backspace));

        EXPECT_EQ(edit->text(), "x");
    }

    TEST_F(TextEditTest, BackspaceRemovesASelectionAcrossLines) {
        ttk::TextEdit *edit = focused("abc\ndef");

        edit->key(key(Code::Right));
        edit->key(key(Code::Down, false, true));
        edit->key(key(Code::Backspace));

        EXPECT_EQ(edit->text(), "aef");
    }

    TEST_F(TextEditTest, CtrlBackspaceAtALineStartRemovesOnlyTheBreak) {
        ttk::TextEdit *edit = focused("ab\ncd");

        edit->key(key(Code::Down));
        edit->key(key(Code::Home));
        edit->key(key(Code::Backspace, true));

        EXPECT_EQ(edit->text(), "abcd");
    }

    TEST_F(TextEditTest, CtrlBackspaceRemovesTheWordBefore) {
        ttk::TextEdit *edit = focused("one two");

        edit->key(key(Code::End));
        edit->key(key(Code::Backspace, true));

        EXPECT_EQ(edit->text(), "one ");
    }

    TEST_F(TextEditTest, CtrlDeleteRemovesTheWordAfter) {
        ttk::TextEdit *edit = focused("one two");

        edit->key(key(Code::Delete, true));

        EXPECT_EQ(edit->text(), " two");
    }

    TEST_F(TextEditTest, CtrlDeleteFromSpacesTakesTheBreak) {
        ttk::TextEdit *edit = focused("ab \ncd");

        type_keys(edit, Code::Right, 2);
        edit->key(key(Code::Delete, true));

        EXPECT_EQ(edit->text(), "abcd");
    }

    TEST_F(TextEditTest, CtrlCIsConsumedAndChangesNothing) {
        ttk::TextEdit *edit = focused("copy");

        edit->select_all();

        EXPECT_TRUE(edit->key(key('c', true)));
        EXPECT_EQ(edit->text(), "copy");
        EXPECT_EQ(edit->selection(), "copy");
        EXPECT_TRUE(calls.texts.empty());
    }

    TEST_F(TextEditTest, CtrlXRemovesTheSelection) {
        ttk::TextEdit *edit = focused("cut\nthis");

        edit->key(key(Code::Down, false, true));

        EXPECT_TRUE(edit->key(key('x', true)));
        EXPECT_EQ(edit->text(), "this");
        EXPECT_EQ(calls.texts, (std::vector<std::string>{"this"}));
    }

    TEST_F(TextEditTest, CtrlXWithoutASelectionChangesNothing) {
        ttk::TextEdit *edit = focused("keep");

        EXPECT_TRUE(edit->key(key('x', true)));
        EXPECT_EQ(edit->text(), "keep");
        EXPECT_TRUE(calls.texts.empty());
    }

    TEST_F(TextEditTest, CtrlVIsConsumed) {
        ttk::TextEdit *edit = focused("paste");

        EXPECT_TRUE(edit->key(key('v', true)));
    }

    TEST_F(TextEditTest, OtherKeysAreNotConsumed) {
        ttk::TextEdit *edit = focused("abc");

        EXPECT_FALSE(edit->key(key('q')));
        EXPECT_FALSE(edit->key(key(Code::F1)));
        EXPECT_FALSE(edit->key(key(Code::Escape)));
        EXPECT_FALSE(edit->key(key('q', true)));
        EXPECT_EQ(edit->text(), "abc");
    }

    TEST_F(TextEditTest, UndoTakesBackAnEdit) {
        ttk::TextEdit *edit = focused("abc");

        edit->key(key(Code::End));
        edit->key(key(Code::Backspace));

        EXPECT_TRUE(edit->key(key('z', true)));
        EXPECT_EQ(edit->text(), "abc");
        EXPECT_EQ(calls.texts.back(), "abc") << "undo did not report its edit";
    }

    TEST_F(TextEditTest, UndoRestoresTheCaret) {
        ttk::TextEdit *edit = focused("abc");

        edit->key(key(Code::End));
        edit->key(key(Code::Backspace));
        edit->key(key(Code::Home));
        edit->key(key('z', true));

        EXPECT_EQ(probe(edit), "abc|");
    }

    TEST_F(TextEditTest, UndoRestoresTheSelection) {
        ttk::TextEdit *edit = focused("abc\ndef");

        edit->select_all();
        edit->key(key(Code::Backspace));
        edit->key(key('z', true));

        EXPECT_EQ(edit->selection(), "abc\ndef");
    }

    TEST_F(TextEditTest, ARunOfTypingUndoesAsOneStep) {
        ttk::TextEdit *edit = focused();

        edit->wrote("a");
        edit->wrote("b");
        edit->wrote("c");
        edit->key(key('z', true));

        EXPECT_TRUE(edit->text().empty());
    }

    TEST_F(TextEditTest, ACaretMoveEndsTheRunOfTyping) {
        ttk::TextEdit *edit = focused();

        edit->wrote("a");
        edit->wrote("b");
        edit->key(key(Code::Left));
        edit->key(key(Code::Right));
        edit->wrote("c");
        edit->key(key('z', true));

        EXPECT_EQ(edit->text(), "ab");

        edit->key(key('z', true));

        EXPECT_TRUE(edit->text().empty());
    }

    TEST_F(TextEditTest, CtrlYRedoes) {
        ttk::TextEdit *edit = focused();

        edit->wrote("abc");
        edit->key(key('z', true));

        EXPECT_TRUE(edit->key(key('y', true)));
        EXPECT_EQ(edit->text(), "abc");
        EXPECT_EQ(calls.texts.back(), "abc");
    }

    TEST_F(TextEditTest, CtrlShiftZRedoes) {
        ttk::TextEdit *edit = focused();

        edit->wrote("abc");
        edit->key(key('z', true));

        EXPECT_TRUE(edit->key(key('z', true, true)));
        EXPECT_EQ(edit->text(), "abc");
    }

    TEST_F(TextEditTest, ANewEditClearsRedo) {
        ttk::TextEdit *edit = focused();

        edit->wrote("a");
        edit->key(key('z', true));
        edit->wrote("b");
        edit->key(key('y', true));

        EXPECT_EQ(edit->text(), "b");
    }

    TEST_F(TextEditTest, UndoWithNothingToUndoDoesNothing) {
        ttk::TextEdit *edit = focused("abc");

        EXPECT_TRUE(edit->key(key('z', true)));
        EXPECT_TRUE(edit->key(key('y', true)));
        EXPECT_EQ(edit->text(), "abc");
        EXPECT_TRUE(calls.texts.empty());
    }

    TEST_F(TextEditTest, UndoKeepsAtMost256Steps) {
        ttk::TextEdit *edit = focused();
        constexpr int STEPS = 300;
        constexpr std::size_t KEPT = 256;

        for (int at = 0; at < STEPS; ++at) {
            edit->wrote("x");
            edit->key(key(Code::Left));
            edit->key(key(Code::Right));
        }

        for (int at = 0; at < STEPS; ++at) {
            edit->key(key('z', true));
        }

        EXPECT_EQ(edit->text(), std::string(STEPS - KEPT, 'x'));
    }

    TEST_F(TextEditTest, UndoOfCutAndReturnAndTab) {
        ttk::TextEdit *edit = focused("ab");

        edit->key(key(Code::End));
        edit->key(key(Code::Return));
        edit->key(key(Code::Tab));
        edit->key(key('a', true));
        edit->key(key('x', true));

        ASSERT_TRUE(edit->text().empty());

        edit->key(key('z', true));

        EXPECT_EQ(edit->text(), "ab\n    ");

        edit->key(key('z', true));

        EXPECT_EQ(edit->text(), "ab\n");

        edit->key(key('z', true));

        EXPECT_EQ(edit->text(), "ab");
    }

    TEST_F(TextEditTest, ReadOnlyIgnoresEveryEdit) {
        ttk::TextEdit *edit = focused("fixed");

        edit->wrote("x");
        edit->read_only();

        EXPECT_FALSE(edit->editable());

        edit->wrote("y");
        edit->key(key(Code::End));
        edit->key(key(Code::Backspace));
        edit->key(key(Code::Home));
        edit->key(key(Code::Delete));
        edit->key(key('v', true));
        edit->key(key('z', true));
        edit->select_all();
        edit->key(key('x', true));
        edit->key(key(Code::Backspace, true));

        EXPECT_EQ(edit->text(), "xfixed");
        EXPECT_EQ(calls.texts.size(), 1U);
    }

    TEST_F(TextEditTest, ReadOnlyLetsReturnAndTabPass) {
        ttk::TextEdit *edit = focused("fixed");

        edit->read_only();

        EXPECT_FALSE(edit->key(key(Code::Return)));
        EXPECT_FALSE(edit->key(key(Code::Tab)));
        EXPECT_EQ(edit->text(), "fixed");
    }

    TEST_F(TextEditTest, ReadOnlyStillSelectsAndCopies) {
        ttk::TextEdit *edit = focused("one two");

        edit->read_only();
        edit->key(key(Code::Right, true, true));

        EXPECT_EQ(edit->selection(), "one");
        EXPECT_TRUE(edit->key(key('c', true)));
        EXPECT_TRUE(edit->key(key('a', true)));
        EXPECT_EQ(edit->selection(), "one two");
    }

    TEST_F(TextEditTest, ReadOnlyDoesNotBlink) {
        ttk::TextEdit *edit = focused("fixed");

        edit->read_only();

        EXPECT_FALSE(edit->advance(5.0));
    }

    TEST_F(TextEditTest, ReadOnlyReturnsTheEditor) {
        ttk::TextEdit *edit = make();

        EXPECT_EQ(edit->read_only(), edit);
        EXPECT_EQ(edit->numbered(), edit);
        EXPECT_EQ(edit->wrapped(), edit);
        EXPECT_EQ(edit->placeholder("p"), edit);
    }

    TEST_F(TextEditTest, AdvanceStopsWithoutFocus) {
        ttk::TextEdit *edit = make("abc");

        EXPECT_FALSE(edit->advance(1.0));
    }

    TEST_F(TextEditTest, FocusWakesTheCaret) {
        ttk::TextEdit *edit = make("abc");

        canvas.root().advance(0.0);
        ASSERT_FALSE(canvas.root().busy());

        canvas.root().focus(edit);

        EXPECT_TRUE(canvas.root().busy());
    }

    TEST_F(TextEditTest, LosingFocusKeepsTheSelection) {
        ttk::TextEdit *edit = focused("keep me");

        edit->select_all();
        canvas.root().focus(nullptr);

        EXPECT_EQ(edit->selection(), "keep me");
    }

    TEST_F(TextEditTest, NaturalSizeIs240By180UnlessFixed) {
        ttk::TextEdit edit;

        EXPECT_DOUBLE_EQ(edit.natural_width(tests::fonts()), 240.0);
        EXPECT_DOUBLE_EQ(edit.natural_height(tests::fonts(), 100.0), 180.0);

        edit.fixedWidth = 50.0;
        edit.fixedHeight = 60.0;

        EXPECT_DOUBLE_EQ(edit.natural_width(tests::fonts()), 50.0);
        EXPECT_DOUBLE_EQ(edit.natural_height(tests::fonts(), 100.0), 60.0);
    }

    TEST_F(TextEditTest, PaintsItsOwnFocus) {
        const ttk::TextEdit edit;

        EXPECT_TRUE(edit.paints_focus());
        EXPECT_TRUE(edit.takes_focus());
    }

    TEST_F(TextEditTest, PressFocusesAndPlacesTheCaretAtTheCell) {
        ttk::TextEdit *edit = make("first\nsecond\nthird");

        press_cell(edit, 1, 3);

        EXPECT_EQ(canvas.root().focused(), edit);
        EXPECT_EQ(probe(edit), "first\nsec|ond\nthird");
    }

    TEST_F(TextEditTest, PressRoundsToTheNearestColumn) {
        ttk::TextEdit *edit = make("abcdef");

        press_cell(edit, 0, 2.6);

        EXPECT_EQ(probe(edit), "abc|def");
    }

    TEST_F(TextEditTest, PressPastTheEndOfARowGoesToItsEnd) {
        ttk::TextEdit *edit = make("ab\ncdef");

        press_cell(edit, 0, 20);

        EXPECT_EQ(probe(edit), "ab|\ncdef");
    }

    TEST_F(TextEditTest, PressBelowTheLastRowGoesToTheLastRow) {
        ttk::TextEdit *edit = make("ab\ncd");

        press_cell(edit, 6, 1);

        EXPECT_EQ(probe(edit), "ab\nc|d");
    }

    TEST_F(TextEditTest, PressLeftOfTheTextGoesToTheFirstColumn) {
        ttk::TextEdit *edit = make("ab\ncd");
        const double y = edit->box().y + PAD_Y + lh + 2.0;

        controls::click(canvas.root(), edit->box().x + 2.0, y);

        EXPECT_EQ(probe(edit), "ab\n|cd");
    }

    TEST_F(TextEditTest, PressOnATabSnapsToItsNearerSide) {
        ttk::TextEdit *edit = make("\tab");

        press_cell(edit, 0, 1);

        EXPECT_EQ(probe(edit), "|\tab");

        edit->set_text("\tab");
        press_cell(edit, 0, 3);

        EXPECT_EQ(probe(edit), "\t|ab");
    }

    TEST_F(TextEditTest, PressCountsMultiByteCharactersAsOneColumn) {
        ttk::TextEdit *edit = make("éйé");

        press_cell(edit, 0, 2);

        EXPECT_EQ(probe(edit), "éй|é");
    }

    TEST_F(TextEditTest, PressInANumberedEditorAllowsForTheGutter) {
        ttk::TextEdit *edit = make("abcdef\nline");

        edit->numbered();
        canvas.frame();
        press_cell(edit, 0, 2, gutter(2));

        EXPECT_EQ(probe(edit), "ab|cdef\nline");
    }

    TEST_F(TextEditTest, TheGutterWidensForMoreDigits) {
        const std::string text = numbered_lines(10) + "\n" + std::string(90, '\n');
        ttk::TextEdit *edit = make(text);

        edit->numbered();
        canvas.frame();
        press_cell(edit, 0, 1, gutter(101));

        EXPECT_EQ(probe(edit).find('|'), 1U) << "the gutter is not three digits wide";
    }

    TEST_F(TextEditTest, ShiftPressExtendsTheSelection) {
        const ttk::TextEdit *edit = focused("first\nsecond");

        press_cell(edit, 1, 3, 0.0, true);

        EXPECT_EQ(edit->selection(), "first\nsec");
    }

    TEST_F(TextEditTest, DragSelectsAcrossLines) {
        const ttk::TextEdit *edit = make("first\nsecond\nthird");
        const double left = edit->box().x + PAD_X + 0.1;
        const double top = edit->box().y + PAD_Y + (lh / 2.0);

        controls::sweep(canvas.root(), left + (2 * cw), top, left + (3 * cw), top + (2 * lh));

        EXPECT_EQ(edit->selection(), "rst\nsecond\nthi");
    }

    TEST_F(TextEditTest, DoubleClickSelectsAWord) {
        const ttk::TextEdit *edit = make("alpha beta gamma");

        canvas.root().set_now(1.0);
        press_cell(edit, 0, 7);
        canvas.root().set_now(1.2);
        press_cell(edit, 0, 7);

        EXPECT_EQ(edit->selection(), "beta");
    }

    TEST_F(TextEditTest, TripleClickSelectsTheLineWithItsBreak) {
        const ttk::TextEdit *edit = make("alpha beta\nnext");

        for (const double now : {1.0, 1.1, 1.2}) {
            canvas.root().set_now(now);
            press_cell(edit, 0, 7);
        }

        EXPECT_EQ(edit->selection(), "alpha beta\n");
    }

    TEST_F(TextEditTest, TripleClickOnTheLastLineSelectsToTheEnd) {
        const ttk::TextEdit *edit = make("alpha\nlast line");

        for (const double now : {1.0, 1.1, 1.2}) {
            canvas.root().set_now(now);
            press_cell(edit, 1, 2);
        }

        EXPECT_EQ(edit->selection(), "last line");
    }

    TEST_F(TextEditTest, AFourthClickStartsOver) {
        const ttk::TextEdit *edit = make("alpha beta\nnext");

        for (const double now : {1.0, 1.1, 1.2, 1.3}) {
            canvas.root().set_now(now);
            press_cell(edit, 0, 7);
        }

        EXPECT_TRUE(edit->selection().empty());
    }

    TEST_F(TextEditTest, ASlowSecondClickIsASingleClick) {
        const ttk::TextEdit *edit = make("alpha beta gamma");

        canvas.root().set_now(1.0);
        press_cell(edit, 0, 7);
        canvas.root().set_now(1.5);
        press_cell(edit, 0, 7);

        EXPECT_TRUE(edit->selection().empty());
    }

    TEST_F(TextEditTest, AFarSecondClickIsASingleClick) {
        const ttk::TextEdit *edit = make("alpha beta gamma");

        canvas.root().set_now(1.0);
        press_cell(edit, 0, 7);
        canvas.root().set_now(1.1);
        press_cell(edit, 0, 8);

        ASSERT_GE(cw, 4.0) << "a column is too narrow to be a far click";
        EXPECT_TRUE(edit->selection().empty());
    }

    TEST_F(TextEditTest, DragAfterADoubleClickKeepsTheWord) {
        const ttk::TextEdit *edit = make("alpha beta gamma");
        const double x = edit->box().x + PAD_X + (7 * cw) + 0.1;
        const double y = edit->box().y + PAD_Y + (lh / 2.0);

        canvas.root().set_now(1.0);
        controls::click(canvas.root(), x, y);
        canvas.root().set_now(1.1);
        controls::sweep(canvas.root(), x, y, x + (8 * cw), y);

        EXPECT_EQ(edit->selection(), "beta");
    }

    TEST_F(TextEditTest, DisabledEditorRefusesThePress) {
        ttk::TextEdit *edit = make("off");

        edit->set_enabled(false);

        EXPECT_FALSE(edit->press(ttk::Pointer{.x = 40.0, .y = 40.0}));
    }

    TEST_F(TextEditTest, UnattachedEditorRefusesThePress) {
        ttk::TextEdit edit;

        EXPECT_FALSE(edit.press(ttk::Pointer{.x = 1.0, .y = 1.0}));
    }

    TEST_F(TextEditTest, WheelScrollsThreeRowsPerNotch) {
        ttk::TextEdit *edit = make(numbered_lines(40));

        canvas.root().wheel(-1.0, 100.0, 100.0);
        press_cell(edit, 0, 0);

        EXPECT_EQ(probe(edit).find('|'), 3U * 4U);
    }

    TEST_F(TextEditTest, WheelStopsAtTheTop) {
        ttk::TextEdit *edit = make(numbered_lines(40));

        EXPECT_TRUE(edit->wheel(5.0, ttk::Pointer{.x = 100.0, .y = 100.0}));

        press_cell(edit, 0, 0);

        EXPECT_EQ(probe(edit).find('|'), 0U);
    }

    TEST_F(TextEditTest, WheelStopsAtTheBottom) {
        ttk::TextEdit *edit = make(numbered_lines(40));
        const double farthest = (40.0 * lh) - view_height();

        edit->wheel(-100.0, ttk::Pointer{.x = 100.0, .y = 100.0});
        press_cell(edit, 0, 0);

        const auto row = static_cast<std::size_t>(std::floor(((lh / 2.0) + farthest) / lh));

        EXPECT_EQ(probe(edit).find('|'), row * 4U);
    }

    TEST_F(TextEditTest, WheelLeavesTheScrollToTheParentWhenEverythingFits) {
        ttk::TextEdit *edit = make("short");

        EXPECT_FALSE(edit->wheel(-1.0, ttk::Pointer{.x = 100.0, .y = 100.0}));
        EXPECT_FALSE(edit->wheel(-1.0, ttk::Pointer{.x = 100.0, .y = 100.0, .shift = true}));
    }

    TEST_F(TextEditTest, ShiftWheelScrollsSideways) {
        ttk::TextEdit *edit = make(std::string(200, 'a'));

        EXPECT_TRUE(edit->wheel(-1.0, ttk::Pointer{.x = 100.0, .y = 100.0, .shift = true}));

        press_cell(edit, 0, 0);

        EXPECT_EQ(probe(edit).find('|'), 3U);
    }

    TEST_F(TextEditTest, WrappedEditorDoesNotScrollSideways) {
        ttk::TextEdit *edit = make(std::string(200, 'a'));

        edit->wrapped();
        canvas.frame();

        EXPECT_FALSE(edit->wheel(-1.0, ttk::Pointer{.x = 100.0, .y = 100.0, .shift = true}));
    }

    TEST_F(TextEditTest, MovingTheCaretDownScrollsItIntoView) {
        ttk::TextEdit *edit = focused(numbered_lines(40));

        edit->key(key(Code::End, true));
        press_cell(edit, 0, 0);

        const double scrolled = (40.0 * lh) - view_height();
        const auto row = static_cast<std::size_t>(std::floor(((lh / 2.0) + scrolled) / lh));

        EXPECT_EQ(probe(edit).find('|'), row * 4U);
    }

    TEST_F(TextEditTest, MovingTheCaretRightScrollsItIntoView) {
        ttk::TextEdit *edit = focused(std::string(200, 'a'));

        edit->key(key(Code::End));
        press_cell(edit, 0, 0);

        const double scrolled = (202.0 * cw) - text_width();
        const auto column = static_cast<std::size_t>(std::round((0.1 + scrolled) / cw));

        EXPECT_EQ(probe(edit).find('|'), column);
    }

    TEST_F(TextEditTest, ThumbDragScrollsWithoutTakingFocus) {
        ttk::TextEdit *edit = make(numbered_lines(40));
        const double lane = edit->box().x + edit->box().w - 5.0;
        const double from = edit->box().y + PAD_Y + 4.0;
        constexpr double MOVED = 20.0;

        controls::sweep(canvas.root(), lane, from, lane, from + MOVED);

        EXPECT_EQ(canvas.root().focused(), nullptr) << "a press on the scroll bar focused the editor";

        const double whole = 40.0 * lh;
        const double scrolled = std::min(MOVED * whole / view_height(), whole - view_height());
        const auto row = static_cast<std::size_t>(std::floor(((lh / 2.0) + scrolled) / lh));

        press_cell(edit, 0, 0);

        EXPECT_EQ(probe(edit).find('|'), row * 4U);
    }

    TEST_F(TextEditTest, ThePressOnTheLaneIsTextWhenNothingScrolls) {
        ttk::TextEdit *edit = make("short");

        controls::click(canvas.root(), edit->box().x + edit->box().w - 5.0, edit->box().y + PAD_Y + 4.0);

        EXPECT_EQ(canvas.root().focused(), edit);
    }

    TEST_F(TextEditTest, RailDragScrollsSidewaysWithoutTakingFocus) {
        ttk::TextEdit *edit = make(std::string(200, 'a'));
        const double y = edit->box().y + edit->box().h - 4.0;
        const double from = edit->box().x + PAD_X + 10.0;
        constexpr double MOVED = 30.0;

        controls::sweep(canvas.root(), from, y, from + MOVED, y);

        EXPECT_EQ(canvas.root().focused(), nullptr);

        const double room = text_width();
        const double farthest = (202.0 * cw) - room;
        const double scrolled = std::min(MOVED * (room + farthest) / room, farthest);
        const auto column = static_cast<std::size_t>(std::round((0.1 + scrolled) / cw));

        press_cell(edit, 0, 0);

        EXPECT_EQ(probe(edit).find('|'), column);
    }

    TEST_F(TextEditTest, CursorIsTheArrowOverTheBarsAndTheGutter) {
        ttk::TextEdit *edit = make(numbered_lines(40) + std::string(200, 'a'));

        edit->numbered();
        canvas.frame();

        const BLRect &box = edit->box();

        EXPECT_EQ(edit->cursor_at(box.x + box.w - 4.0, box.y + 50.0), ttk::Cursor::Default);
        EXPECT_EQ(edit->cursor_at(box.x + 4.0, box.y + 50.0), ttk::Cursor::Default);
        EXPECT_EQ(edit->cursor_at(box.x + gutter(40) + 40.0, box.y + box.h - 3.0), ttk::Cursor::Default);
        EXPECT_EQ(edit->cursor_at(box.x + gutter(40) + 40.0, box.y + 50.0), ttk::Cursor::Text);
    }

    TEST_F(TextEditTest, CursorIsTextEverywhereWhenNothingScrolls) {
        const ttk::TextEdit *edit = make("short");
        const BLRect &box = edit->box();

        EXPECT_EQ(edit->cursor_at(box.x + box.w - 4.0, box.y + 50.0), ttk::Cursor::Text);
        EXPECT_EQ(edit->cursor_at(box.x + 4.0, box.y + box.h - 3.0), ttk::Cursor::Text);
    }

    TEST_F(TextEditTest, WrappingBreaksAfterTheLastSpaceThatFits) {
        ttk::TextEdit *edit = focused("aaaa bbbb cccc dddd", fitting(10));

        edit->wrapped();
        canvas.frame();
        edit->key(key(Code::Down));

        EXPECT_EQ(probe(edit), "aaaa bbbb |cccc dddd");
    }

    TEST_F(TextEditTest, WrappingBreaksAtTheEdgeWithoutASpace) {
        ttk::TextEdit *edit = focused(std::string(15, 'a'), fitting(10));

        edit->wrapped();
        canvas.frame();
        edit->key(key(Code::Down));

        EXPECT_EQ(probe(edit).find('|'), 10U);
    }

    TEST_F(TextEditTest, WrappingCountsMultiByteCharacters) {
        std::string text;

        for (int at = 0; at < 12; ++at) {
            text += "é";
        }

        ttk::TextEdit *edit = focused(text, fitting(10));

        edit->wrapped();
        canvas.frame();
        edit->key(key(Code::Down));

        EXPECT_EQ(probe(edit).find('|'), 20U) << "the break was not after ten characters";
    }

    TEST_F(TextEditTest, WrappedHomeAndEndKeepToTheRow) {
        ttk::TextEdit *edit = focused("aaaa bbbb cccc dddd", fitting(10));

        edit->wrapped();
        canvas.frame();
        edit->key(key(Code::Down));
        type_keys(edit, Code::Right, 2);
        edit->key(key(Code::Home));

        EXPECT_EQ(probe(edit), "aaaa bbbb |cccc dddd");

        edit->set_text("aaaa bbbb cccc dddd");
        edit->key(key(Code::End));

        EXPECT_EQ(probe(edit), "aaaa bbbb |cccc dddd");
    }

    TEST_F(TextEditTest, TypingRewrapsTheLine) {
        ttk::TextEdit *edit = focused("aaaa", fitting(10));

        edit->wrapped();
        canvas.frame();
        edit->key(key(Code::End));
        edit->wrote(" bbbb cccc");
        edit->key(key(Code::Home, true));
        edit->key(key(Code::Down));

        EXPECT_EQ(probe(edit), "aaaa bbbb |cccc");
    }

    TEST_F(TextEditTest, WrapFollowsTheWidth) {
        ttk::TextEdit *edit = focused("aaaa bbbb cccc dddd", fitting(10));

        edit->wrapped();
        canvas.frame();

        auto *holder = static_cast<controls::Holder *>(edit->parent());

        holder->spot = fitting(15);
        canvas.root().relayout();
        canvas.frame();
        edit->key(key(Code::Down));

        EXPECT_EQ(probe(edit), "aaaa bbbb cccc |dddd");
    }

    TEST_F(TextEditTest, PageDownCountsWrappedRows) {
        std::string text;

        for (int at = 0; at < 20; ++at) {
            text += "aaaa bbbb ";
        }

        const BLRect spot = fitting(10);
        ttk::TextEdit *edit = focused(text, spot);
        const auto page = static_cast<std::size_t>(std::floor(view_height(spot) / lh) - 1.0);

        edit->wrapped();
        canvas.frame();
        edit->key(key(Code::PageDown));

        EXPECT_EQ(probe(edit).find('|'), page * 10U);
    }

    TEST_F(TextEditTest, NumberedGutterIsPaintedSunken) {
        ttk::TextEdit *edit = make("one\ntwo");
        const int x = static_cast<int>(edit->box().x + 4.0);
        const int y = static_cast<int>(edit->box().y + (edit->box().h / 2.0));

        canvas.full();

        EXPECT_EQ(canvas.at(x, y), ttk::Theme::palette().field.value);

        edit->numbered();
        canvas.frame();

        EXPECT_EQ(canvas.at(x, y), ttk::Theme::palette().sunken.value);
    }

    TEST_F(TextEditTest, OnlyTheFirstRowOfAWrappedLineIsNumbered) {
        ttk::TextEdit *edit = focused("aaaa bbbb cccc dddd eeee", fitting(10));

        edit->wrapped();
        edit->numbered();
        canvas.frame();

        const BLRect &box = edit->box();
        const auto gutterBox = [&](const int row) {
            return BLRectI{static_cast<int>(box.x + 2.0), static_cast<int>(box.y + PAD_Y + (row * lh) + 3.0),
                           static_cast<int>(gutter(1) - 4.0), static_cast<int>(lh - 6.0)};
        };
        const BLRgba32 sunken = ttk::Theme::palette().sunken;
        const auto inked = [&](const int row) {
            const BLRectI area = gutterBox(row);

            return (area.w * area.h) - controls::count_near(canvas, area, sunken, 0);
        };

        EXPECT_GT(inked(0), 0) << "the first row carries no number";
        EXPECT_EQ(inked(1), 0) << "a continuation row was numbered";
    }

    TEST_F(TextEditTest, PlaceholderShowsOnlyWhileEmpty) {
        ttk::TextEdit *edit = make();
        const BLRectI area{25, 25, 300, 30};

        canvas.full();

        const std::vector<std::uint32_t> bare = controls::pixels_in(canvas, area);

        edit->placeholder("Nothing here yet");
        edit->invalidate();
        canvas.frame();

        EXPECT_NE(bare, controls::pixels_in(canvas, area));

        edit->set_text("x");
        canvas.frame();

        const std::vector<std::uint32_t> typed = controls::pixels_in(canvas, area);

        edit->placeholder("");
        edit->invalidate();
        canvas.frame();

        EXPECT_EQ(typed, controls::pixels_in(canvas, area));
    }

    TEST_F(TextEditTest, FocusedEditorIsOutlinedInTheAccent) {
        ttk::TextEdit *edit = make("abc");
        const int x = static_cast<int>(edit->box().x + (edit->box().w / 2.0));
        const int y = static_cast<int>(edit->box().y);
        const ttk::Theme::Palette &palette = ttk::Theme::palette();

        canvas.full();

        EXPECT_EQ(canvas.at(x, y), palette.border.value);

        canvas.root().focus(edit);
        canvas.frame();

        EXPECT_EQ(canvas.at(x, y), palette.accent.value);
    }

    TEST_F(TextEditTest, CaretIsDrawnAtTheStartOfAnEmptyFocusedEditor) {
        const ttk::TextEdit *edit = focused();
        const int x = static_cast<int>(edit->box().x + PAD_X);
        const int y = static_cast<int>(edit->box().y + PAD_Y + (lh / 2.0));

        EXPECT_EQ(canvas.at(x, y), ttk::Theme::palette().accent.value);
    }

    TEST_F(TextEditTest, ReadOnlyEditorShowsNoCaret) {
        ttk::TextEdit *edit = make();

        edit->read_only();
        canvas.root().focus(edit);
        canvas.frame();

        const int x = static_cast<int>(edit->box().x + PAD_X);
        const int y = static_cast<int>(edit->box().y + PAD_Y + (lh / 2.0));

        EXPECT_NE(canvas.at(x, y), ttk::Theme::palette().accent.value);
    }

    TEST_F(TextEditTest, CaretRowIsTintedWhileFocused) {
        ttk::TextEdit *edit = make("ab\ncd");
        const int x = static_cast<int>(edit->box().x + 200.0);
        const auto rowY = [&](const int row) { return static_cast<int>(edit->box().y + PAD_Y + (row * lh) + 2.0); };
        const std::uint32_t field = ttk::Theme::palette().field.value;

        canvas.full();

        EXPECT_EQ(canvas.at(x, rowY(0)), field);

        canvas.root().focus(edit);
        canvas.frame();

        EXPECT_NE(canvas.at(x, rowY(0)), field) << "the caret's row is not tinted";
        EXPECT_EQ(canvas.at(x, rowY(1)), field) << "another row was tinted";
    }

    TEST_F(TextEditTest, SelectionIsTintedAndLighterWithoutFocus) {
        ttk::TextEdit *edit = focused("abcdef\nabcdef");
        const int x = static_cast<int>(edit->box().x + PAD_X + (cw * 0.5));
        const int y = static_cast<int>(edit->box().y + PAD_Y + 1.0);
        const ttk::Theme::Palette &palette = ttk::Theme::palette();

        edit->select_all();
        canvas.frame();

        const std::uint32_t focusedTint = canvas.at(x, y);

        canvas.root().focus(nullptr);
        canvas.frame();

        const std::uint32_t restingTint = canvas.at(x, y);

        edit->key(key(Code::Right));
        canvas.frame();

        EXPECT_EQ(canvas.at(x, y), palette.field.value) << "a collapsed selection is still tinted";
        EXPECT_NE(restingTint, palette.field.value) << "the selection vanished with focus";
        EXPECT_LT(controls::distance(focusedTint, palette.accent.value),
                  controls::distance(restingTint, palette.accent.value))
                << "the selection is not stronger while focused";
    }

    TEST_F(TextEditTest, ThumbIsDrawnOnlyWhenTheTextScrolls) {
        ttk::TextEdit *edit = make("short");
        const int x = static_cast<int>(edit->box().x + edit->box().w - ttk::Theme::lane + 5.0);
        const int y = static_cast<int>(edit->box().y + PAD_Y + 6.0);
        const std::uint32_t field = ttk::Theme::palette().field.value;

        canvas.full();

        EXPECT_EQ(canvas.at(x, y), field);

        edit->set_text(numbered_lines(40));
        canvas.frame();

        EXPECT_NE(canvas.at(x, y), field) << "no thumb at the top of the lane";
    }

    TEST_F(TextEditTest, ThumbMovesToTheBottomWithTheScroll) {
        ttk::TextEdit *edit = make(numbered_lines(120));
        const int x = static_cast<int>(edit->box().x + edit->box().w - ttk::Theme::lane + 5.0);
        const int top = static_cast<int>(edit->box().y + PAD_Y + 6.0);
        const int bottom = static_cast<int>(edit->box().y + PAD_Y + view_height() - 6.0);
        const std::uint32_t field = ttk::Theme::palette().field.value;

        canvas.full();

        EXPECT_NE(canvas.at(x, top), field);
        EXPECT_EQ(canvas.at(x, bottom), field);

        edit->wheel(-1000.0, ttk::Pointer{.x = 100.0, .y = 100.0});
        canvas.frame();

        EXPECT_EQ(canvas.at(x, top), field);
        EXPECT_NE(canvas.at(x, bottom), field);
    }

    TEST_F(TextEditTest, ScrollingRepaintsMatchAFullOne) {
        ttk::TextEdit *edit = focused(numbered_lines(60));

        edit->numbered();
        canvas.frame();
        edit->wheel(-2.0, ttk::Pointer{.x = 100.0, .y = 100.0});
        canvas.frame();
        edit->key(key(Code::PageDown, false, true));
        canvas.frame();
        edit->wrote("typed\n");
        canvas.frame();

        EXPECT_TRUE(canvas.matches_full());
    }

    TEST_F(TextEditTest, CogIsHiddenByDefault) {
        const ttk::TextEdit edit;

        EXPECT_TRUE(edit.children().empty());
    }

    TEST_F(TextEditTest, CogOpensAndClosesTheSettingsMenu) {
        const ttk::TextEdit *edit = make("abc", BOX, ttk::TextEdit::Cog::Shown);

        ASSERT_EQ(edit->children().size(), 1U);

        const BLRect cog = edit->children().front()->box();

        ASSERT_GT(cog.w, 0.0);

        controls::click(canvas.root(), cog.x + (cog.w / 2.0), cog.y + (cog.h / 2.0));

        EXPECT_TRUE(canvas.root().has_dismiss());
        EXPECT_EQ(canvas.root().layer(ttk::Root::POPUPS)->children().size(), 1U);

        controls::click(canvas.root(), cog.x + (cog.w / 2.0), cog.y + (cog.h / 2.0));

        EXPECT_FALSE(canvas.root().has_dismiss());
        EXPECT_TRUE(canvas.root().layer(ttk::Root::POPUPS)->children().empty());
    }

    TEST_F(TextEditTest, CogSitsInTheTopRightCorner) {
        const ttk::TextEdit *edit = make("abc", BOX, ttk::TextEdit::Cog::Shown);
        const BLRect cog = edit->children().front()->box();

        EXPECT_DOUBLE_EQ(cog.y, BOX.y + 2.0);
        EXPECT_DOUBLE_EQ(cog.x + cog.w, BOX.x + BOX.w - ttk::Theme::lane - 2.0);
    }

    TEST_F(TextEditTest, DestroyingTheEditorClosesItsMenu) {
        const ttk::TextEdit *edit = make("abc", BOX, ttk::TextEdit::Cog::Shown);
        const BLRect cog = edit->children().front()->box();

        controls::click(canvas.root(), cog.x + (cog.w / 2.0), cog.y + (cog.h / 2.0));
        ASSERT_TRUE(canvas.root().has_dismiss());

        ttk::Widget *holder = edit->parent();

        holder->erase(edit);

        EXPECT_FALSE(canvas.root().has_dismiss());
        EXPECT_TRUE(canvas.root().layer(ttk::Root::POPUPS)->children().empty());
    }

    TEST_F(TextEditTest, LongLineTypingAtTheEnd) {
        ttk::TextEdit *edit = focused(std::string(5000, 'x'));

        edit->key(key(Code::End));
        edit->wrote("!");
        edit->key(key(Code::Home));
        edit->key(key(Code::Delete));

        EXPECT_EQ(edit->text().size(), 5000U);
        EXPECT_EQ(edit->text().back(), '!');
    }
}
