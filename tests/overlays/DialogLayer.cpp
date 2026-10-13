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

namespace {
    using overlays::Probe;
    using overlays::Taker;

    class Stack : public ::testing::Test {
    protected:
        tests::Canvas canvas;
        ttk::DialogLayer *layer = nullptr;
        int destroyed = 0;

        void SetUp() override { layer = overlays::dialog_layer(canvas); }

        Probe *show(const double tall = 120.0) {
            auto made = std::make_unique<Probe>(tall);

            made->destroyed = &destroyed;

            auto *shown = static_cast<Probe *>(layer->show(std::move(made)));

            canvas.frame();

            return shown;
        }
    };

    TEST_F(Stack, EmptyStackIsUncoveredWithNoTop) {
        EXPECT_FALSE(layer->covered());
        EXPECT_EQ(layer->top(), nullptr);
    }

    TEST_F(Stack, ShowPutsTheDialogOnTopAndReturnsIt) {
        auto made = std::make_unique<Probe>();
        const Probe *raw = made.get();

        EXPECT_EQ(layer->show(std::move(made)), raw);
        EXPECT_EQ(layer->top(), raw);
        EXPECT_TRUE(layer->covered());
        EXPECT_EQ(raw->parent(), layer);
    }

    TEST_F(Stack, ShowHidesTheDialogUnder) {
        Probe *under = show();
        Probe *over = show();

        EXPECT_FALSE(under->visible());
        EXPECT_TRUE(over->visible());
        EXPECT_EQ(layer->top(), over);
        EXPECT_EQ(layer->children().size(), 2U);
    }

    TEST_F(Stack, ShowCallsOpenedOnceWithDismissedAlreadySet) {
        auto made = std::make_unique<Probe>();
        Probe *raw = made.get();
        bool wired = false;
        bool scheduled = false;

        made->on_opened = [&] {
            wired = static_cast<bool>(raw->dismissed);
            scheduled = canvas.root().dirty();
        };

        layer->show(std::move(made));

        EXPECT_EQ(raw->opens, 1);
        EXPECT_TRUE(wired) << "opened() ran before dismissed was set to close the dialog";
        EXPECT_TRUE(scheduled) << "opened() ran before the relayout was requested";
    }

    TEST_F(Stack, ShowRequestsARelayout) {
        layer->show(std::make_unique<Probe>());

        EXPECT_TRUE(canvas.root().settle());
    }

    TEST_F(Stack, DismissedOfAShownDialogClosesIt) {
        Probe *shown = show();

        shown->dismissed();

        EXPECT_EQ(layer->top(), nullptr);
        EXPECT_EQ(destroyed, 1);
    }

    TEST_F(Stack, CloseAsksTheDialogAndStaysUpWhenRefused) {
        Probe *shown = show();
        int closed = 0;

        layer->closed = [&closed](ttk::Dialog *) { ++closed; };
        shown->stay = true;
        layer->close();

        EXPECT_EQ(shown->asked, 1);
        EXPECT_EQ(layer->top(), shown);
        EXPECT_EQ(closed, 0);
        EXPECT_EQ(destroyed, 0);
    }

    TEST_F(Stack, CloseTakesTheDialogDownWhenItAgrees) {
        ASSERT_NE(show(), nullptr);

        layer->close();

        EXPECT_EQ(layer->top(), nullptr);
        EXPECT_EQ(destroyed, 1);
    }

    TEST_F(Stack, CloseWithNothingShownDoesNothing) {
        int closed = 0;

        layer->closed = [&closed](ttk::Dialog *) { ++closed; };
        layer->close();

        EXPECT_EQ(closed, 0);
        EXPECT_FALSE(layer->covered());
    }

    TEST_F(Stack, DismissDoesNotAskTheDialog) {
        Probe *shown = show();

        shown->stay = true;
        layer->dismiss();

        EXPECT_EQ(destroyed, 1);
        EXPECT_EQ(layer->top(), nullptr);
    }

    TEST_F(Stack, DismissCallsClosedWithTheTopBeforeDestroyingIt) {
        Probe *shown = show();
        const ttk::Dialog *told = nullptr;
        int aliveThen = -1;
        const ttk::Dialog *topThen = nullptr;

        layer->closed = [&](ttk::Dialog *gone) {
            told = gone;
            aliveThen = destroyed;
            topThen = layer->top();
        };
        layer->dismiss();

        EXPECT_EQ(told, shown);
        EXPECT_EQ(aliveThen, 0) << "closed ran after the dialog was destroyed";
        EXPECT_EQ(topThen, shown);
        EXPECT_EQ(destroyed, 1);
    }

    TEST_F(Stack, DismissShowsTheDialogUnderAndCallsItsOpened) {
        Probe *under = show();

        show();

        ASSERT_EQ(under->opens, 1);

        layer->dismiss();

        EXPECT_EQ(layer->top(), under);
        EXPECT_TRUE(under->visible());
        EXPECT_EQ(under->opens, 2) << "the dialog uncovered was not told it is on top again";
    }

    TEST_F(Stack, DismissWithNothingShownDoesNothing) {
        int closed = 0;

        layer->closed = [&closed](ttk::Dialog *) { ++closed; };
        layer->dismiss();

        EXPECT_EQ(closed, 0);
    }

    TEST_F(Stack, DialogsComeDownInReverseOrder) {
        std::vector<const ttk::Dialog *> order;
        const Probe *first = show();
        const Probe *second = show();
        const Probe *third = show();

        layer->closed = [&order](ttk::Dialog *gone) { order.push_back(gone); };
        layer->close();
        layer->close();
        layer->close();

        EXPECT_EQ(order, (std::vector<const ttk::Dialog *>{third, second, first}));
        EXPECT_FALSE(layer->covered());
    }

    TEST_F(Stack, SyncReachesOnlyTheTop) {
        Probe *under = show();
        Probe *over = show();

        layer->sync();

        EXPECT_EQ(over->syncs, 1);
        EXPECT_EQ(under->syncs, 0);
    }

    TEST_F(Stack, SyncWithNothingShownDoesNothing) {
        layer->sync();

        EXPECT_FALSE(layer->covered());
    }

    TEST_F(Stack, DialogsAreLaidOutBelowTheTitleBar) {
        const Probe *shown = show();
        const BLRect box = shown->box();

        EXPECT_DOUBLE_EQ(box.x, 0.0);
        EXPECT_DOUBLE_EQ(box.y, ttk::Theme::barHeight);
        EXPECT_DOUBLE_EQ(box.w, canvas.width());
        EXPECT_DOUBLE_EQ(box.h, canvas.height() - ttk::Theme::barHeight);
    }

    TEST_F(Stack, CoveredDialogsAreNotLaidOut) {
        Probe *under = show();

        show();

        const int before = under->arranged;

        canvas.root().relayout();
        canvas.frame();

        EXPECT_EQ(under->arranged, before) << "a hidden dialog was measured again";
    }

    TEST_F(Stack, TitleBarStaysUncovered) {
        const std::vector<std::uint32_t> before = canvas.pixels();

        show();
        canvas.frame_at(1.0);

        const BLRectI covered = overlays::changed(before, canvas.pixels(), canvas.width());

        EXPECT_GE(covered.y, static_cast<int>(ttk::Theme::barHeight)) << "the scrim reached into the title bar";
    }

    TEST_F(Stack, DismissRepaintsWhatTheDialogCovered) {
        const std::vector<std::uint32_t> before = canvas.pixels();

        show();
        canvas.frame_at(1.0);
        layer->dismiss();
        canvas.frame();

        EXPECT_EQ(canvas.pixels(), before) << "the dialog left pixels behind";
    }

    TEST_F(Stack, DismissOverAnotherRepaintsTheOneUnder) {
        show();
        canvas.frame_at(1.0);

        const std::vector<std::uint32_t> alone = canvas.pixels();

        show(60.0);
        canvas.frame_at(2.0);
        layer->dismiss();
        canvas.frame_at(3.0);

        EXPECT_EQ(canvas.pixels(), alone);
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST_F(Stack, TabKeepsFocusInsideTheTopDialog) {
        auto *page = static_cast<Taker *>(canvas.root().content()->add(std::make_unique<Taker>()));
        auto made = std::make_unique<Probe>();

        made->card()->fills = false;

        Taker *one = made->card()->append(std::make_unique<Taker>());
        Taker *two = made->card()->append(std::make_unique<Taker>());

        layer->show(std::move(made));
        canvas.root().relayout();
        canvas.frame();

        std::vector<const ttk::Widget *> seen;

        for (int step = 0; step < 4; ++step) {
            ASSERT_TRUE(canvas.root().key(ttk::Key{.code = ttk::Code::Tab}));

            seen.push_back(canvas.root().focused());
        }

        EXPECT_EQ(seen, (std::vector<const ttk::Widget *>{one, two, one, two}));
        EXPECT_NE(canvas.root().focused(), page);
    }

    TEST_F(Stack, ShiftTabWalksTheDialogBackwards) {
        auto made = std::make_unique<Probe>();

        made->card()->fills = false;

        const Taker *one = made->card()->append(std::make_unique<Taker>());
        const Taker *two = made->card()->append(std::make_unique<Taker>());

        layer->show(std::move(made));
        canvas.frame();
        canvas.root().key(ttk::Key{.code = ttk::Code::Tab, .shift = true});

        EXPECT_EQ(canvas.root().focused(), two);

        canvas.root().key(ttk::Key{.code = ttk::Code::Tab, .shift = true});

        EXPECT_EQ(canvas.root().focused(), one);
    }

    TEST_F(Stack, TabReachesThePageOnceTheDialogIsDown) {
        const auto *page = static_cast<Taker *>(canvas.root().content()->add(std::make_unique<Taker>()));
        auto made = std::make_unique<Probe>();

        made->card()->append(std::make_unique<Taker>());
        layer->show(std::move(made));
        canvas.frame();
        canvas.root().key(ttk::Key{.code = ttk::Code::Tab});
        layer->dismiss();

        EXPECT_EQ(canvas.root().focused(), nullptr) << "focus was left on a destroyed widget";

        canvas.root().key(ttk::Key{.code = ttk::Code::Tab});

        EXPECT_EQ(canvas.root().focused(), page);
    }

    TEST_F(Stack, EscapeIsLeftToTheApplication) {
        auto made = std::make_unique<Probe>();
        Taker *inside = made->card()->append(std::make_unique<Taker>());

        layer->show(std::move(made));
        canvas.frame();
        canvas.root().focus(inside);

        EXPECT_FALSE(canvas.root().key(ttk::Key{.code = ttk::Code::Escape}))
                << "a dialog consumed Escape, which the application binds to close()";
        EXPECT_TRUE(layer->covered());

        layer->close();

        EXPECT_FALSE(layer->covered());
    }
}
