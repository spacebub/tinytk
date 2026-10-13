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
#include "ttk/toolkit/controls/TextView.h"

namespace {
using controls::key;
namespace Code = ttk::Code;

constexpr BLRect BOX{20, 20, 360, 200};

class TextViewTest : public controls::Painted {
protected:
    tests::Canvas canvas;
    double rh = 0.0;

    void SetUp() override {
        Painted::SetUp();

        if (IsSkipped()) {
            return;
        }

        ttk::Typeface &type = tests::fonts();

        rh = type.line_height(type.at(ttk::Typeface::mono, ttk::Theme::fontSmall));
    }

    ttk::TextView *make(std::vector<std::string> rows = {}, const BLRect &spot = BOX) {
        auto made = std::make_unique<ttk::TextView>();

        made->set_rows(std::move(rows));

        return controls::mount(canvas, std::move(made), spot);
    }

    ttk::TextView *run(const std::string_view text, const BLRect &spot = BOX) {
        auto made = std::make_unique<ttk::TextView>();

        made->set_run(text);

        return controls::mount(canvas, std::move(made), spot);
    }

    [[nodiscard]] static double advance(const std::string_view text) {
        ttk::Typeface &type = tests::fonts();

        return type.width(type.at(ttk::Typeface::mono, ttk::Theme::fontSmall), text);
    }

    [[nodiscard]] BLPoint at(const ttk::TextView *view, const std::size_t row, const std::string_view before) const {
        return BLPoint{view->box().x + advance(before) + 0.2,
                       view->box().y + (static_cast<double>(row) * rh) + (rh / 2.0)};
    }

    void select(const BLPoint from, const BLPoint to) { controls::sweep(canvas.root(), from.x, from.y, to.x, to.y); }

    void settle_frame() {
        canvas.frame();
        ASSERT_FALSE(canvas.root().dirty());
    }
};

TEST_F(TextViewTest, StartsEmpty) {
    const ttk::TextView view;

    EXPECT_EQ(view.rows(), 0U);
    EXPECT_TRUE(view.selection().empty());
}

TEST_F(TextViewTest, SetRowsReplacesTheRows) {
    ttk::TextView *view = make({"one", "two"});

    view->set_rows({"three", "four", "five"});

    EXPECT_EQ(view->rows(), 3U);

    view->select_all();

    EXPECT_EQ(view->selection(), "three\nfour\nfive");
}

TEST_F(TextViewTest, SetRowsWithTheSameRowsRepaintsNothing) {
    ttk::TextView *view = make({"same", "rows"});

    settle_frame();
    view->set_rows({"same", "rows"});

    EXPECT_FALSE(canvas.root().dirty());
}

TEST_F(TextViewTest, SetRowsKeepsTheSelectionClamped) {
    ttk::TextView *view = make({"abcdef", "ghij"});

    view->select_all();
    view->set_rows({"ab"});

    EXPECT_EQ(view->selection(), "ab");
}

TEST_F(TextViewTest, SetRowsDiscardsARun) {
    ttk::TextView *view = run("a run of words");

    view->set_rows({"row"});
    view->select_all();

    EXPECT_EQ(view->rows(), 1U);
    EXPECT_EQ(view->selection(), "row");
}

TEST_F(TextViewTest, AddRowsAppends) {
    ttk::TextView *view = make({"one"});

    view->add_rows({"two", "three"});
    view->select_all();

    EXPECT_EQ(view->rows(), 3U);
    EXPECT_EQ(view->selection(), "one\ntwo\nthree");
}

TEST_F(TextViewTest, AddRowsWithNothingDoesNothing) {
    ttk::TextView *view = make({"one"});

    settle_frame();
    view->add_rows({});

    EXPECT_EQ(view->rows(), 1U);
    EXPECT_FALSE(canvas.root().dirty());
}

TEST_F(TextViewTest, AddRowsClosesAnOpenRow) {
    ttk::TextView *view = make();

    view->append("par");
    view->add_rows({"x"});
    view->append("y");
    view->select_all();

    EXPECT_EQ(view->rows(), 3U);
    EXPECT_EQ(view->selection(), "par\nx\ny");
}

TEST_F(TextViewTest, AddRowsDiscardsARun) {
    ttk::TextView *view = run("a run of words");

    view->add_rows({"row"});

    EXPECT_EQ(view->rows(), 1U);
}

TEST_F(TextViewTest, AppendSplitsAtLineBreaks) {
    ttk::TextView *view = make();

    view->append("a\nb\nc");

    EXPECT_EQ(view->rows(), 3U);
}

TEST_F(TextViewTest, AppendContinuesTheOpenRow) {
    ttk::TextView *view = make();

    view->append("a\nb\nc");
    view->append("d\ne");
    view->select_all();

    EXPECT_EQ(view->selection(), "a\nb\ncd\ne");
}

TEST_F(TextViewTest, AppendClosesTheRowAtATrailingBreak) {
    ttk::TextView *view = make();

    view->append("a\n");

    EXPECT_EQ(view->rows(), 1U);

    view->append("b");

    EXPECT_EQ(view->rows(), 2U);
}

TEST_F(TextViewTest, AppendTreatsCarriageReturnLineFeedAsALineBreak) {
    ttk::TextView *view = make();

    view->append("a\r\nb\r\n");
    view->select_all();

    EXPECT_EQ(view->rows(), 2U);
    EXPECT_EQ(view->selection(), "a\nb");
}

TEST_F(TextViewTest, ALoneCarriageReturnRestartsTheRow) {
    ttk::TextView *view = make();

    view->append("progress 10%\rprogress 50%");
    view->select_all();

    EXPECT_EQ(view->rows(), 1U);
    EXPECT_EQ(view->selection(), "progress 50%");
}

TEST_F(TextViewTest, ACarriageReturnInALaterCallRestartsTheOpenRow) {
    ttk::TextView *view = make();

    view->append("abc");
    view->append("\rxy\ndone");
    view->select_all();

    EXPECT_EQ(view->selection(), "xy\ndone");
}

TEST_F(TextViewTest, AppendNothingDoesNothing) {
    ttk::TextView *view = make({"one"});

    settle_frame();
    view->append("");

    EXPECT_EQ(view->rows(), 1U);
    EXPECT_FALSE(canvas.root().dirty());
}

TEST_F(TextViewTest, AppendDiscardsARun) {
    ttk::TextView *view = run("a run of words");

    view->append("out");
    view->select_all();

    EXPECT_EQ(view->selection(), "out");
}

TEST_F(TextViewTest, AppendRepaints) {
    ttk::TextView *view = make({"one"});

    settle_frame();
    view->append("x");

    EXPECT_TRUE(canvas.root().dirty());
}

TEST_F(TextViewTest, DropRowsRemovesFromTheTop) {
    ttk::TextView *view = make({"a", "b", "c", "d"});

    view->drop_rows(2);
    view->select_all();

    EXPECT_EQ(view->rows(), 2U);
    EXPECT_EQ(view->selection(), "c\nd");
}

TEST_F(TextViewTest, DropRowsBeyondTheEndRemovesAll) {
    ttk::TextView *view = make({"a", "b"});

    view->drop_rows(10);

    EXPECT_EQ(view->rows(), 0U);
    EXPECT_TRUE(view->selection().empty());
}

TEST_F(TextViewTest, DropNoRowsDoesNothing) {
    ttk::TextView *view = make({"a", "b"});

    settle_frame();
    view->drop_rows(0);

    EXPECT_EQ(view->rows(), 2U);
    EXPECT_FALSE(canvas.root().dirty());
}

TEST_F(TextViewTest, DropRowsMovesTheSelectionUpWithItsRows) {
    ttk::TextView *view = make({"zero", "one", "two", "three", "four"});

    select(at(view, 3, "t"), at(view, 4, "fo"));
    ASSERT_EQ(view->selection(), "hree\nfo");

    view->drop_rows(2);

    EXPECT_EQ(view->selection(), "hree\nfo");
}

TEST_F(TextViewTest, DropRowsGathersADroppedEndAtTheTop) {
    ttk::TextView *view = make({"a", "bb", "cc"});

    view->select_all();
    view->drop_rows(1);

    EXPECT_EQ(view->selection(), "bb\ncc");
}

TEST_F(TextViewTest, DropRowsIsIgnoredForARun) {
    ttk::TextView *view = run("one\ntwo\nthree");

    ASSERT_EQ(view->rows(), 3U);

    view->drop_rows(1);

    EXPECT_EQ(view->rows(), 3U);
}

TEST_F(TextViewTest, RowHeightIsTheLineHeightOfTheFace) {
    ttk::TextView view;
    ttk::Typeface &type = tests::fonts();

    EXPECT_DOUBLE_EQ(view.row_height(type), rh);

    view.face(ttk::Typeface::regular, ttk::Theme::fontLarge);

    EXPECT_DOUBLE_EQ(view.row_height(type), type.line_height(type.at(400, ttk::Theme::fontLarge)));
}

TEST_F(TextViewTest, NaturalHeightIsEveryRow) {
    ttk::TextView view;

    view.set_rows({"a", "b", "c"});

    EXPECT_DOUBLE_EQ(view.natural_height(tests::fonts(), 300.0), 3.0 * rh);
}

TEST_F(TextViewTest, SetRunFoldsAtTheWidth) {
    const std::string text = "the quick brown fox jumps over the lazy dog again and again";
    ttk::TextView *view = run(text, BLRect{20, 20, advance("the quick brown ") + 1.0, 200});

    EXPECT_GE(view->rows(), 4U);
    EXPECT_DOUBLE_EQ(view->natural_height(tests::fonts(), view->box().w), static_cast<double>(view->rows()) * rh);
}

TEST_F(TextViewTest, SetRunBreaksAtLineBreaks) {
    const ttk::TextView *view = run("one\ntwo\nthree");

    EXPECT_EQ(view->rows(), 3U);
}

TEST_F(TextViewTest, SetRunCutsAWordWiderThanTheView) {
    const ttk::TextView *view = run(std::string(80, 'w'), BLRect{20, 20, 100, 200});

    EXPECT_GE(view->rows(), 2U);
}

TEST_F(TextViewTest, SetRunClearsTheSelection) {
    ttk::TextView *view = make({"rows"});

    view->select_all();
    view->set_run("a run");

    EXPECT_TRUE(view->selection().empty());
}

TEST_F(TextViewTest, SetRunWithTheSameRunRepaintsNothing) {
    ttk::TextView *view = run("a run");

    settle_frame();
    view->set_run("a run");

    EXPECT_FALSE(canvas.root().dirty());
}

TEST_F(TextViewTest, SelectionOfARunGivesBackTheRunItself) {
    const std::string text = "the quick brown fox\njumps   over the lazy dog";
    ttk::TextView *view = run(text, BLRect{20, 20, advance("the quick ") + 1.0, 200});

    ASSERT_GT(view->rows(), 3U);

    view->select_all();

    EXPECT_EQ(view->selection(), text);
}

TEST_F(TextViewTest, PartialSelectionOfARunKeepsItsSpaces) {
    const ttk::TextView *view = run("alpha beta gamma", BLRect{20, 20, advance("alpha beta ") + 1.0, 200});

    ASSERT_EQ(view->rows(), 2U);

    select(at(view, 0, "alpha "), at(view, 1, "ga"));

    EXPECT_EQ(view->selection(), "beta ga");
}

TEST_F(TextViewTest, ARunRefoldsWhenTheWidthChanges) {
    const ttk::TextView *view = run("alpha beta gamma delta", BLRect{20, 20, advance("alpha beta ") + 1.0, 200});
    const std::size_t narrow = view->rows();

    static_cast<controls::Holder *>(view->parent())->spot = BOX;
    canvas.root().relayout();
    canvas.frame();

    EXPECT_GT(narrow, 1U);
    EXPECT_EQ(view->rows(), 1U);
}

TEST_F(TextViewTest, FaceReturnsTheViewAndRefoldsTheRun) {
    ttk::TextView *view = run("alpha beta gamma delta", BLRect{20, 20, advance("alpha beta gamma ") + 1.0, 200});
    const std::size_t small = view->rows();

    EXPECT_EQ(view->face(ttk::Typeface::mono, 30.0F), view);

    canvas.root().relayout();
    canvas.frame();

    EXPECT_GT(view->rows(), small);
}

TEST_F(TextViewTest, InkIsAskedForEveryRowDrawn) {
    ttk::TextView *view = make({"a", "b", "c"});
    std::vector<std::size_t> asked;

    EXPECT_EQ(view->ink([&](const std::size_t row) {
        asked.push_back(row);

        return ttk::Theme::palette().text;
    }),
              view);

    canvas.full();

    EXPECT_EQ(asked, (std::vector<std::size_t>{0, 1, 2}));
}

TEST_F(TextViewTest, OnlyTheRowsInTheRepaintedAreaAreDrawn) {
    std::vector<std::string> rows;

    rows.reserve(100);

    for (int at = 0; at < 100; ++at) {
        rows.push_back("row " + std::to_string(at));
    }

    ttk::TextView *view = make(std::move(rows), BLRect{20, 20, 360, 100 * rh});
    std::vector<std::size_t> asked;

    view->ink([&](const std::size_t row) {
        asked.push_back(row);

        return ttk::Theme::palette().text;
    });
    canvas.full();

    EXPECT_LT(asked.size(), 20U) << "rows outside the window were drawn";

    asked.clear();
    canvas.root().damage(BLRect{20, 20 + (5 * rh) + 2.0, 360, 2.0});
    canvas.frame();

    ASSERT_FALSE(asked.empty());
    EXPECT_TRUE(std::ranges::find(asked, 5U) != asked.end());
    EXPECT_LE(asked.size(), 3U) << "a thin strip drew more than the rows around it";
}

TEST_F(TextViewTest, InkFollowsTheRow) {
    ttk::TextView *view = make({"WWWW"});
    const BLRectI area{20, 20, 200, static_cast<int>(rh)};

    canvas.full();

    const int plain = controls::count_near(canvas, area, ttk::Theme::palette().danger, 40);

    view->ink([](std::size_t) { return ttk::Theme::palette().danger; });
    canvas.full();

    EXPECT_EQ(plain, 0);
    EXPECT_GT(controls::count_near(canvas, area, ttk::Theme::palette().danger, 40), 0)
            << "the row is not drawn in its ink";
}

TEST_F(TextViewTest, SelectAllSelectsEveryRow) {
    ttk::TextView *view = make({"one", "", "three"});

    view->select_all();

    EXPECT_EQ(view->selection(), "one\n\nthree");
}

TEST_F(TextViewTest, SelectAllOfNothingIsEmpty) {
    ttk::TextView *view = make();

    view->select_all();

    EXPECT_TRUE(view->selection().empty());
}

TEST_F(TextViewTest, ClearSelectionClearsAndRepaints) {
    ttk::TextView *view = make({"one"});

    view->select_all();
    settle_frame();
    view->clear_selection();

    EXPECT_TRUE(view->selection().empty());
    EXPECT_TRUE(canvas.root().dirty());
}

TEST_F(TextViewTest, ClearSelectionWithoutOneRepaintsNothing) {
    ttk::TextView *view = make({"one"});

    settle_frame();
    view->clear_selection();

    EXPECT_FALSE(canvas.root().dirty());
}

TEST_F(TextViewTest, PressFocusesTheView) {
    ttk::TextView *view = make({"one"});
    const BLPoint point = at(view, 0, "o");

    controls::click(canvas.root(), point.x, point.y);

    EXPECT_EQ(canvas.root().focused(), view);
    EXPECT_TRUE(view->selection().empty());
}

TEST_F(TextViewTest, DragSelectsWithinARow) {
    const ttk::TextView *view = make({"abcdef"});

    select(at(view, 0, "a"), at(view, 0, "abcd"));

    EXPECT_EQ(view->selection(), "bcd");
}

TEST_F(TextViewTest, DragSelectsAcrossRows) {
    const ttk::TextView *view = make({"first", "second", "third"});

    select(at(view, 0, "fir"), at(view, 2, "th"));

    EXPECT_EQ(view->selection(), "st\nsecond\nth");
}

TEST_F(TextViewTest, DragUpwardsSelectsToo) {
    const ttk::TextView *view = make({"first", "second", "third"});

    select(at(view, 2, "th"), at(view, 0, "fir"));

    EXPECT_EQ(view->selection(), "st\nsecond\nth");
}

TEST_F(TextViewTest, DragBelowTheLastRowClampsToIt) {
    const ttk::TextView *view = make({"first", "second"});

    select(at(view, 0, "fir"), BLPoint{view->box().x + advance("sec") + 0.2, view->box().y + 150.0});

    EXPECT_EQ(view->selection(), "st\nsec");
}

TEST_F(TextViewTest, DragLandsOnMultiByteBoundaries) {
    const ttk::TextView *view = make({"aéйb"});

    select(at(view, 0, "a"), at(view, 0, "aéй"));

    EXPECT_EQ(view->selection(), "éй");
}

TEST_F(TextViewTest, ShiftPressExtendsTheSelection) {
    const ttk::TextView *view = make({"abcdef"});
    const BLPoint from = at(view, 0, "a");
    const BLPoint to = at(view, 0, "abcde");

    controls::click(canvas.root(), from.x, from.y);
    controls::click(canvas.root(), to.x, to.y, true);

    EXPECT_EQ(view->selection(), "bcde");
}

TEST_F(TextViewTest, APlainPressClearsTheSelection) {
    ttk::TextView *view = make({"abcdef"});
    const BLPoint point = at(view, 0, "ab");

    view->select_all();
    controls::click(canvas.root(), point.x, point.y);

    EXPECT_TRUE(view->selection().empty());
}

TEST_F(TextViewTest, PressOnAnEmptyViewSelectsNothing) {
    ttk::TextView *view = make();

    controls::sweep(canvas.root(), 40.0, 30.0, 200.0, 100.0);

    EXPECT_EQ(canvas.root().focused(), view);
    EXPECT_TRUE(view->selection().empty());
}

TEST_F(TextViewTest, UnattachedViewRefusesThePress) {
    ttk::TextView view;

    EXPECT_FALSE(view.press(ttk::Pointer{.x = 1.0, .y = 1.0}));
}

TEST_F(TextViewTest, CtrlASelectsAll) {
    ttk::TextView *view = make({"one", "two"});

    EXPECT_TRUE(view->key(key('a', true)));
    EXPECT_EQ(view->selection(), "one\ntwo");
}

TEST_F(TextViewTest, CtrlCIsConsumedAndKeepsTheSelection) {
    ttk::TextView *view = make({"one", "two"});

    view->select_all();

    EXPECT_TRUE(view->key(key('c', true)));
    EXPECT_TRUE(view->key(key('c', true, true)));
    EXPECT_EQ(view->selection(), "one\ntwo");
}

TEST_F(TextViewTest, OtherKeysAreNotConsumed) {
    ttk::TextView *view = make({"one"});

    EXPECT_FALSE(view->key(key('a')));
    EXPECT_FALSE(view->key(key('c')));
    EXPECT_FALSE(view->key(key('x', true)));
    EXPECT_FALSE(view->key(key(Code::Left)));
    EXPECT_FALSE(view->key(key(Code::Return)));
    EXPECT_FALSE(view->key(key(Code::Escape)));
}

TEST_F(TextViewTest, LosingFocusClearsTheSelection) {
    ttk::TextView *view = make({"one", "two"});

    canvas.root().focus(view);
    view->select_all();
    canvas.root().focus(nullptr);

    EXPECT_TRUE(view->selection().empty());
}

TEST_F(TextViewTest, SelectionIsWashedInTheAccent) {
    ttk::TextView *view = make({"one", "two"});
    const ttk::Theme::Palette &palette = ttk::Theme::palette();
    const int x = static_cast<int>(view->box().x + 300.0);
    const int y = static_cast<int>(view->box().y + (rh / 2.0));

    canvas.full();

    EXPECT_EQ(canvas.at(x, y), palette.background.value);

    view->select_all();
    canvas.frame();

    EXPECT_NE(canvas.at(x, y), palette.background.value) << "a row selected to its end is not washed across the view";
    EXPECT_LT(controls::distance(canvas.at(x, y), palette.accent.value),
              controls::distance(palette.background.value, palette.accent.value))
            << "the wash is not of the accent";
    EXPECT_EQ(canvas.at(x, static_cast<int>(view->box().y + rh + (rh / 2.0))), palette.background.value)
            << "the last row was washed past its text";
}

TEST_F(TextViewTest, EmptyViewPaintsNothing) {
    make();
    canvas.full();

    const BLRectI area{20, 20, 360, 200};

    EXPECT_EQ(controls::count_near(canvas, area, ttk::Theme::palette().background, 0), 360 * 200);
}

TEST_F(TextViewTest, PartialRepaintsMatchAFullOne) {
    ttk::TextView *view = make({"first", "second", "third"});

    select(at(view, 0, "fi"), at(view, 1, "sec"));
    canvas.frame();
    view->append("\nmore");
    canvas.frame();
    view->drop_rows(1);
    canvas.frame();

    EXPECT_TRUE(canvas.matches_full());
}
}
