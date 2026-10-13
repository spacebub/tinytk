// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cstdint>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "Overlays.h"
#include "support/Canvas.h"
#include "ttk/draw/Glyphs.h"
#include "ttk/toolkit/overlays/Menu.h"

namespace {
using overlays::Taker;

enum class Act : std::uint8_t {
    Open = 1,
    Copy = 2,
    Remove = 7,
};

constexpr double LEFT = 50.0;
constexpr double TOP = 40.0;
constexpr double PAD = 5.0;
constexpr double ROW = 32.0;
constexpr double RULE = 9.0;

std::vector<ttk::Menu::Row> rows(const bool danger = true, const bool disabled = true) {
    return {
            ttk::Menu::item(Act::Open, "Open", ttk::Glyphs::Glyph::Folder),
            ttk::Menu::item(Act::Copy, "Copy", ttk::Glyphs::Glyph::Copy, false, disabled),
            ttk::Menu::rule(),
            ttk::Menu::item(Act::Remove, "Remove", ttk::Glyphs::Glyph::Trash, danger),
    };
}

double row_y(const int index) {
    double y = TOP + PAD;

    for (int at = 0; at < index; ++at) {
        y += at == 2 ? RULE : ROW;
    }

    return y + ((index == 2 ? RULE : ROW) / 2.0);
}

BLRect placed() {
    return BLRect{LEFT, TOP, ttk::Menu::WIDTH, ttk::Menu::height_of(rows())};
}

TEST(MenuRow, ItemKeepsTheActionValueLabelGlyphAndFlags) {
    const ttk::Menu::Row row = ttk::Menu::item(Act::Remove, "Remove", ttk::Glyphs::Glyph::Trash, true, true);

    EXPECT_EQ(row.action, 7);
    EXPECT_EQ(row.label, "Remove");
    EXPECT_EQ(row.glyph, ttk::Glyphs::Glyph::Trash);
    EXPECT_TRUE(row.danger);
    EXPECT_TRUE(row.disabled);
    EXPECT_FALSE(row.separator);
}

TEST(MenuRow, ItemIsSafeAndEnabledByDefault) {
    const ttk::Menu::Row row = ttk::Menu::item(Act::Open, "Open", ttk::Glyphs::Glyph::Empty);

    EXPECT_FALSE(row.danger);
    EXPECT_FALSE(row.disabled);
    EXPECT_FALSE(row.separator);
}

TEST(MenuRow, RuleIsASeparatorWithoutAnAction) {
    const ttk::Menu::Row row = ttk::Menu::rule();

    EXPECT_TRUE(row.separator);
    EXPECT_EQ(row.action, -1);
    EXPECT_TRUE(row.label.empty());
}

TEST(MenuSize, EmptyMenuIsOnlyPadding) {
    EXPECT_DOUBLE_EQ(ttk::Menu::height_of({}), 10.0);
}

TEST(MenuSize, HeightCountsActionsRulesAndPadding) {
    EXPECT_DOUBLE_EQ(ttk::Menu::height_of(rows()), (2 * PAD) + (3 * ROW) + RULE);
    EXPECT_DOUBLE_EQ(ttk::Menu::height_of({ttk::Menu::rule(), ttk::Menu::rule()}), (2 * PAD) + (2 * RULE));
}

TEST(MenuSize, WidthIsTwoHundredForty) {
    EXPECT_DOUBLE_EQ(ttk::Menu::WIDTH, 240.0);
}

class Picking : public ::testing::Test {
protected:
    std::vector<int> fired;
    ttk::Menu menu{rows(), [this](const int action) { fired.push_back(action); }};

    void SetUp() override { menu.place(placed(), tests::fonts()); }

    void release_at(const double y, const double x = LEFT + 100.0) { menu.release(ttk::Pointer{.x = x, .y = y}); }
};

TEST_F(Picking, DoesNotTakeFocusAndShowsAPointingCursor) {
    EXPECT_FALSE(menu.takes_focus()) << "the control the menu dropped from must keep focus";
    EXPECT_EQ(menu.cursor, ttk::Cursor::Pointer);
}

TEST_F(Picking, PressIsTakenOnlyInsideTheBox) {
    EXPECT_TRUE(menu.press(ttk::Pointer{.x = LEFT + 1.0, .y = TOP + 1.0}));
    EXPECT_FALSE(menu.press(ttk::Pointer{.x = LEFT - 1.0, .y = TOP + 1.0}));
    EXPECT_FALSE(menu.press(ttk::Pointer{.x = LEFT + 10.0, .y = TOP + placed().h + 1.0}));
}

TEST_F(Picking, ReleaseOnARowReportsItsAction) {
    release_at(row_y(0));
    release_at(row_y(3));

    EXPECT_EQ(fired, (std::vector<int>{1, 7}));
}

TEST_F(Picking, RowEdgesBelongToTheRowBelow) {
    release_at(TOP + PAD);
    release_at(TOP + PAD + ROW - 0.01);

    EXPECT_EQ(fired, (std::vector<int>{1, 1}));
}

TEST_F(Picking, ReleaseOnTheTopPaddingReportsNothing) {
    release_at(TOP + 2.0);

    EXPECT_TRUE(fired.empty());
}

TEST_F(Picking, ReleaseOnTheBottomPaddingReportsNothing) {
    release_at(TOP + placed().h - 2.0);

    EXPECT_TRUE(fired.empty());
}

TEST_F(Picking, ReleaseOnARuleReportsNothing) {
    release_at(row_y(2));

    EXPECT_TRUE(fired.empty());
}

TEST_F(Picking, ReleaseOnADisabledRowReportsNothing) {
    release_at(row_y(1));

    EXPECT_TRUE(fired.empty());
}

TEST_F(Picking, ReleaseIsMatchedByHeightAlone) {
    release_at(row_y(0), LEFT + 1000.0);

    EXPECT_EQ(fired, (std::vector<int>{1})) << "the row is the one at the height of the release";
}

TEST(Menu, ReleaseWithoutACallbackIsSafe) {
    ttk::Menu menu(rows(), nullptr);

    menu.place(placed(), tests::fonts());
    menu.release(ttk::Pointer{.x = LEFT + 100.0, .y = row_y(0)});

    SUCCEED();
}

TEST(Menu, EnabledCopyReportsItsAction) {
    std::vector<int> fired;
    ttk::Menu menu(rows(true, false), [&fired](const int action) { fired.push_back(action); });

    menu.place(placed(), tests::fonts());
    menu.release(ttk::Pointer{.x = LEFT + 100.0, .y = row_y(1)});

    EXPECT_EQ(fired, (std::vector<int>{2}));
}

class Popup : public overlays::Painted {
protected:
    tests::Canvas canvas;
    ttk::Widget *popups = nullptr;
    ttk::Menu *menu = nullptr;
    std::vector<int> fired;
    int dismissed = 0;

    void SetUp() override {
        overlays::Painted::SetUp();

        if (IsSkipped()) {
            return;
        }

        canvas.frame();
        popups = canvas.root().layer(ttk::Root::POPUPS);
    }

    void open(const std::vector<ttk::Menu::Row> &made = rows(), const ttk::Widget *owner = nullptr) {
        ttk::Menu *opened = popups->append(std::make_unique<ttk::Menu>(made, [this](const int action) {
            fired.push_back(action);
            canvas.root().dismiss();
        }));

        // Placing and erasing damage nothing, so the caller repaints the menu's box both times.
        opened->place(placed(), canvas.root().type());
        opened->invalidate();
        canvas.root().set_dismiss(
                [this, opened] {
                    ++dismissed;
                    opened->invalidate();
                    popups->erase(opened);

                    if (menu == opened) {
                        menu = nullptr;
                    }
                },
                owner);
        menu = opened;
        canvas.frame();
    }

    void hover(const int index) {
        canvas.root().motion(LEFT + 200.0, row_y(index));
        canvas.frame();
    }

    std::uint32_t lit(const int index) const {
        return canvas.at(static_cast<int>(LEFT + 200.0), static_cast<int>(row_y(index)));
    }
};

TEST_F(Popup, ClickTriggersTheActionAndTheCallbackMayDestroyTheMenu) {
    open();
    overlays::click(canvas.root(), LEFT + 100.0, row_y(3));

    EXPECT_EQ(fired, (std::vector<int>{7}));
    EXPECT_EQ(dismissed, 1);
    EXPECT_EQ(menu, nullptr);
    EXPECT_TRUE(popups->children().empty());
    EXPECT_FALSE(canvas.root().has_dismiss());
}

TEST_F(Popup, ClickOnADisabledRowKeepsTheMenuOpen) {
    open();
    overlays::click(canvas.root(), LEFT + 100.0, row_y(1));

    EXPECT_TRUE(fired.empty());
    EXPECT_NE(menu, nullptr);
    EXPECT_TRUE(canvas.root().has_dismiss());
}

TEST_F(Popup, PressOutsideDismissesTheMenu) {
    open();
    overlays::click(canvas.root(), 380.0, 220.0);

    EXPECT_EQ(dismissed, 1);
    EXPECT_TRUE(fired.empty());
    EXPECT_EQ(menu, nullptr);
}

TEST_F(Popup, PressOutsideStillReachesTheWidgetThere) {
    auto *page = static_cast<Taker *>(canvas.root().content()->add(std::make_unique<Taker>()));

    canvas.root().relayout();
    open();
    overlays::click(canvas.root(), 380.0, 220.0);

    EXPECT_EQ(dismissed, 1);
    EXPECT_EQ(page->presses, 1) << "the press that closed the menu was swallowed";
}

TEST_F(Popup, PressOnTheOwnerClosesTheMenuAndGoesNoFurther) {
    auto *owner = static_cast<Taker *>(canvas.root().content()->add(std::make_unique<Taker>()));

    canvas.root().relayout();
    open(rows(), owner);
    overlays::click(canvas.root(), 380.0, 220.0);

    EXPECT_EQ(dismissed, 1);
    EXPECT_EQ(owner->presses, 0) << "the owner would open the menu again";
}

TEST_F(Popup, PressInsideTheMenuDoesNotDismissIt) {
    open();
    overlays::press(canvas.root(), LEFT + 100.0, row_y(2));

    EXPECT_EQ(dismissed, 0);
    EXPECT_EQ(canvas.root().grabbed(), menu);

    overlays::release(canvas.root(), LEFT + 100.0, row_y(2));

    EXPECT_EQ(dismissed, 0);
}

TEST_F(Popup, ResizeDismissesTheMenu) {
    open();
    canvas.root().resize(401, 240);
    canvas.root().resize(400, 240);

    EXPECT_EQ(dismissed, 1);
}

TEST_F(Popup, OpeningAnotherDismissesTheFirst) {
    open();
    open();

    EXPECT_EQ(dismissed, 1);
    EXPECT_EQ(popups->children().size(), 1U);
}

TEST_F(Popup, KeepsItsBoxOnARelayout) {
    open();
    canvas.root().relayout();
    canvas.frame();

    const BLRect box = menu->box();

    EXPECT_DOUBLE_EQ(box.x, LEFT);
    EXPECT_DOUBLE_EQ(box.y, TOP);
    EXPECT_DOUBLE_EQ(box.w, ttk::Menu::WIDTH);
}

TEST_F(Popup, PaintsAPanelUnlikeThePage) {
    const std::uint32_t page = canvas.at(static_cast<int>(LEFT + 100.0), static_cast<int>(TOP + 2.0));

    open();

    EXPECT_NE(canvas.at(static_cast<int>(LEFT + 100.0), static_cast<int>(TOP + 2.0)), page);
    EXPECT_EQ(canvas.at(static_cast<int>(LEFT - 2.0), static_cast<int>(TOP + 2.0)), page);
}

TEST_F(Popup, RuleIsDrawnAcrossItsRow) {
    open();

    const int x = static_cast<int>(LEFT + 120.0);
    const int line = static_cast<int>(TOP + PAD + (2 * ROW) + ((RULE - 1.0) / 2.0));

    EXPECT_NE(canvas.at(x, line), canvas.at(x, line - 2));
    EXPECT_EQ(canvas.at(x, line + 2), canvas.at(x, line - 2));
}

TEST_F(Popup, HoverHighlightsTheRowUnderThePointer) {
    open();

    const std::uint32_t plain = lit(0);

    hover(0);

    EXPECT_NE(lit(0), plain);
    EXPECT_EQ(lit(3), plain) << "a row away from the pointer lit up";
}

TEST_F(Popup, HighlightFollowsThePointer) {
    open();

    const std::uint32_t plain = lit(0);

    hover(0);
    hover(3);

    EXPECT_EQ(lit(0), plain);
    EXPECT_NE(lit(3), plain);
    EXPECT_TRUE(canvas.matches_full());
}

TEST_F(Popup, DisabledRowIsNotHighlighted) {
    open();

    const std::uint32_t plain = lit(1);

    hover(1);

    EXPECT_EQ(lit(1), plain);
}

TEST_F(Popup, LeavingClearsTheHighlight) {
    open();

    const std::uint32_t plain = lit(0);

    hover(0);
    canvas.root().motion(380.0, 220.0);
    canvas.frame();

    EXPECT_EQ(lit(0), plain);
    EXPECT_FALSE(menu->hovered());
    EXPECT_TRUE(canvas.matches_full());
}

TEST_F(Popup, PointerLeavingTheWindowClearsTheHighlight) {
    open();

    const std::uint32_t plain = lit(0);

    hover(0);
    canvas.root().leave();
    canvas.frame();

    EXPECT_EQ(lit(0), plain);
}

TEST_F(Popup, DangerRowIsPaintedUnlikeAPlainOne) {
    open(rows(false));

    const std::vector<std::uint32_t> plain = canvas.pixels();

    canvas.root().dismiss();
    open(rows(true));

    const BLRectI apart = overlays::changed(plain, canvas.pixels(), canvas.width());

    ASSERT_GT(apart.w, 0) << "danger made no difference";
    EXPECT_GE(apart.y, static_cast<int>(row_y(3) - (ROW / 2.0)));
    EXPECT_LE(apart.y + apart.h, static_cast<int>(row_y(3) + (ROW / 2.0)));
}

TEST_F(Popup, DisabledRowIsPaintedUnlikeAnEnabledOne) {
    open(rows(true, false));

    const std::vector<std::uint32_t> enabled = canvas.pixels();

    canvas.root().dismiss();
    open(rows(true, true));

    const BLRectI apart = overlays::changed(enabled, canvas.pixels(), canvas.width());

    ASSERT_GT(apart.w, 0) << "disabling made no difference";
    EXPECT_GE(apart.y, static_cast<int>(row_y(1) - (ROW / 2.0)));
    EXPECT_LE(apart.y + apart.h, static_cast<int>(row_y(1) + (ROW / 2.0)));
}

TEST_F(Popup, DismissRepaintsWhatTheMenuCovered) {
    const std::vector<std::uint32_t> before = canvas.pixels();

    open();
    hover(0);
    canvas.root().dismiss();
    canvas.frame();

    EXPECT_EQ(canvas.pixels(), before);
}
}
