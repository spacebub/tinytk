// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "Mount.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Event.h"
#include "ttk/toolkit/controls/TextBox.h"

namespace {
using controls::Calls;
using controls::key;
namespace Code = ttk::Code;

constexpr BLRect WIDE{20, 20, 300, 28};
constexpr BLRect NARROW{20, 20, 60, 28};

double advance(const std::string_view run, const bool mono = false) {
    ttk::Typeface &type = tests::fonts();

    return type.width(type.at(ttk::Typeface::pick(400, mono), ttk::Theme::fontBody), run);
}

// Writes a bar at the caret and answers the text, so the caret's offset shows in it.
std::string probe(ttk::TextBox *box) {
    box->wrote("|");

    return box->text();
}

class TextBoxTest : public controls::Painted {
protected:
    tests::Canvas canvas;
    Calls calls;

    ttk::TextBox *make(const std::string_view text = {}, const BLRect &spot = WIDE) {
        auto made = std::make_unique<ttk::TextBox>(calls.recorder());

        made->set_text(text);

        return controls::mount(canvas, std::move(made), spot);
    }

    ttk::TextBox *focused(const std::string_view text = {}, const BLRect &spot = WIDE) {
        ttk::TextBox *box = make(text, spot);

        canvas.root().focus(box);
        canvas.frame();

        return box;
    }

    void press_at(const ttk::TextBox *box, const double x, const bool shift = false) {
        controls::click(canvas.root(), x, box->box().y + (box->box().h / 2.0), shift);
    }
};

TEST_F(TextBoxTest, StartsEmpty) {
    const ttk::TextBox *box = make();

    EXPECT_TRUE(box->text().empty());
}

TEST_F(TextBoxTest, SetTextReplacesTheTextWithoutCallingEdited) {
    ttk::TextBox *box = make();

    box->set_text("hello");

    EXPECT_EQ(box->text(), "hello");
    EXPECT_TRUE(calls.texts.empty()) << "set_text reported an edit";
}

TEST_F(TextBoxTest, SetTextToTheSameTextRepaintsNothing) {
    ttk::TextBox *box = make("same");

    canvas.frame();
    ASSERT_FALSE(canvas.root().dirty());

    box->set_text("same");

    EXPECT_FALSE(canvas.root().dirty()) << "an unchanged text was repainted";
}

TEST_F(TextBoxTest, SetTextRepaints) {
    ttk::TextBox *box = make("before");

    canvas.frame();
    box->set_text("after");

    EXPECT_TRUE(canvas.root().dirty());
}

TEST_F(TextBoxTest, SetTextKeepsTheCaretWhereItWas) {
    ttk::TextBox *box = focused("hello");

    box->key(key(Code::Home));
    box->key(key(Code::Right));
    box->key(key(Code::Right));
    box->set_text("world");

    EXPECT_EQ(probe(box), "wo|rld");
}

TEST_F(TextBoxTest, SetTextClampsTheCaretToTheNewEnd) {
    ttk::TextBox *box = focused("hello");

    box->key(key(Code::End));
    box->set_text("hi");

    EXPECT_EQ(probe(box), "hi|");
}

TEST_F(TextBoxTest, SetTextClearsTheSelection) {
    ttk::TextBox *box = focused("hello");

    box->select_all();
    box->set_text("hello there");

    EXPECT_EQ(probe(box), "hello| there") << "the old selection was replaced";
}

TEST_F(TextBoxTest, WroteInsertsAtTheCaretAndReportsTheWholeText) {
    ttk::TextBox *box = focused("held");

    box->key(key(Code::Home));
    box->key(key(Code::Right));
    box->wrote("ea");

    EXPECT_EQ(box->text(), "heaeld");
    ASSERT_EQ(calls.texts.size(), 1U);
    EXPECT_EQ(calls.texts.back(), "heaeld");
}

TEST_F(TextBoxTest, WroteMovesTheCaretPastWhatItInserted) {
    ttk::TextBox *box = focused();

    box->wrote("ab");
    box->wrote("cd");

    EXPECT_EQ(box->text(), "abcd");
    EXPECT_EQ(calls.texts, (std::vector<std::string>{"ab", "abcd"}));
}

TEST_F(TextBoxTest, WroteReplacesTheSelection) {
    ttk::TextBox *box = focused("hello world");

    box->key(key(Code::Home));

    for (int step = 0; step < 5; ++step) {
        box->key(key(Code::Right, false, true));
    }

    box->wrote("bye");

    EXPECT_EQ(box->text(), "bye world");
    EXPECT_EQ(calls.texts.back(), "bye world");
}

TEST_F(TextBoxTest, WroteTakesMultiByteText) {
    ttk::TextBox *box = focused();

    box->wrote("é");
    box->wrote("й");
    box->wrote("😀");

    EXPECT_EQ(box->text(), "éй😀");
    EXPECT_EQ(probe(box), "éй😀|");
}

TEST_F(TextBoxTest, LeftAndRightMoveByOneCharacter) {
    ttk::TextBox *box = focused("abc");

    box->key(key(Code::End));
    box->key(key(Code::Left));
    box->key(key(Code::Left));
    box->key(key(Code::Right));

    EXPECT_EQ(probe(box), "ab|c");
}

TEST_F(TextBoxTest, LeftStepsOverAWholeTwoByteCharacter) {
    ttk::TextBox *box = focused("aéb");

    box->key(key(Code::End));
    box->key(key(Code::Left));
    box->key(key(Code::Left));

    EXPECT_EQ(probe(box), "a|éb");
}

TEST_F(TextBoxTest, RightStepsOverAWholeFourByteCharacter) {
    ttk::TextBox *box = focused("a😀b");

    box->key(key(Code::Home));
    box->key(key(Code::Right));
    box->key(key(Code::Right));

    EXPECT_EQ(probe(box), "a😀|b");
}

TEST_F(TextBoxTest, LeftStepsOverAWholeFourByteCharacter) {
    ttk::TextBox *box = focused("a😀b");

    box->key(key(Code::End));
    box->key(key(Code::Left));
    box->key(key(Code::Left));

    EXPECT_EQ(probe(box), "a|😀b");
}

TEST_F(TextBoxTest, ACombiningMarkIsACodePointOfItsOwn) {
    ttk::TextBox *box = focused("éx");

    box->key(key(Code::Home));
    box->key(key(Code::Right));

    EXPECT_EQ(probe(box), "e|́x") << "the caret moves by code points, not by grapheme";
}

TEST_F(TextBoxTest, LeftAtTheStartStaysThere) {
    ttk::TextBox *box = focused("abc");

    box->key(key(Code::Home));

    EXPECT_TRUE(box->key(key(Code::Left)));
    EXPECT_EQ(probe(box), "|abc");
}

TEST_F(TextBoxTest, RightAtTheEndStaysThere) {
    ttk::TextBox *box = focused("abc");

    box->key(key(Code::End));

    EXPECT_TRUE(box->key(key(Code::Right)));
    EXPECT_EQ(probe(box), "abc|");
}

TEST_F(TextBoxTest, CaretKeysDoNotCallEdited) {
    ttk::TextBox *box = focused("one two");

    for (const int code : {Code::Left, Code::Right, Code::Home, Code::End}) {
        box->key(key(code));
        box->key(key(code, true));
        box->key(key(code, false, true));
    }

    box->key(key('a', true));

    EXPECT_TRUE(calls.texts.empty());
}

TEST_F(TextBoxTest, HomeAndEndGoToEitherEnd) {
    ttk::TextBox *box = focused("middle");

    box->key(key(Code::Home));
    box->key(key(Code::Right));
    box->key(key(Code::End));

    EXPECT_EQ(probe(box), "middle|");

    box->key(key(Code::Home));

    EXPECT_EQ(probe(box), "|middle|");
}

TEST_F(TextBoxTest, CtrlRightMovesPastTheNextWord) {
    ttk::TextBox *box = focused("one two three");

    box->key(key(Code::Home));
    box->key(key(Code::Right, true));

    EXPECT_EQ(probe(box), "one| two three");
}

TEST_F(TextBoxTest, CtrlRightSkipsTheGapBeforeTheWord) {
    ttk::TextBox *box = focused("one   two three");

    box->key(key(Code::Home));
    box->key(key(Code::Right, true));
    box->key(key(Code::Right, true));

    EXPECT_EQ(probe(box), "one   two| three");
}

TEST_F(TextBoxTest, CtrlLeftMovesToTheStartOfThePreviousWord) {
    ttk::TextBox *box = focused("one two three");

    box->key(key(Code::End));
    box->key(key(Code::Left, true));

    EXPECT_EQ(probe(box), "one two |three");
}

TEST_F(TextBoxTest, CtrlLeftSkipsTrailingSpaces) {
    ttk::TextBox *box = focused("one two   ");

    box->key(key(Code::End));
    box->key(key(Code::Left, true));

    EXPECT_EQ(probe(box), "one |two   ");
}

TEST_F(TextBoxTest, WordsHoldDigitsAndUnderscores) {
    ttk::TextBox *box = focused("foo_bar2 baz");

    box->key(key(Code::Home));
    box->key(key(Code::Right, true));

    EXPECT_EQ(probe(box), "foo_bar2| baz");
}

TEST_F(TextBoxTest, PunctuationEndsAWord) {
    ttk::TextBox *box = focused("foo.bar");

    box->key(key(Code::Home));
    box->key(key(Code::Right, true));

    EXPECT_EQ(probe(box), "foo|.bar");
}

TEST_F(TextBoxTest, NonAsciiCharactersBelongToWords) {
    ttk::TextBox *box = focused("naïve café");

    box->key(key(Code::Home));
    box->key(key(Code::Right, true));

    EXPECT_EQ(probe(box), "naïve| café");
}

TEST_F(TextBoxTest, CtrlLeftOverNonAsciiWord) {
    ttk::TextBox *box = focused("naïve café");

    box->key(key(Code::End));
    box->key(key(Code::Left, true));

    EXPECT_EQ(probe(box), "naïve |café");
}

TEST_F(TextBoxTest, ShiftLeftExtendsTheSelection) {
    ttk::TextBox *box = focused("abcdef");

    box->key(key(Code::End));
    box->key(key(Code::Left, false, true));
    box->key(key(Code::Left, false, true));
    box->wrote("X");

    EXPECT_EQ(box->text(), "abcdX");
}

TEST_F(TextBoxTest, ShiftRightExtendsTheSelection) {
    ttk::TextBox *box = focused("abcdef");

    box->key(key(Code::Home));
    box->key(key(Code::Right, false, true));
    box->key(key(Code::Right, false, true));
    box->wrote("X");

    EXPECT_EQ(box->text(), "Xcdef");
}

TEST_F(TextBoxTest, ShiftHomeSelectsToTheStart) {
    ttk::TextBox *box = focused("abcdef");

    box->key(key(Code::End));
    box->key(key(Code::Left));
    box->key(key(Code::Home, false, true));
    box->wrote("X");

    EXPECT_EQ(box->text(), "Xf");
}

TEST_F(TextBoxTest, ShiftEndSelectsToTheEnd) {
    ttk::TextBox *box = focused("abcdef");

    box->key(key(Code::Home));
    box->key(key(Code::Right));
    box->key(key(Code::End, false, true));
    box->wrote("X");

    EXPECT_EQ(box->text(), "aX");
}

TEST_F(TextBoxTest, CtrlShiftLeftSelectsAWord) {
    ttk::TextBox *box = focused("one two");

    box->key(key(Code::End));
    box->key(key(Code::Left, true, true));
    box->wrote("X");

    EXPECT_EQ(box->text(), "one X");
}

TEST_F(TextBoxTest, ShiftBackOverTheAnchorFlipsTheSelection) {
    ttk::TextBox *box = focused("abcdef");

    box->key(key(Code::Home));
    box->key(key(Code::Right));
    box->key(key(Code::Right));
    box->key(key(Code::Right, false, true));
    box->key(key(Code::Left, false, true));
    box->key(key(Code::Left, false, true));
    box->wrote("X");

    EXPECT_EQ(box->text(), "aXcdef");
}

TEST_F(TextBoxTest, LeftCollapsesASelectionToItsStart) {
    ttk::TextBox *box = focused("abcdef");

    box->select_all();
    box->key(key(Code::Left));

    EXPECT_EQ(probe(box), "|abcdef");
}

TEST_F(TextBoxTest, RightCollapsesASelectionToItsEnd) {
    ttk::TextBox *box = focused("abcdef");

    box->key(key(Code::Home));
    box->key(key(Code::Right, false, true));
    box->key(key(Code::Right, false, true));
    box->key(key(Code::Right));

    EXPECT_EQ(probe(box), "ab|cdef") << "Right moved past the selection instead of to its end";
}

TEST_F(TextBoxTest, CtrlASelectsAllWithTheCaretAtTheEnd) {
    ttk::TextBox *box = focused("abcdef");

    box->key(key(Code::Home));

    EXPECT_TRUE(box->key(key('a', true)));

    box->key(key(Code::Left, false, true));
    box->wrote("X");

    EXPECT_EQ(box->text(), "Xf") << "the caret of Ctrl+A was not at the end";
}

TEST_F(TextBoxTest, SelectAllThenTypingReplacesEverything) {
    ttk::TextBox *box = focused("abcdef");

    box->select_all();
    box->wrote("new");

    EXPECT_EQ(box->text(), "new");
}

TEST_F(TextBoxTest, CtrlAOnAnEmptyFieldThenTyping) {
    ttk::TextBox *box = focused();

    box->key(key('a', true));
    box->wrote("x");

    EXPECT_EQ(box->text(), "x");
}

TEST_F(TextBoxTest, BackspaceRemovesTheCharacterBeforeTheCaret) {
    ttk::TextBox *box = focused("abc");

    box->key(key(Code::End));
    box->key(key(Code::Left));

    EXPECT_TRUE(box->key(key(Code::Backspace)));
    EXPECT_EQ(box->text(), "ac");
    EXPECT_EQ(calls.texts, (std::vector<std::string>{"ac"}));
    EXPECT_EQ(probe(box), "a|c");
}

TEST_F(TextBoxTest, BackspaceRemovesAWholeTwoByteCharacter) {
    ttk::TextBox *box = focused("aé");

    box->key(key(Code::End));
    box->key(key(Code::Backspace));

    EXPECT_EQ(box->text(), "a");
}

TEST_F(TextBoxTest, BackspaceRemovesAWholeFourByteCharacter) {
    ttk::TextBox *box = focused("a😀");

    box->key(key(Code::End));
    box->key(key(Code::Backspace));

    EXPECT_EQ(box->text(), "a");
}

TEST_F(TextBoxTest, BackspaceRemovesOnlyTheCombiningMark) {
    ttk::TextBox *box = focused("é");

    box->key(key(Code::End));
    box->key(key(Code::Backspace));

    EXPECT_EQ(box->text(), "e");
}

TEST_F(TextBoxTest, BackspaceAtTheStartDoesNothing) {
    ttk::TextBox *box = focused("abc");

    box->key(key(Code::Home));

    EXPECT_TRUE(box->key(key(Code::Backspace)));
    EXPECT_EQ(box->text(), "abc");
    EXPECT_TRUE(calls.texts.empty()) << "a Backspace that removed nothing reported an edit";
}

TEST_F(TextBoxTest, BackspaceOnAnEmptyFieldDoesNothing) {
    ttk::TextBox *box = focused();

    EXPECT_TRUE(box->key(key(Code::Backspace)));
    EXPECT_TRUE(box->key(key(Code::Backspace, true)));
    EXPECT_TRUE(box->text().empty());
    EXPECT_TRUE(calls.texts.empty());
}

TEST_F(TextBoxTest, BackspaceOfTheOnlyCharacterReportsAnEmptyText) {
    ttk::TextBox *box = focused("a");

    box->key(key(Code::End));
    box->key(key(Code::Backspace));

    EXPECT_TRUE(box->text().empty());
    EXPECT_EQ(calls.texts, (std::vector<std::string>{""}));
}

TEST_F(TextBoxTest, BackspaceRemovesTheSelectionAlone) {
    ttk::TextBox *box = focused("abcdef");

    box->key(key(Code::Home));
    box->key(key(Code::Right, false, true));
    box->key(key(Code::Right, false, true));
    box->key(key(Code::Backspace));

    EXPECT_EQ(box->text(), "cdef");
    EXPECT_EQ(probe(box), "|cdef");
}

TEST_F(TextBoxTest, CtrlBackspaceRemovesTheWordBeforeTheCaret) {
    ttk::TextBox *box = focused("one two");

    box->key(key(Code::End));
    box->key(key(Code::Backspace, true));

    EXPECT_EQ(box->text(), "one ");
}

TEST_F(TextBoxTest, CtrlBackspaceTakesTheSpacesBeforeTheWordToo) {
    ttk::TextBox *box = focused("one two   ");

    box->key(key(Code::End));
    box->key(key(Code::Backspace, true));

    EXPECT_EQ(box->text(), "one ");
}

TEST_F(TextBoxTest, CtrlBackspaceRemovesANonAsciiWord) {
    ttk::TextBox *box = focused("big café");

    box->key(key(Code::End));
    box->key(key(Code::Backspace, true));

    EXPECT_EQ(box->text(), "big ");
}

TEST_F(TextBoxTest, DeleteRemovesTheCharacterAfterTheCaret) {
    ttk::TextBox *box = focused("abc");

    box->key(key(Code::Home));

    EXPECT_TRUE(box->key(key(Code::Delete)));
    EXPECT_EQ(box->text(), "bc");
    EXPECT_EQ(calls.texts, (std::vector<std::string>{"bc"}));
    EXPECT_EQ(probe(box), "|bc");
}

TEST_F(TextBoxTest, DeleteRemovesAWholeMultiByteCharacter) {
    ttk::TextBox *box = focused("é😀");

    box->key(key(Code::Home));
    box->key(key(Code::Delete));

    EXPECT_EQ(box->text(), "😀");

    box->key(key(Code::Delete));

    EXPECT_TRUE(box->text().empty());
}

TEST_F(TextBoxTest, DeleteAtTheEndDoesNothing) {
    ttk::TextBox *box = focused("abc");

    box->key(key(Code::End));

    EXPECT_TRUE(box->key(key(Code::Delete)));
    EXPECT_TRUE(box->key(key(Code::Delete, true)));
    EXPECT_EQ(box->text(), "abc");
    EXPECT_TRUE(calls.texts.empty());
}

TEST_F(TextBoxTest, DeleteRemovesTheSelectionAlone) {
    ttk::TextBox *box = focused("abcdef");

    box->key(key(Code::End));
    box->key(key(Code::Left, false, true));
    box->key(key(Code::Delete));

    EXPECT_EQ(box->text(), "abcde");
}

TEST_F(TextBoxTest, CtrlDeleteRemovesTheWordAfterTheCaret) {
    ttk::TextBox *box = focused("one two");

    box->key(key(Code::Home));
    box->key(key(Code::Delete, true));

    EXPECT_EQ(box->text(), " two");
}

TEST_F(TextBoxTest, CtrlDeleteTakesTheGapBeforeTheWord) {
    ttk::TextBox *box = focused("one   two");

    box->key(key(Code::Home));
    box->key(key(Code::Right, true));
    box->key(key(Code::Delete, true));

    EXPECT_EQ(box->text(), "one");
}

TEST_F(TextBoxTest, CtrlCIsConsumedAndChangesNothing) {
    ttk::TextBox *box = focused("copy me");

    box->select_all();

    EXPECT_TRUE(box->key(key('c', true)));
    EXPECT_EQ(box->text(), "copy me");
    EXPECT_TRUE(calls.texts.empty());
}

TEST_F(TextBoxTest, CtrlXRemovesTheSelection) {
    ttk::TextBox *box = focused("cut this");

    box->key(key(Code::End));
    box->key(key(Code::Left, true, true));

    EXPECT_TRUE(box->key(key('x', true)));
    EXPECT_EQ(box->text(), "cut ");
    EXPECT_EQ(calls.texts, (std::vector<std::string>{"cut "}));
}

TEST_F(TextBoxTest, CtrlXWithoutASelectionChangesNothing) {
    ttk::TextBox *box = focused("keep");

    EXPECT_TRUE(box->key(key('x', true)));
    EXPECT_EQ(box->text(), "keep");
    EXPECT_TRUE(calls.texts.empty());
}

TEST_F(TextBoxTest, CtrlVIsConsumed) {
    ttk::TextBox *box = focused("paste");

    EXPECT_TRUE(box->key(key('v', true)));
}

TEST_F(TextBoxTest, ReturnCallsAccepted) {
    ttk::TextBox *box = focused("go");
    int accepted = 0;

    box->accepted = [&] { ++accepted; };

    EXPECT_TRUE(box->key(key(Code::Return)));
    EXPECT_EQ(accepted, 1);
    EXPECT_EQ(box->text(), "go");
}

TEST_F(TextBoxTest, ReturnIsConsumedWithoutAccepted) {
    ttk::TextBox *box = focused("go");

    EXPECT_TRUE(box->key(key(Code::Return)));
}

TEST_F(TextBoxTest, ReturnThroughTheRootReachesAccepted) {
    ttk::TextBox *box = focused("go");
    int accepted = 0;

    box->accepted = [&] { ++accepted; };

    EXPECT_TRUE(canvas.root().key(key(Code::Return)));
    EXPECT_EQ(accepted, 1) << "the root's Return fallback fired as well, or not at all";
}

TEST_F(TextBoxTest, EscapeCallsCancelled) {
    ttk::TextBox *box = focused("stop");
    int cancelled = 0;

    box->cancelled = [&] { ++cancelled; };

    EXPECT_TRUE(box->key(key(Code::Escape)));
    EXPECT_EQ(cancelled, 1);
}

TEST_F(TextBoxTest, EscapeWithoutCancelledPassesOn) {
    ttk::TextBox *box = focused("stop");

    EXPECT_FALSE(box->key(key(Code::Escape)));
}

TEST_F(TextBoxTest, OtherKeysAreNotConsumed) {
    ttk::TextBox *box = focused("abc");

    EXPECT_FALSE(box->key(key('q')));
    EXPECT_FALSE(box->key(key(Code::Up)));
    EXPECT_FALSE(box->key(key(Code::Down)));
    EXPECT_FALSE(box->key(key(Code::PageUp)));
    EXPECT_FALSE(box->key(key(Code::F1)));
    EXPECT_FALSE(box->key(key(Code::Tab)));
    EXPECT_FALSE(box->key(key('z', true)));
    EXPECT_EQ(box->text(), "abc");
}

TEST_F(TextBoxTest, TabLeavesTheField) {
    ttk::TextBox *first = make("first");
    ttk::TextBox *second = make("second");

    canvas.root().focus(first);

    EXPECT_TRUE(canvas.root().key(key(Code::Tab)));
    EXPECT_EQ(canvas.root().focused(), second);
    EXPECT_EQ(first->text(), "first") << "Tab was typed into the field";

    canvas.root().key(ttk::Key{.code = Code::Tab, .shift = true});

    EXPECT_EQ(canvas.root().focused(), first);
}

TEST_F(TextBoxTest, ReadOnlyIgnoresTyping) {
    ttk::TextBox *box = focused("fixed");

    box->read_only();
    box->wrote("x");

    EXPECT_EQ(box->text(), "fixed");
    EXPECT_TRUE(calls.texts.empty());
}

TEST_F(TextBoxTest, ReadOnlyIgnoresDeleting) {
    ttk::TextBox *box = focused("fixed");

    box->read_only();
    box->key(key(Code::End));
    box->key(key(Code::Backspace));
    box->key(key(Code::Backspace, true));
    box->key(key(Code::Home));
    box->key(key(Code::Delete));
    box->key(key(Code::Delete, true));
    box->select_all();
    box->key(key(Code::Backspace));

    EXPECT_EQ(box->text(), "fixed");
    EXPECT_TRUE(calls.texts.empty());
}

TEST_F(TextBoxTest, ReadOnlyIgnoresCutAndPaste) {
    ttk::TextBox *box = focused("fixed");

    box->read_only();
    box->select_all();

    EXPECT_TRUE(box->key(key('x', true)));
    EXPECT_TRUE(box->key(key('v', true)));
    EXPECT_EQ(box->text(), "fixed");
    EXPECT_TRUE(calls.texts.empty());
}

TEST_F(TextBoxTest, ReadOnlyStillMovesTheCaret) {
    ttk::TextBox *box = focused("fixed");

    box->read_only();
    box->key(key(Code::Home));
    box->key(key(Code::Right));
    box->key(key(Code::Right));
    box->read_only(false);

    EXPECT_EQ(probe(box), "fi|xed");
}

TEST_F(TextBoxTest, ReadOnlyStillSelects) {
    ttk::TextBox *box = focused("fixed");

    box->read_only();
    box->key(key(Code::Home));
    box->key(key(Code::Right, false, true));
    box->read_only(false);
    box->wrote("F");

    EXPECT_EQ(box->text(), "Fixed");
}

TEST_F(TextBoxTest, ReadOnlyStillTakesFocus) {
    ttk::TextBox *box = make("fixed");

    box->read_only();
    press_at(box, box->box().x + 5.0);

    EXPECT_EQ(canvas.root().focused(), box);
}

TEST_F(TextBoxTest, SetterChainsReturnTheField) {
    ttk::TextBox *box = make();

    EXPECT_EQ(box->placeholder("p"), box);
    EXPECT_EQ(box->mono(), box);
    EXPECT_EQ(box->read_only(false), box);
    EXPECT_EQ(box->secret(false), box);
}

TEST_F(TextBoxTest, SecretKeepsTheRealText) {
    ttk::TextBox *box = focused();

    box->secret();
    box->wrote("hunter2");

    EXPECT_EQ(box->text(), "hunter2");
    EXPECT_EQ(calls.texts.back(), "hunter2");
}

TEST_F(TextBoxTest, SecretDrawsOneBulletPerCharacter) {
    ttk::TextBox *box = make("iiii");
    const BLRectI area{20, 20, 300, 28};

    box->secret();
    canvas.full();

    const std::vector<std::uint32_t> narrow = controls::pixels_in(canvas, area);

    box->set_text("WWWW");
    canvas.full();

    EXPECT_EQ(narrow, controls::pixels_in(canvas, area)) << "secret text of equal length painted differently";

    box->secret(false);
    canvas.full();

    EXPECT_NE(narrow, controls::pixels_in(canvas, area)) << "turning secret off did not show the text";
}

TEST_F(TextBoxTest, SecretCountsMultiByteCharactersOnce) {
    ttk::TextBox *box = make("aé😀");
    const BLRectI area{20, 20, 300, 28};

    box->secret();
    canvas.full();

    const std::vector<std::uint32_t> wide = controls::pixels_in(canvas, area);

    box->set_text("abc");
    canvas.full();

    EXPECT_EQ(wide, controls::pixels_in(canvas, area)) << "a multi-byte character drew more than one bullet";
}

TEST_F(TextBoxTest, NaturalWidthIs120UnlessFixed) {
    ttk::TextBox box(nullptr);

    EXPECT_DOUBLE_EQ(box.natural_width(tests::fonts()), 120.0);

    box.fixedWidth = 77.0;

    EXPECT_DOUBLE_EQ(box.natural_width(tests::fonts()), 77.0);
}

TEST_F(TextBoxTest, NaturalHeightIsTheBodyLineHeightUnlessFixed) {
    ttk::TextBox box(nullptr);
    ttk::Typeface &type = tests::fonts();

    EXPECT_DOUBLE_EQ(box.natural_height(type, 100.0), type.line_height(type.at(400, ttk::Theme::fontBody)));

    box.fixedHeight = 33.0;

    EXPECT_DOUBLE_EQ(box.natural_height(type, 100.0), 33.0);
}

TEST_F(TextBoxTest, PaintsItsOwnFocus) {
    const ttk::TextBox box(nullptr);

    EXPECT_TRUE(box.paints_focus());
    EXPECT_TRUE(box.takes_focus());
}

TEST_F(TextBoxTest, PressFocusesAndPlacesTheCaretAtTheNearestBoundary) {
    ttk::TextBox *box = make("abcdef");

    press_at(box, box->box().x + advance("abc") + 0.4);

    EXPECT_EQ(canvas.root().focused(), box);
    EXPECT_EQ(probe(box), "abc|def");
}

TEST_F(TextBoxTest, PressBeforeTheTextGoesToTheStart) {
    ttk::TextBox *box = make("abcdef");

    press_at(box, box->box().x + 0.5);

    EXPECT_EQ(probe(box), "|abcdef");
}

TEST_F(TextBoxTest, PressPastTheTextGoesToTheEnd) {
    ttk::TextBox *box = make("abc");

    press_at(box, box->box().x + box->box().w - 2.0);

    EXPECT_EQ(probe(box), "abc|");
}

TEST_F(TextBoxTest, PressLandsOnAMultiByteBoundary) {
    ttk::TextBox *box = make("éйé");

    press_at(box, box->box().x + advance("éй") + 0.2);

    EXPECT_EQ(probe(box), "éй|é");
}

TEST_F(TextBoxTest, ShiftPressExtendsTheSelection) {
    ttk::TextBox *box = focused("abcdef");

    box->key(key(Code::Home));
    press_at(box, box->box().x + advance("abc") + 0.2, true);
    box->wrote("X");

    EXPECT_EQ(box->text(), "Xdef");
}

TEST_F(TextBoxTest, DragSelectsFromThePress) {
    ttk::TextBox *box = make("abcdef");
    const double y = box->box().y + 10.0;

    controls::sweep(canvas.root(), box->box().x + advance("a") + 0.2, y, box->box().x + advance("abcd") + 0.2, y);
    box->wrote("X");

    EXPECT_EQ(box->text(), "aXef");
}

TEST_F(TextBoxTest, DragBackwardsSelectsToo) {
    ttk::TextBox *box = make("abcdef");
    const double y = box->box().y + 10.0;

    controls::sweep(canvas.root(), box->box().x + advance("abcd") + 0.2, y, box->box().x + advance("a") + 0.2, y);
    box->wrote("X");

    EXPECT_EQ(box->text(), "aXef");
}

TEST_F(TextBoxTest, DragOutsideTheBoxClampsToTheEnds) {
    ttk::TextBox *box = make("abcdef");
    const double y = box->box().y + 10.0;

    controls::sweep(canvas.root(), box->box().x + advance("ab") + 0.2, y, box->box().x + 380.0, y);
    box->wrote("X");

    EXPECT_EQ(box->text(), "abX");
}

TEST_F(TextBoxTest, DisabledFieldRefusesThePress) {
    ttk::TextBox *box = make("off");

    box->set_enabled(false);
    canvas.frame();

    EXPECT_FALSE(box->press(ttk::Pointer{.x = box->box().x + 4.0, .y = box->box().y + 4.0}));

    press_at(box, box->box().x + 4.0);

    EXPECT_NE(canvas.root().focused(), box);
}

TEST_F(TextBoxTest, MonoMeasuresInTheFixedWidthFace) {
    auto made = std::make_unique<ttk::TextBox>(nullptr);

    made->mono();
    made->set_text("iiiiWWWW");

    ttk::TextBox *box = controls::mount(canvas, std::move(made), WIDE);

    ASSERT_GT(std::abs(advance("iiii", true) - advance("iiii")), 4.0) << "the faces measure alike";

    press_at(box, box->box().x + advance("iiii", true) + 0.2);

    EXPECT_EQ(probe(box), "iiii|WWWW");
}

TEST_F(TextBoxTest, MonoChangesThePainting) {
    ttk::TextBox *box = make("iiiiWWWW");
    const BLRectI area{20, 20, 300, 28};

    canvas.full();

    const std::vector<std::uint32_t> proportional = controls::pixels_in(canvas, area);

    box->mono();
    canvas.full();

    EXPECT_NE(proportional, controls::pixels_in(canvas, area));
}

TEST_F(TextBoxTest, CaretScrollsIntoViewInANarrowField) {
    const std::string text = "abcdefghijklmnopqrstuvwxyz0123456789";
    ttk::TextBox *box = focused(text, NARROW);
    const double room = NARROW.w - 2.0;

    ASSERT_GT(advance(text), room * 2.0) << "the text fits the narrow field";

    box->key(key(Code::End));
    canvas.frame();

    const int caretX = static_cast<int>(NARROW.x + room);
    const int middle = static_cast<int>(NARROW.y + (NARROW.h / 2.0));

    EXPECT_EQ(canvas.at(caretX, middle), ttk::Theme::palette().accent.value)
            << "the caret at the end is not drawn at the right edge of the field";

    const double wanted = advance(text) - room + 1.0;
    std::size_t nearest = 0;

    for (std::size_t at = 0; at <= text.size(); ++at) {
        if (std::abs(advance(text.substr(0, at)) - wanted) < std::abs(advance(text.substr(0, nearest)) - wanted)) {
            nearest = at;
        }
    }

    press_at(box, NARROW.x + 1.0);

    EXPECT_GT(nearest, 0U);
    EXPECT_EQ(probe(box).find('|'), nearest) << "the press ignored the sideways scroll";
}

TEST_F(TextBoxTest, HomeScrollsBackToTheStart) {
    ttk::TextBox *box = focused("abcdefghijklmnopqrstuvwxyz0123456789", NARROW);

    box->key(key(Code::End));
    box->key(key(Code::Home));
    box->key(key(Code::End, false, true));
    box->key(key(Code::Home, false, true));
    press_at(box, NARROW.x + 0.5);

    EXPECT_EQ(probe(box).find('|'), 0U);
}

TEST_F(TextBoxTest, LosingFocusScrollsBackToTheStart) {
    ttk::TextBox *other = make("other");
    ttk::TextBox *box = focused("abcdefghijklmnopqrstuvwxyz0123456789", NARROW);

    box->key(key(Code::End));
    canvas.root().focus(other);
    canvas.root().focus(box);
    press_at(box, NARROW.x + 0.5);

    EXPECT_EQ(probe(box).find('|'), 0U) << "the field kept its sideways scroll after losing focus";
}

TEST_F(TextBoxTest, LosingFocusClearsTheSelection) {
    ttk::TextBox *other = make("other");
    ttk::TextBox *box = focused("abc");

    box->select_all();
    canvas.root().focus(other);
    canvas.root().focus(box);

    EXPECT_EQ(probe(box), "abc|");
}

TEST_F(TextBoxTest, GainingFocusWakesTheCaret) {
    ttk::TextBox *box = make("abc");

    for (int step = 0; step < 4; ++step) {
        canvas.root().advance(0.0);
    }

    ASSERT_FALSE(canvas.root().busy());

    canvas.root().focus(box);

    EXPECT_TRUE(canvas.root().busy());
}

TEST_F(TextBoxTest, AdvanceStopsWithoutFocus) {
    ttk::TextBox *box = make("abc");

    EXPECT_FALSE(box->advance(1.0));
}

TEST_F(TextBoxTest, AdvanceSleepsBetweenBlinks) {
    ttk::TextBox *box = focused("abc");

    EXPECT_FALSE(box->advance(0.1)) << "a caret between blinks kept the frame loop running";
    EXPECT_GT(canvas.root().waking(), 0.1);
}

TEST_F(TextBoxTest, LongTextMovesByWordsToEitherEnd) {
    std::string text;

    for (int at = 0; at < 400; ++at) {
        text += "word ";
    }

    ttk::TextBox *box = focused(text);

    box->key(key(Code::End));
    box->key(key(Code::Left, true));
    box->key(key(Code::Backspace, true));

    EXPECT_EQ(box->text().size(), text.size() - 5U);

    box->key(key(Code::Home));
    box->key(key(Code::Delete, true));

    EXPECT_EQ(box->text().size(), text.size() - 9U);
    EXPECT_EQ(box->text().substr(0, 6), " word ");
}

TEST_F(TextBoxTest, FocusedEmptyFieldDrawsItsCaretAtTheLeftEdge) {
    const ttk::TextBox *box = focused();
    const int x = static_cast<int>(box->box().x);
    const int y = static_cast<int>(box->box().y + 14.0);

    EXPECT_EQ(canvas.at(x, y), ttk::Theme::palette().accent.value);
}

TEST_F(TextBoxTest, UnfocusedEmptyFieldDrawsNothing) {
    const ttk::TextBox *box = make();
    const BLRectI area{static_cast<int>(box->box().x), static_cast<int>(box->box().y), 300, 28};

    canvas.full();

    EXPECT_EQ(controls::count_near(canvas, area, ttk::Theme::palette().background, 0), 300 * 28);
}

TEST_F(TextBoxTest, CaretIsDrawnWhereItStands) {
    ttk::TextBox *box = focused("abcdef");

    box->key(key(Code::Home));
    canvas.frame();

    EXPECT_EQ(canvas.at(static_cast<int>(box->box().x), static_cast<int>(box->box().y + 14.0)),
              ttk::Theme::palette().accent.value);

    box->key(key(Code::End));
    canvas.frame();

    const double x = box->box().x + advance("abcdef");

    EXPECT_TRUE(controls::leans(canvas.at(static_cast<int>(std::floor(x)) + 1, static_cast<int>(box->box().y + 4.0)),
                                ttk::Theme::palette().accent,
                                ttk::Theme::palette().background))
            << "no caret after the last character";
    EXPECT_NE(canvas.at(static_cast<int>(box->box().x), static_cast<int>(box->box().y + 4.0)),
              ttk::Theme::palette().accent.value)
            << "the caret was left behind at the start";
}

TEST_F(TextBoxTest, FocusedSelectionIsFilledWithTheAccent) {
    ttk::TextBox *box = focused("hello world");

    box->select_all();
    canvas.frame();

    const int y = static_cast<int>(box->box().y + 3.0);
    const auto at = [&](const double x) { return canvas.at(static_cast<int>(box->box().x + x), y); };

    EXPECT_EQ(at(2.0), ttk::Theme::palette().accent.value);
    EXPECT_EQ(at(advance("hello world") - 2.0), ttk::Theme::palette().accent.value);
    EXPECT_EQ(at(advance("hello world") + 4.0), ttk::Theme::palette().background.value)
            << "the selection fill ran past the text";
}

TEST_F(TextBoxTest, PartialSelectionFillsOnlyItsSpan) {
    ttk::TextBox *box = focused("hello world");

    box->key(key(Code::End));
    box->key(key(Code::Left, true, true));
    canvas.frame();

    const int y = static_cast<int>(box->box().y + 3.0);
    const auto at = [&](const double x) { return canvas.at(static_cast<int>(box->box().x + x), y); };

    EXPECT_EQ(at(2.0), ttk::Theme::palette().background.value) << "the unselected head was filled";
    EXPECT_EQ(at(advance("hello w") + 1.0), ttk::Theme::palette().accent.value);
}

TEST_F(TextBoxTest, UnfocusedSelectionIsNotPainted) {
    ttk::TextBox *box = make("hello world");

    box->select_all();
    canvas.full();

    EXPECT_EQ(canvas.at(static_cast<int>(box->box().x + 2.0), static_cast<int>(box->box().y + 3.0)),
              ttk::Theme::palette().background.value);
}

TEST_F(TextBoxTest, PlaceholderShowsOnlyWhileEmpty) {
    ttk::TextBox *box = make();
    const BLRectI area{20, 20, 300, 28};

    canvas.full();

    const std::vector<std::uint32_t> bare = controls::pixels_in(canvas, area);

    box->placeholder("Type here");
    box->invalidate();
    canvas.frame();

    const std::vector<std::uint32_t> hinted = controls::pixels_in(canvas, area);

    EXPECT_NE(bare, hinted) << "the placeholder was not drawn";
    EXPECT_GT(controls::count_near(canvas, area, ttk::Theme::palette().faint, 60), 0)
            << "the placeholder is not in the faint colour";

    box->set_text("x");
    canvas.frame();

    const std::vector<std::uint32_t> typed = controls::pixels_in(canvas, area);

    box->placeholder("");
    box->invalidate();
    canvas.frame();

    EXPECT_EQ(typed, controls::pixels_in(canvas, area)) << "the placeholder showed under text";
}

TEST_F(TextBoxTest, PartialRepaintsMatchAFullOne) {
    ttk::TextBox *box = focused("hello world", NARROW);

    box->key(key(Code::End));
    canvas.frame();
    box->key(key(Code::Left, true, true));
    canvas.frame();
    box->wrote("é");
    canvas.frame();

    EXPECT_TRUE(canvas.matches_full());
}
}
