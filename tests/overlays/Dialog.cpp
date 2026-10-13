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
#include "ttk/dialogs/DialogLayer.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/layout/Box.h"

namespace {
    using overlays::Probe;
    using overlays::Taker;

    constexpr double MARGIN = 24.0;
    constexpr double GROW = 0.14;

    constexpr BLRect UNDER{0.0, ttk::Theme::barHeight, 400.0, 240.0 - ttk::Theme::barHeight};

    struct Shown {
        tests::Canvas canvas;
        ttk::DialogLayer *layer = nullptr;
        Probe *dialog = nullptr;

        explicit Shown(std::unique_ptr<Probe> made = std::make_unique<Probe>())
            : layer(overlays::dialog_layer(canvas)), dialog(static_cast<Probe *>(layer->show(std::move(made)))) {
            canvas.frame();
        }

        void finish() { canvas.frame_at(1.0); }
    };

    TEST(Dialog, NewDialogIsOpenVisibleAndHasACard) {
        const Probe dialog;

        EXPECT_TRUE(dialog.open());
        EXPECT_TRUE(dialog.visible());
        EXPECT_NE(dialog.card(), nullptr);
        EXPECT_EQ(dialog.card()->parent(), &dialog);
        EXPECT_FALSE(dialog.takes_focus()) << "the scrim must not be a Tab stop";
    }

    TEST(Dialog, CardIsCentredAndNarrowedToLeaveMargins) {
        const Shown shown;
        const BLRect card = shown.dialog->card()->box();
        const double wide = UNDER.w - (2 * MARGIN);

        EXPECT_DOUBLE_EQ(card.w, wide) << "460 wide does not fit 400 less two margins";
        EXPECT_DOUBLE_EQ(card.h, 120.0);
        EXPECT_DOUBLE_EQ(card.x, UNDER.x + ((UNDER.w - wide) / 2.0));
        EXPECT_DOUBLE_EQ(card.y, UNDER.y + ((UNDER.h - 120.0) / 2.0));
    }

    TEST(Dialog, NarrowWantedWidthIsKept) {
        auto made = std::make_unique<Probe>();

        made->wanted = 200.0;

        const Shown shown(std::move(made));
        const BLRect card = shown.dialog->card()->box();

        EXPECT_DOUBLE_EQ(card.w, 200.0);
        EXPECT_DOUBLE_EQ(card.x, 100.0);
    }

    TEST(Dialog, TallCardIsCutToLeaveMarginsAboveAndBelow) {
        const Shown shown(std::make_unique<Probe>(1000.0));
        const BLRect card = shown.dialog->card()->box();

        EXPECT_DOUBLE_EQ(card.h, UNDER.h - (2 * MARGIN));
        EXPECT_DOUBLE_EQ(card.y, UNDER.y + MARGIN);
    }

    TEST(Dialog, ZeroTallSizesTheCardToItsContent) {
        auto made = std::make_unique<Probe>(0.0);

        made->card()->append(std::make_unique<Taker>())->fixedHeight = 90.0;

        const Shown shown(std::move(made));

        EXPECT_DOUBLE_EQ(shown.dialog->card()->box().h, 90.0);
    }

    TEST(Dialog, ContentTallerThanTheRoomIsCut) {
        auto made = std::make_unique<Probe>(0.0);

        made->card()->append(std::make_unique<Taker>())->fixedHeight = 900.0;

        const Shown shown(std::move(made));

        EXPECT_DOUBLE_EQ(shown.dialog->card()->box().h, UNDER.h - (2 * MARGIN));
    }

    TEST(Dialog, HeadingAndBodyAppendTheirTextToTheBox) {
        auto column = ttk::Box::column();
        const ttk::Label *title = Probe::heading(column.get(), "Title");
        const ttk::Label *said = Probe::body(column.get(), "Body text");

        ASSERT_EQ(column->children().size(), 2U);
        EXPECT_EQ(column->children()[0].get(), title);
        EXPECT_EQ(column->children()[1].get(), said);
        EXPECT_EQ(title->text(), "Title");
        EXPECT_EQ(said->text(), "Body text");
    }

    TEST(Dialog, AtAnswersForTheScrimAndTheEmptyCard) {
        const Shown shown;
        const BLRect card = shown.dialog->card()->box();

        EXPECT_EQ(shown.dialog->at(2.0, 230.0), shown.dialog);
        EXPECT_EQ(shown.dialog->at(overlays::mid_x(card), overlays::mid_y(card)), shown.dialog);
    }

    TEST(Dialog, AtAnswersWithTheCardWidgetUnderThePoint) {
        auto made = std::make_unique<Probe>();
        Taker *inside = made->card()->append(std::make_unique<Taker>());

        const Shown shown(std::move(made));
        const BLRect card = shown.dialog->card()->box();

        EXPECT_EQ(shown.dialog->at(overlays::mid_x(card), overlays::mid_y(card)), inside);
    }

    TEST(Dialog, HiddenDialogAnswersNoPoint) {
        const Shown shown;

        shown.dialog->set_open(false);

        EXPECT_FALSE(shown.dialog->open());
        EXPECT_FALSE(shown.dialog->visible());
        EXPECT_EQ(shown.dialog->at(2.0, 230.0), nullptr);
    }

    TEST(Dialog, ClickOnTheScrimCallsDismissed) {
        Shown shown;
        int dismissed = 0;

        shown.dialog->dismissed = [&dismissed] { ++dismissed; };
        overlays::click(shown.canvas.root(), 4.0, 230.0);

        EXPECT_EQ(dismissed, 1);
    }

    TEST(Dialog, ClickOnTheScrimTakesTheDialogDownThroughTheLayer) {
        Shown shown;

        overlays::click(shown.canvas.root(), 4.0, 230.0);

        EXPECT_EQ(shown.layer->top(), nullptr);
        EXPECT_FALSE(shown.layer->covered());
    }

    TEST(Dialog, ClickOnTheCardDoesNotDismiss) {
        Shown shown;
        int dismissed = 0;
        const BLRect card = shown.dialog->card()->box();

        shown.dialog->dismissed = [&dismissed] { ++dismissed; };
        overlays::click(shown.canvas.root(), overlays::mid_x(card), overlays::mid_y(card));

        EXPECT_EQ(dismissed, 0);
    }

    TEST(Dialog, PressOnTheCardReleasedOnTheScrimDoesNotDismiss) {
        Shown shown;
        int dismissed = 0;
        const BLRect card = shown.dialog->card()->box();

        shown.dialog->dismissed = [&dismissed] { ++dismissed; };
        overlays::press(shown.canvas.root(), overlays::mid_x(card), overlays::mid_y(card));
        overlays::release(shown.canvas.root(), 4.0, 230.0);

        EXPECT_EQ(dismissed, 0) << "a drag out of the card counted as a scrim click";
    }

    TEST(Dialog, PressOnTheScrimReleasedOnTheCardDoesNotDismiss) {
        Shown shown;
        int dismissed = 0;
        const BLRect card = shown.dialog->card()->box();

        shown.dialog->dismissed = [&dismissed] { ++dismissed; };
        overlays::press(shown.canvas.root(), 4.0, 230.0);
        overlays::release(shown.canvas.root(), overlays::mid_x(card), overlays::mid_y(card));

        EXPECT_EQ(dismissed, 0);
    }

    TEST(Dialog, PressThatClosedAPopupIsNoScrimClick) {
        Shown shown;
        int dismissed = 0;
        int popups = 0;

        shown.dialog->dismissed = [&dismissed] { ++dismissed; };
        shown.canvas.root().set_dismiss([&popups] { ++popups; });
        overlays::click(shown.canvas.root(), 4.0, 230.0);

        EXPECT_EQ(popups, 1);
        EXPECT_FALSE(shown.canvas.root().has_dismiss());
        EXPECT_EQ(dismissed, 0) << "the press that closed the popup also closed the dialog";

        overlays::click(shown.canvas.root(), 4.0, 230.0);

        EXPECT_EQ(dismissed, 1);
    }

    TEST(Dialog, DismissedLeftEmptyIsSafe) {
        Shown shown;

        shown.dialog->dismissed = nullptr;
        overlays::click(shown.canvas.root(), 4.0, 230.0);

        EXPECT_EQ(shown.layer->top(), shown.dialog);
    }

    TEST(Dialog, PageUnderTheDialogGetsNoPointer) {
        tests::Canvas canvas;
        auto *page = static_cast<Taker *>(canvas.root().content()->add(std::make_unique<Taker>()));
        ttk::DialogLayer *layer = overlays::dialog_layer(canvas);

        overlays::click(canvas.root(), 4.0, 230.0);

        ASSERT_EQ(page->presses, 1) << "the page widget is not where the case expects";

        layer->show(std::make_unique<Probe>());
        canvas.frame();
        layer->top()->dismissed = nullptr;
        canvas.root().motion(4.0, 230.0);

        EXPECT_NE(canvas.root().hovered(), page);

        overlays::click(canvas.root(), 4.0, 230.0);
        overlays::click(canvas.root(), 200.0, 147.0);
        canvas.root().wheel(1.0, 4.0, 230.0);

        EXPECT_EQ(page->presses, 1) << "a press reached the page through the dialog";
    }

    TEST(Dialog, PageGetsThePointerBackOnceTheDialogIsDown) {
        tests::Canvas canvas;
        auto *page = static_cast<Taker *>(canvas.root().content()->add(std::make_unique<Taker>()));
        ttk::DialogLayer *layer = overlays::dialog_layer(canvas);

        layer->show(std::make_unique<Probe>());
        canvas.frame();
        layer->dismiss();
        canvas.frame();
        overlays::click(canvas.root(), 4.0, 230.0);

        EXPECT_EQ(page->presses, 1);
    }

    TEST(Dialog, CardWidgetsReceiveClicks) {
        auto made = std::make_unique<Probe>();
        Taker *inside = made->card()->append(std::make_unique<Taker>());

        Shown shown(std::move(made));

        overlays::click(shown.canvas.root(), inside);

        EXPECT_EQ(inside->presses, 1);
        EXPECT_EQ(inside->releases, 1);
        EXPECT_EQ(shown.layer->top(), shown.dialog);
    }

    TEST(Dialog, CardGrowsFromNinetyFivePercentOnItsFirstLayout) {
        Shown shown;
        const BLRect card = shown.dialog->card()->box();
        const int edge = static_cast<int>(card.x) + 4;
        const int middle = static_cast<int>(overlays::mid_y(card));
        const std::uint32_t scrim = shown.canvas.at(2, middle);

        EXPECT_EQ(shown.canvas.at(edge, middle), scrim) << "the card edge is painted at full size before the grow";

        shown.finish();

        EXPECT_NE(shown.canvas.at(edge, middle), scrim) << "the card did not reach its full size";
    }

    TEST(Dialog, GrowKeepsTheDialogLiveUntilItEnds) {
        Shown shown;

        EXPECT_TRUE(shown.canvas.root().busy());

        shown.canvas.frame_at(GROW / 2.0);

        EXPECT_TRUE(shown.canvas.root().busy()) << "the grow stopped half way";
        EXPECT_TRUE(shown.dialog->advance(GROW / 2.0));

        shown.canvas.frame_at(GROW + 0.01);

        EXPECT_FALSE(shown.canvas.root().busy());
        EXPECT_FALSE(shown.dialog->advance(GROW + 0.02));
    }

    TEST(Dialog, GrowStartsFromTheClockOfTheFirstLayout) {
        tests::Canvas canvas;
        ttk::DialogLayer *layer = overlays::dialog_layer(canvas);

        canvas.root().set_now(5.0);

        auto *dialog = static_cast<Probe *>(layer->show(std::make_unique<Probe>()));

        canvas.frame();
        canvas.frame_at(5.0 + (GROW / 2.0));

        EXPECT_TRUE(canvas.root().busy()) << "the grow was timed from before the dialog was laid out";

        canvas.frame_at(5.0 + GROW + 0.01);

        EXPECT_FALSE(canvas.root().busy());
        EXPECT_EQ(dialog->open(), true);
    }

    TEST(Dialog, ReopeningPlaysTheGrowAgain) {
        Shown shown;

        shown.finish();

        const BLRect card = shown.dialog->card()->box();
        const int edge = static_cast<int>(card.x) + 4;
        const int middle = static_cast<int>(overlays::mid_y(card));

        shown.dialog->set_open(false);
        shown.canvas.frame_at(2.0);

        const std::uint32_t scrimless = shown.canvas.at(edge, middle);

        shown.dialog->set_open(true);
        shown.canvas.frame_at(3.0);

        EXPECT_NE(shown.canvas.at(edge, middle), scrimless);
        EXPECT_EQ(shown.canvas.at(edge, middle), shown.canvas.at(2, middle)) << "reopening skipped the grow";

        shown.canvas.frame_at(4.0);

        EXPECT_NE(shown.canvas.at(edge, middle), shown.canvas.at(2, middle));
    }

    TEST(Dialog, OpeningAnOpenDialogDoesNotReplayTheGrow) {
        Shown shown;

        shown.finish();
        shown.dialog->set_open(true);
        shown.canvas.root().relayout();
        shown.canvas.frame_at(2.0);

        EXPECT_FALSE(shown.canvas.root().busy());
    }

    TEST(Dialog, HidingClearsTheScrim) {
        Shown shown;
        const std::uint32_t scrim = shown.canvas.at(2, 230);

        shown.finish();
        shown.dialog->set_open(false);
        shown.canvas.frame_at(2.0);

        EXPECT_NE(shown.canvas.at(2, 230), scrim);
        EXPECT_TRUE(shown.canvas.matches_full());
    }

    TEST(Dialog, GrowingPaintMatchesAFullRepaint) {
        Shown shown;

        for (const double at : {0.0, 0.02, 0.05, 0.09, 0.12}) {
            shown.canvas.frame_at(at);

            EXPECT_TRUE(shown.canvas.matches_full()) << "partial/full mismatch while growing at " << at;
        }
    }

    TEST(Dialog, ClippedRepaintWhileGrowingKeepsScaledChildren) {
        auto made = std::make_unique<Probe>();

        made->card()->fills = false;
        made->corner = made->card()->append(std::make_unique<Taker>());
        made->cornerAt = BLRect{0.0, 20.0, 20.0, 20.0};

        Shown shown(std::move(made));
        const BLRect card = shown.dialog->card()->box();

        // At 95% the swatch sits about nine pixels in from where it is laid out, past its own box.
        for (const double at : {0.0, 0.03}) {
            shown.canvas.frame_at(at);
            shown.canvas.root().damage(BLRect{card.x + 22.0, card.y + 18.0, 6.0, 24.0});
            shown.canvas.frame();

            EXPECT_TRUE(shown.canvas.matches_full()) << "a scaled child was culled by the clip at " << at;
        }
    }

    TEST(Dialog, PaintOverIsDrawnUnderTheGrowScale) {
        auto made = std::make_unique<Probe>();

        made->marker = true;

        Shown shown(std::move(made));
        const BLRect card = shown.dialog->card()->box();
        const int x = static_cast<int>(card.x) + 1;
        const int y = static_cast<int>(card.y) + 1;
        constexpr std::uint32_t green = 0xff00ff00;

        EXPECT_GT(shown.dialog->overs, 0);
        EXPECT_NE(shown.canvas.at(x, y), green) << "paint_over was drawn at full size while the card grows";

        shown.finish();

        EXPECT_EQ(shown.canvas.at(x, y), green);
    }

    TEST(Dialog, PaintFillsTheWholeBoxWithTheScrim) {
        tests::Canvas canvas;
        ttk::DialogLayer *layer = overlays::dialog_layer(canvas);
        const std::vector<std::uint32_t> before = canvas.pixels();

        layer->show(std::make_unique<Probe>());
        canvas.frame_at(1.0);

        const BLRectI covered = overlays::changed(before, canvas.pixels(), canvas.width());

        EXPECT_EQ(covered.x, 0);
        EXPECT_EQ(covered.y, static_cast<int>(ttk::Theme::barHeight));
        EXPECT_EQ(covered.w, canvas.width());
        EXPECT_EQ(covered.y + covered.h, canvas.height());
    }
}
