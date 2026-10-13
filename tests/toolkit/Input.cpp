// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL_keycode.h>
#include <gtest/gtest.h>

#include "support/Canvas.h"
#include "toolkit/Probe.h"
#include "ttk/toolkit/Event.h"
#include "ttk/toolkit/Root.h"

namespace {
constexpr int WIDTH = 200;
constexpr int HEIGHT = 120;

using Journal = std::vector<std::string>;

class Routed : public ::testing::Test {
protected:
    ttk::Root root{tests::fonts()};
    Journal journal;

    void SetUp() override {
        root.resize(WIDTH, HEIGHT);
        root.settle();
        root.take();
    }

    tests::Probe *on(ttk::Widget *parent, const BLRect &box, const char *name) {
        return tests::mount(parent, box, tests::fonts(), name, &journal);
    }

    tests::Probe *popup(const BLRect &box, const char *name) { return on(root.layer(ttk::Root::POPUPS), box, name); }

    tests::Probe *group(ttk::Widget *parent, const BLRect &box, const char *name) {
        tests::Probe *made = on(parent, box, name);

        made->set_pointer(false);

        return made;
    }

    static ttk::Key tab(const bool shift = false) { return ttk::Key{.code = ttk::Code::Tab, .shift = shift}; }

    bool fresh(const char *entry) {
        return std::ranges::none_of(journal, [entry](const std::string &line) { return line == entry; });
    }
};

TEST(Event, PointerDefaultsToALeftClickAtTheOrigin) {
    const ttk::Pointer at;

    EXPECT_EQ(at.x, 0.0);
    EXPECT_EQ(at.y, 0.0);
    EXPECT_EQ(at.button, ttk::Click::Left);
    EXPECT_FALSE(at.ctrl);
    EXPECT_FALSE(at.shift);
}

TEST(Event, KeyDefaultsToNoKeyAndNoModifier) {
    const ttk::Key pressed;

    EXPECT_EQ(pressed.code, 0);
    EXPECT_FALSE(pressed.ctrl);
    EXPECT_FALSE(pressed.shift);
    EXPECT_FALSE(pressed.alt);
    EXPECT_FALSE(pressed.repeat);
}

TEST(Event, CodesAreSdlKeycodes) {
    EXPECT_EQ(ttk::Code::Escape, static_cast<int>(SDLK_ESCAPE));
    EXPECT_EQ(ttk::Code::Space, static_cast<int>(SDLK_SPACE));
    EXPECT_EQ(ttk::Code::Return, static_cast<int>(SDLK_RETURN));
    EXPECT_EQ(ttk::Code::Tab, static_cast<int>(SDLK_TAB));
    EXPECT_EQ(ttk::Code::Backspace, static_cast<int>(SDLK_BACKSPACE));
    EXPECT_EQ(ttk::Code::Delete, static_cast<int>(SDLK_DELETE));
    EXPECT_EQ(ttk::Code::Left, static_cast<int>(SDLK_LEFT));
    EXPECT_EQ(ttk::Code::Right, static_cast<int>(SDLK_RIGHT));
    EXPECT_EQ(ttk::Code::Up, static_cast<int>(SDLK_UP));
    EXPECT_EQ(ttk::Code::Down, static_cast<int>(SDLK_DOWN));
    EXPECT_EQ(ttk::Code::Home, static_cast<int>(SDLK_HOME));
    EXPECT_EQ(ttk::Code::End, static_cast<int>(SDLK_END));
    EXPECT_EQ(ttk::Code::PageUp, static_cast<int>(SDLK_PAGEUP));
    EXPECT_EQ(ttk::Code::PageDown, static_cast<int>(SDLK_PAGEDOWN));
    EXPECT_EQ(ttk::Code::F1, static_cast<int>(SDLK_F1));
    EXPECT_EQ(static_cast<int>(SDLK_A), 'a') << "a printable key is its unshifted character";
}

TEST_F(Routed, MotionRecordsThePointer) {
    root.motion(12.5, 34.25);

    EXPECT_EQ(root.pointer_x(), 12.5);
    EXPECT_EQ(root.pointer_y(), 34.25);
}

TEST_F(Routed, MotionOverNothingHoversNothing) {
    root.motion(10, 10);

    EXPECT_EQ(root.hovered(), nullptr);
    EXPECT_EQ(root.cursor(), ttk::Cursor::Default);
}

TEST_F(Routed, MotionOntoAWidgetEntersAndHoversIt) {
    tests::Probe *button = popup({10, 10, 40, 20}, "button");

    root.motion(20, 15);

    EXPECT_EQ(root.hovered(), button);
    EXPECT_TRUE(button->hovered());
    EXPECT_TRUE(button->holds_pointer());
    EXPECT_EQ(journal, (Journal{"button:enter", "button:hover"}));
    ASSERT_EQ(button->hoveredAt.size(), 1U);
    EXPECT_EQ(button->hoveredAt[0].x, 20.0);
    EXPECT_EQ(button->hoveredAt[0].y, 15.0);
}

TEST_F(Routed, MotionAcrossAWidgetOnlyHoversIt) {
    const tests::Probe *button = popup({10, 10, 40, 20}, "button");

    root.motion(20, 15);
    journal.clear();
    root.motion(30, 15);

    EXPECT_EQ(journal, Journal{"button:hover"});
    EXPECT_EQ(button->enters, 1);
}

TEST_F(Routed, MotionOffAWidgetLeavesIt) {
    const tests::Probe *button = popup({10, 10, 40, 20}, "button");

    root.motion(20, 15);
    root.motion(150, 100);

    EXPECT_EQ(root.hovered(), nullptr);
    EXPECT_FALSE(button->hovered());
    EXPECT_EQ(button->leaves, 1);
}

TEST_F(Routed, MotionFromOneWidgetToAnotherLeavesBeforeEntering) {
    popup({10, 10, 40, 20}, "one");
    popup({60, 10, 40, 20}, "two");

    root.motion(20, 15);
    journal.clear();
    root.motion(70, 15);

    EXPECT_EQ(journal, (Journal{"one:leave", "two:enter", "two:hover"}));
}

TEST_F(Routed, EnterAndLeaveRepaintTheLitBox) {
    tests::Probe *button = popup({10, 10, 40, 20}, "button");

    root.motion(20, 15);

    EXPECT_TRUE(tests::covers(root.take(), 49, 29));

    root.motion(150, 100);

    EXPECT_TRUE(tests::covers(root.take(), 10, 10));
    EXPECT_NE(button, nullptr);
}

TEST_F(Routed, BoxEdgesAreHalfOpen) {
    tests::Probe *button = popup({10, 10, 40, 20}, "button");

    root.motion(10, 10);

    EXPECT_EQ(root.hovered(), button);

    root.motion(50, 15);

    EXPECT_EQ(root.hovered(), nullptr) << "the right edge belongs to the next pixel";

    root.motion(20, 30);

    EXPECT_EQ(root.hovered(), nullptr) << "the bottom edge belongs to the next pixel";
}

TEST_F(Routed, WithinIsToldOnlyWhereThePathsPart) {
    tests::Probe *row = group(root.layer(ttk::Root::POPUPS), {0, 0, 100, 40}, "row");

    on(row, {0, 0, 40, 40}, "left");
    on(row, {50, 0, 40, 40}, "right");

    root.motion(10, 10);

    EXPECT_EQ(row->inside, std::vector<bool>{true});

    root.motion(60, 10);

    EXPECT_EQ(row->inside, std::vector<bool>{true}) << "moving between children left the row";

    root.motion(150, 100);

    EXPECT_EQ(row->inside, (std::vector<bool>{true, false}));
}

TEST_F(Routed, WithinReachesEveryAncestor) {
    tests::Probe *outer = group(root.layer(ttk::Root::POPUPS), {0, 0, 100, 100}, "outer");
    tests::Probe *inner = group(outer, {0, 0, 100, 100}, "inner");

    on(inner, {10, 10, 20, 20}, "leaf");
    root.motion(15, 15);

    EXPECT_EQ(outer->inside, std::vector<bool>{true});
    EXPECT_EQ(inner->inside, std::vector<bool>{true});
    EXPECT_TRUE(outer->holds_pointer());
    EXPECT_FALSE(outer->hovered()) << "only the widget itself is hovered";
}

TEST_F(Routed, DisabledWidgetsAreNotHovered) {
    tests::Probe *button = popup({10, 10, 40, 20}, "button");

    button->set_enabled(false);
    root.motion(20, 15);

    EXPECT_EQ(root.hovered(), nullptr);
}

TEST_F(Routed, HiddenWidgetsAreNotHovered) {
    tests::Probe *button = popup({10, 10, 40, 20}, "button");

    button->set_visible(false);
    root.motion(20, 15);

    EXPECT_EQ(root.hovered(), nullptr);
}

TEST_F(Routed, AHintMakesAContainerHoverable) {
    tests::Probe *label = group(root.layer(ttk::Root::POPUPS), {10, 10, 40, 20}, "label");

    root.motion(20, 15);

    EXPECT_EQ(root.hovered(), nullptr);

    label->hint = "Explains the label";
    root.motion(21, 15);

    EXPECT_EQ(root.hovered(), label);
}

TEST_F(Routed, TheLastChildIsHitFirst) {
    popup({10, 10, 40, 20}, "under");
    tests::Probe *over = popup({20, 10, 40, 20}, "over");

    root.motion(25, 15);

    EXPECT_EQ(root.hovered(), over);
}

TEST_F(Routed, PopupsTakeThePointerBeforeThePage) {
    on(root.content(), {0, 0, WIDTH, HEIGHT}, "page");
    tests::Probe *menu = popup({10, 10, 40, 20}, "menu");

    root.motion(20, 15);

    EXPECT_EQ(root.hovered(), menu);
}

TEST_F(Routed, HigherLayersTakeThePointerFirst) {
    popup({10, 10, 40, 20}, "menu");
    tests::Probe *tip = on(root.layer(ttk::Root::TIPS), {10, 10, 40, 20}, "tip");

    root.motion(20, 15);

    EXPECT_EQ(root.hovered(), tip);
}

TEST_F(Routed, LayersLetThePointerThroughWhereTheyHoldNothing) {
    tests::Probe *page = on(root.content(), {0, 0, WIDTH, HEIGHT}, "page");

    popup({10, 10, 40, 20}, "menu");
    on(root.layer(ttk::Root::DIALOGS), {150, 80, 20, 20}, "dialog");
    root.motion(100, 60);

    EXPECT_EQ(root.hovered(), page);
}

TEST_F(Routed, PressIsTakenByTheWidgetUnderThePointer) {
    tests::Probe *button = popup({10, 10, 40, 20}, "button");

    root.press({.x = 20, .y = 15, .button = ttk::Click::Right, .ctrl = true, .shift = true});

    EXPECT_EQ(root.grabbed(), button);
    EXPECT_TRUE(button->pressed());
    EXPECT_EQ(root.hovered(), button) << "a press without motion first did not move the hover";
    ASSERT_EQ(button->pressedAt.size(), 1U);
    EXPECT_EQ(button->pressedAt[0].button, ttk::Click::Right);
    EXPECT_TRUE(button->pressedAt[0].ctrl);
    EXPECT_TRUE(button->pressedAt[0].shift);
    EXPECT_EQ(root.pointer_x(), 20.0);
    EXPECT_EQ(root.pointer_y(), 15.0);
}

TEST_F(Routed, PressNotTakenIsOfferedToTheAncestors) {
    tests::Probe *card = popup({0, 0, 100, 100}, "card");
    tests::Probe *badge = on(card, {10, 10, 20, 20}, "badge");

    badge->takesPress = false;
    root.press({.x = 15, .y = 15});

    EXPECT_EQ(journal, (Journal{"badge:enter", "badge:hover", "card:within", "badge:press", "card:press"}));
    EXPECT_EQ(root.grabbed(), card);
    EXPECT_TRUE(card->pressed());
    EXPECT_FALSE(badge->pressed());
}

TEST_F(Routed, PressSkipsADisabledAncestor) {
    tests::Probe *outer = popup({0, 0, 100, 100}, "outer");
    tests::Probe *middle = on(outer, {0, 0, 100, 100}, "middle");
    tests::Probe *leaf = on(middle, {10, 10, 20, 20}, "leaf");

    leaf->takesPress = false;
    middle->set_enabled(false);
    root.press({.x = 15, .y = 15});

    EXPECT_TRUE(fresh("middle:press"));
    EXPECT_EQ(root.grabbed(), outer);
}

TEST_F(Routed, PressThatNothingTakesClearsFocus) {
    tests::Probe *field = popup({10, 10, 40, 20}, "field");

    root.focus(field);
    root.press({.x = 150, .y = 100});

    EXPECT_EQ(root.focused(), nullptr);
    EXPECT_EQ(root.grabbed(), nullptr);
}

TEST_F(Routed, PressRefusedEverywhereClearsFocus) {
    tests::Probe *field = popup({10, 10, 40, 20}, "field");
    tests::Probe *sink = popup({60, 10, 40, 20}, "sink");

    sink->takesPress = false;
    root.focus(field);
    root.press({.x = 70, .y = 15});

    EXPECT_EQ(root.focused(), nullptr);
}

TEST_F(Routed, PressTakenKeepsFocus) {
    tests::Probe *field = popup({10, 10, 40, 20}, "field");

    popup({60, 10, 40, 20}, "button");
    root.focus(field);
    root.press({.x = 70, .y = 15});

    EXPECT_EQ(root.focused(), field) << "a press does not move focus by itself";
}

TEST_F(Routed, HeldPressDragsAnywhereThePointerGoes) {
    tests::Probe *slider = popup({10, 10, 40, 20}, "slider");

    popup({100, 10, 40, 20}, "other");
    root.press({.x = 20, .y = 15});
    journal.clear();
    root.motion(120, 15);
    root.motion(-30, 500);

    EXPECT_EQ(journal, (Journal{"slider:drag", "slider:drag"}));
    EXPECT_EQ(root.hovered(), slider) << "the hover moved while a press was held";
    ASSERT_EQ(slider->draggedAt.size(), 2U);
    EXPECT_EQ(slider->draggedAt[1].x, -30.0);
    EXPECT_EQ(slider->draggedAt[1].y, 500.0);
}

TEST_F(Routed, ReleaseEndsThePressAndMovesTheHover) {
    tests::Probe *slider = popup({10, 10, 40, 20}, "slider");
    tests::Probe *other = popup({100, 10, 40, 20}, "other");

    root.press({.x = 20, .y = 15});
    root.motion(110, 15);
    journal.clear();
    root.release({.x = 110, .y = 15});

    EXPECT_EQ(root.grabbed(), nullptr);
    EXPECT_FALSE(slider->pressed());
    EXPECT_EQ(root.hovered(), other);
    ASSERT_EQ(slider->releasedAt.size(), 1U);
    EXPECT_EQ(slider->releasedAt[0].x, 110.0);
    EXPECT_EQ(journal.front(), "slider:release");
}

TEST_F(Routed, ReleaseWithoutAPressDoesNothing) {
    const tests::Probe *button = popup({10, 10, 40, 20}, "button");

    root.release({.x = 20, .y = 15});

    EXPECT_TRUE(journal.empty());
    EXPECT_EQ(root.hovered(), nullptr);
    EXPECT_TRUE(button->releasedAt.empty());
}

TEST_F(Routed, GrabMakesAWidgetHoldThePress) {
    tests::Probe *handle = popup({10, 10, 40, 20}, "handle");

    root.grab(handle);

    EXPECT_EQ(root.grabbed(), handle);

    root.motion(150, 100);

    EXPECT_EQ(journal, Journal{"handle:drag"});

    root.release({.x = 150, .y = 100});

    EXPECT_EQ(journal.back(), "handle:release");
}

TEST_F(Routed, GrabOfNullReleasesWithoutCallingAnything) {
    const tests::Probe *handle = popup({10, 10, 40, 20}, "handle");

    root.press({.x = 20, .y = 15});
    journal.clear();
    root.grab(nullptr);

    EXPECT_EQ(root.grabbed(), nullptr);
    EXPECT_TRUE(journal.empty());

    root.release({.x = 20, .y = 15});

    EXPECT_TRUE(handle->releasedAt.empty());
}

TEST_F(Routed, WheelIsOfferedUpTheTreeUntilConsumed) {
    tests::Probe *scroller = popup({0, 0, 100, 100}, "scroller");
    const tests::Probe *row = on(scroller, {0, 0, 100, 20}, "row");

    scroller->takesWheel = true;
    root.wheel(-2.5, 10, 10);

    EXPECT_EQ(journal, (Journal{"row:wheel", "scroller:wheel"}));
    EXPECT_EQ(scroller->wheeled, -2.5);
    EXPECT_EQ(row->wheeled, 0.0);
}

TEST_F(Routed, WheelConsumedStopsThere) {
    tests::Probe *scroller = popup({0, 0, 100, 100}, "scroller");
    tests::Probe *inner = on(scroller, {0, 0, 100, 50}, "inner");

    scroller->takesWheel = true;
    inner->takesWheel = true;
    root.wheel(1, 10, 10);

    EXPECT_EQ(inner->wheeled, 1.0);
    EXPECT_EQ(scroller->wheeled, 0.0);
}

TEST_F(Routed, WheelOverNothingDoesNothing) {
    root.wheel(1, 10, 10);

    EXPECT_TRUE(journal.empty());
}

TEST_F(Routed, LeaveClearsTheHover) {
    const tests::Probe *button = popup({10, 10, 40, 20}, "button");

    root.motion(20, 15);
    root.leave();

    EXPECT_EQ(root.hovered(), nullptr);
    EXPECT_EQ(button->leaves, 1);
    EXPECT_FALSE(button->hovered());
}

TEST_F(Routed, LeaveIsIgnoredWhileAPressIsHeld) {
    tests::Probe *button = popup({10, 10, 40, 20}, "button");

    root.press({.x = 20, .y = 15});
    root.leave();

    EXPECT_EQ(root.hovered(), button);
    EXPECT_EQ(button->leaves, 0);
}

TEST_F(Routed, LeaveClearsThePressedLook) {
    const tests::Probe *button = popup({10, 10, 40, 20}, "button");

    root.press({.x = 20, .y = 15});
    root.grab(nullptr);
    root.leave();

    EXPECT_FALSE(button->pressed());
}

TEST_F(Routed, CursorFollowsTheHover) {
    tests::Probe *link = popup({10, 10, 40, 20}, "link");

    link->cursor = ttk::Cursor::Pointer;
    root.motion(20, 15);

    EXPECT_EQ(root.cursor(), ttk::Cursor::Pointer);

    root.motion(150, 100);

    EXPECT_EQ(root.cursor(), ttk::Cursor::Default);
}

TEST_F(Routed, CursorOfTheHeldWidgetWinsWhereverThePointerIs) {
    tests::Probe *grip = popup({10, 10, 40, 20}, "grip");
    tests::Probe *text = popup({100, 10, 40, 20}, "text");

    grip->cursor = ttk::Cursor::Grabbing;
    text->cursor = ttk::Cursor::Text;
    root.press({.x = 20, .y = 15});
    root.motion(110, 15);

    EXPECT_EQ(root.cursor(), ttk::Cursor::Grabbing);
}

TEST_F(Routed, KeyGoesToTheFocusedWidget) {
    tests::Probe *field = popup({10, 10, 40, 20}, "field");

    field->takesKey = true;
    root.focus(field);

    EXPECT_TRUE(root.key({.code = 'a', .ctrl = true}));
    ASSERT_EQ(field->keys.size(), 1U);
    EXPECT_EQ(field->keys[0].code, 'a');
    EXPECT_TRUE(field->keys[0].ctrl);
}

TEST_F(Routed, KeyNotConsumedIsOfferedToTheAncestors) {
    tests::Probe *form = popup({0, 0, 100, 100}, "form");
    tests::Probe *field = on(form, {10, 10, 40, 20}, "field");

    form->takesKey = true;
    root.focus(field);
    journal.clear();

    EXPECT_TRUE(root.key({.code = ttk::Code::Escape}));
    EXPECT_EQ(journal, (Journal{"field:key", "form:key"}));
}

TEST_F(Routed, KeyNobodyConsumesIsNotConsumed) {
    tests::Probe *field = popup({10, 10, 40, 20}, "field");

    root.focus(field);

    EXPECT_FALSE(root.key({.code = 'q'}));
    EXPECT_FALSE(root.key({.code = ttk::Code::Escape}));
}

TEST_F(Routed, KeyWithNothingFocusedIsNotConsumed) {
    popup({10, 10, 40, 20}, "field");

    EXPECT_FALSE(root.key({.code = 'q'}));
    EXPECT_TRUE(journal.empty());
}

TEST_F(Routed, WroteGoesToTheFocusedWidget) {
    tests::Probe *field = popup({10, 10, 40, 20}, "field");

    root.focus(field);
    root.wrote("héllo ");
    root.wrote("日本");

    EXPECT_EQ(field->written, "héllo 日本");
}

TEST_F(Routed, WroteWithNothingFocusedIsIgnored) {
    const tests::Probe *field = popup({10, 10, 40, 20}, "field");

    root.wrote("lost");

    EXPECT_TRUE(field->written.empty());
}

TEST_F(Routed, FocusMovesBetweenWidgets) {
    tests::Probe *one = popup({10, 10, 40, 20}, "one");
    tests::Probe *two = popup({60, 10, 40, 20}, "two");

    root.focus(one);

    EXPECT_EQ(root.focused(), one);
    EXPECT_TRUE(one->focused());

    journal.clear();
    root.focus(two);

    EXPECT_EQ(journal, (Journal{"one:lost", "two:gained"}));
    EXPECT_FALSE(one->focused());
    EXPECT_TRUE(two->focused());
}

TEST_F(Routed, FocusOnTheFocusedWidgetDoesNothing) {
    tests::Probe *one = popup({10, 10, 40, 20}, "one");
    int told = 0;

    root.composing = [&](bool) { ++told; };
    root.focus(one);
    journal.clear();
    root.focus(one);

    EXPECT_TRUE(journal.empty());
    EXPECT_EQ(told, 1);
}

TEST_F(Routed, FocusOfNullClearsIt) {
    tests::Probe *one = popup({10, 10, 40, 20}, "one");

    root.focus(one);
    root.focus(nullptr);

    EXPECT_EQ(root.focused(), nullptr);
    EXPECT_EQ(one->lost, 1);
}

TEST_F(Routed, FocusRepaintsBothWidgets) {
    tests::Probe *one = popup({10, 10, 40, 20}, "one");
    tests::Probe *two = popup({100, 60, 40, 20}, "two");

    root.focus(one);
    root.take();
    root.focus(two);

    const std::vector<BLRect> &taken = root.take();

    EXPECT_TRUE(tests::covers(taken, 10, 10));
    EXPECT_TRUE(tests::covers(taken, 139, 79));
}

TEST_F(Routed, ComposingIsToldWhetherTheFocusTakesText) {
    tests::Probe *field = popup({10, 10, 40, 20}, "field");
    tests::Probe *panel = group(root.layer(ttk::Root::POPUPS), {60, 10, 40, 20}, "panel");
    std::vector<bool> told;

    root.composing = [&](const bool on) { told.push_back(on); };
    root.focus(field);
    root.focus(panel);
    root.focus(nullptr);

    EXPECT_EQ(told, (std::vector<bool>{true, false, false}));
}

TEST_F(Routed, TabFromNothingFocusesTheFirstStop) {
    tests::Probe *one = popup({10, 10, 20, 20}, "one");

    popup({40, 10, 20, 20}, "two");

    EXPECT_TRUE(root.key(tab()));
    EXPECT_EQ(root.focused(), one);
}

TEST_F(Routed, ShiftTabFromNothingFocusesTheLastStop) {
    popup({10, 10, 20, 20}, "one");
    tests::Probe *two = popup({40, 10, 20, 20}, "two");

    EXPECT_TRUE(root.key(tab(true)));
    EXPECT_EQ(root.focused(), two);
}

TEST_F(Routed, TabWalksTheTreeInOrderAndWraps) {
    tests::Probe *one = popup({10, 10, 20, 20}, "one");
    tests::Probe *box = group(root.layer(ttk::Root::POPUPS), {40, 10, 60, 20}, "box");
    tests::Probe *two = on(box, {40, 10, 20, 20}, "two");
    tests::Probe *three = on(box, {70, 10, 20, 20}, "three");

    const std::vector<tests::Probe *> order{one, two, three, one};

    for (tests::Probe *expected : order) {
        root.key(tab());

        EXPECT_EQ(root.focused(), expected) << expected->name;
    }
}

TEST_F(Routed, ShiftTabWalksBackwardsAndWraps) {
    tests::Probe *one = popup({10, 10, 20, 20}, "one");
    tests::Probe *two = popup({40, 10, 20, 20}, "two");
    tests::Probe *three = popup({70, 10, 20, 20}, "three");

    root.focus(two);
    root.key(tab(true));

    EXPECT_EQ(root.focused(), one);

    root.key(tab(true));

    EXPECT_EQ(root.focused(), three);
}

TEST_F(Routed, CtrlTabMovesFocusToo) {
    tests::Probe *one = popup({10, 10, 20, 20}, "one");

    EXPECT_TRUE(root.key({.code = ttk::Code::Tab, .ctrl = true}));
    EXPECT_EQ(root.focused(), one);
}

TEST_F(Routed, AltTabIsLeftAlone) {
    popup({10, 10, 20, 20}, "one");

    EXPECT_FALSE(root.key({.code = ttk::Code::Tab, .alt = true}));
    EXPECT_EQ(root.focused(), nullptr);
}

TEST_F(Routed, HeldTabMovesFocusOnce) {
    tests::Probe *one = popup({10, 10, 20, 20}, "one");

    popup({40, 10, 20, 20}, "two");
    root.focus(one);

    EXPECT_TRUE(root.key({.code = ttk::Code::Tab, .repeat = true})) << "a held Tab still counts as consumed";
    EXPECT_EQ(root.focused(), one);
}

TEST_F(Routed, TabConsumedByTheFocusedWidgetMovesNothing) {
    tests::Probe *editor = popup({10, 10, 20, 20}, "editor");

    popup({40, 10, 20, 20}, "two");
    editor->takesKey = true;
    root.focus(editor);

    EXPECT_TRUE(root.key(tab()));
    EXPECT_EQ(root.focused(), editor);
}

TEST_F(Routed, TabWithNoStopIsStillConsumed) {
    EXPECT_TRUE(root.key(tab()));
    EXPECT_EQ(root.focused(), nullptr);
}

TEST_F(Routed, TabSkipsHiddenDisabledAndPointerlessWidgets) {
    tests::Probe *one = popup({10, 10, 20, 20}, "one");
    tests::Probe *hidden = popup({40, 10, 20, 20}, "hidden");
    tests::Probe *disabled = popup({70, 10, 20, 20}, "disabled");
    const tests::Probe *box = group(root.layer(ttk::Root::POPUPS), {100, 10, 20, 20}, "box");
    tests::Probe *last = popup({130, 10, 20, 20}, "last");

    hidden->set_visible(false);
    disabled->set_enabled(false);
    root.focus(one);
    root.key(tab());

    EXPECT_EQ(root.focused(), last);
    EXPECT_FALSE(box->takes_focus());
}

TEST_F(Routed, TabSkipsEverythingInsideAHiddenOrDisabledContainer) {
    tests::Probe *one = popup({10, 10, 20, 20}, "one");
    tests::Probe *hidden = group(root.layer(ttk::Root::POPUPS), {40, 10, 40, 20}, "hidden");
    tests::Probe *disabled = group(root.layer(ttk::Root::POPUPS), {90, 10, 40, 20}, "disabled");

    on(hidden, {40, 10, 20, 20}, "inside hidden");
    on(disabled, {90, 10, 20, 20}, "inside disabled");
    hidden->set_visible(false);
    disabled->set_enabled(false);
    root.focus(one);
    root.key(tab());

    EXPECT_EQ(root.focused(), one) << "focus went into a hidden or disabled container";
}

TEST_F(Routed, PositiveTabIndicesComeFirstInAscendingOrder) {
    tests::Probe *plain = popup({10, 10, 20, 20}, "plain");
    tests::Probe *second = popup({40, 10, 20, 20}, "second");
    tests::Probe *first = popup({70, 10, 20, 20}, "first");

    second->set_tab_index(2);
    first->set_tab_index(1);

    const std::vector<tests::Probe *> order{first, second, plain, first};

    for (tests::Probe *expected : order) {
        root.key(tab());

        EXPECT_EQ(root.focused(), expected) << expected->name;
    }
}

TEST_F(Routed, EqualTabIndicesKeepTreeOrder) {
    tests::Probe *one = popup({10, 10, 20, 20}, "one");
    tests::Probe *two = popup({40, 10, 20, 20}, "two");

    one->set_tab_index(3);
    two->set_tab_index(3);
    root.key(tab());

    EXPECT_EQ(root.focused(), one);

    root.key(tab());

    EXPECT_EQ(root.focused(), two);
}

TEST_F(Routed, ZeroTabIndexTakesTheNearestAncestorsIndex) {
    tests::Probe *loose = popup({10, 10, 20, 20}, "loose");
    tests::Probe *form = group(root.layer(ttk::Root::POPUPS), {40, 10, 80, 20}, "form");
    tests::Probe *name = on(form, {40, 10, 20, 20}, "name");
    tests::Probe *mail = on(form, {70, 10, 20, 20}, "mail");

    form->set_tab_index(1);

    const std::vector<tests::Probe *> order{name, mail, loose};

    for (tests::Probe *expected : order) {
        root.key(tab());

        EXPECT_EQ(root.focused(), expected) << expected->name;
    }
}

TEST_F(Routed, NegativeTabIndexTakesASubtreeOutOfOrder) {
    tests::Probe *one = popup({10, 10, 20, 20}, "one");
    tests::Probe *skipped = popup({40, 10, 20, 20}, "skipped");
    tests::Probe *box = group(root.layer(ttk::Root::POPUPS), {70, 10, 40, 20}, "box");
    tests::Probe *inside = on(box, {70, 10, 20, 20}, "inside");
    tests::Probe *last = popup({120, 10, 20, 20}, "last");

    inside->set_tab_index(5);
    skipped->set_tab_index(-1);
    box->set_tab_index(-1);
    root.focus(one);
    root.key(tab());

    EXPECT_EQ(root.focused(), last);
}

TEST_F(Routed, TabFromAWidgetOutOfOrderMovesOnFromWhereItSits) {
    popup({10, 10, 20, 20}, "one");
    tests::Probe *skipped = popup({40, 10, 20, 20}, "skipped");
    tests::Probe *three = popup({70, 10, 20, 20}, "three");

    skipped->set_tab_index(-1);
    root.focus(skipped);

    EXPECT_EQ(root.focused(), skipped) << "a widget out of Tab order still takes focus directly";

    root.key(tab());

    EXPECT_EQ(root.focused(), three);
}

TEST_F(Routed, TheTopmostLayerWithStopsKeepsFocusToItself) {
    on(root.content(), {0, 0, 20, 20}, "page");

    tests::Probe *ok = on(root.layer(ttk::Root::DIALOGS), {10, 10, 20, 20}, "ok");
    tests::Probe *cancel = on(root.layer(ttk::Root::DIALOGS), {40, 10, 20, 20}, "cancel");

    root.key(tab());

    EXPECT_EQ(root.focused(), ok);

    root.key(tab());

    EXPECT_EQ(root.focused(), cancel);

    root.key(tab());

    EXPECT_EQ(root.focused(), ok) << "focus left the dialog";
}

TEST_F(Routed, AHiddenLayerGivesUpItsStops) {
    tests::Probe *page = on(root.content(), {0, 0, 20, 20}, "page");

    on(root.layer(ttk::Root::DIALOGS), {10, 10, 20, 20}, "ok");
    root.layer(ttk::Root::DIALOGS)->set_visible(false);
    root.key(tab());

    EXPECT_EQ(root.focused(), page);

    root.layer(ttk::Root::DIALOGS)->set_visible(true);
}

TEST_F(Routed, TabFallsBackToThePage) {
    tests::Probe *page = on(root.content(), {0, 0, 20, 20}, "page");

    group(root.layer(ttk::Root::DIALOGS), {10, 10, 20, 20}, "scrim");
    root.key(tab());

    EXPECT_EQ(root.focused(), page);
}

TEST_F(Routed, TabRevealsTheFocusOnEveryAncestorNearestFirst) {
    tests::Probe *outer = group(root.layer(ttk::Root::POPUPS), {0, 0, 200, 100}, "outer");
    tests::Probe *inner = group(outer, {0, 0, 200, 100}, "inner");
    tests::Probe *field = on(inner, {30, 40, 20, 10}, "field");

    field->spread = true;
    field->spreadBox = {28, 38, 24, 14};
    root.key(tab());

    ASSERT_EQ(inner->revealed.size(), 1U);
    ASSERT_EQ(outer->revealed.size(), 1U);
    EXPECT_TRUE(tests::same(inner->revealed[0], field->spreadBox)) << "reveal was not given what is drawn";
    EXPECT_TRUE(tests::same(outer->revealed[0], field->spreadBox));
}

TEST_F(Routed, FocusGivenDirectlyRevealsNothing) {
    tests::Probe *outer = group(root.layer(ttk::Root::POPUPS), {0, 0, 200, 100}, "outer");
    tests::Probe *field = on(outer, {30, 40, 20, 10}, "field");

    root.focus(field);

    EXPECT_TRUE(outer->revealed.empty());
}

TEST_F(Routed, FocusNextIsTheSameAsTab) {
    tests::Probe *one = popup({10, 10, 20, 20}, "one");
    tests::Probe *two = popup({40, 10, 20, 20}, "two");

    root.focus_next(false);

    EXPECT_EQ(root.focused(), one);

    root.focus_next(true);

    EXPECT_EQ(root.focused(), two);
}

TEST_F(Routed, ReturnPressesAndReleasesTheFocusedControlAtItsCentre) {
    tests::Probe *button = popup({10, 20, 40, 30}, "button");

    root.focus(button);
    journal.clear();

    EXPECT_TRUE(root.key({.code = ttk::Code::Return}));
    EXPECT_EQ(journal, (Journal{"button:key", "button:press", "button:release"}));
    ASSERT_EQ(button->pressedAt.size(), 1U);
    EXPECT_EQ(button->pressedAt[0].x, 30.0);
    EXPECT_EQ(button->pressedAt[0].y, 35.0);
    ASSERT_EQ(button->releasedAt.size(), 1U);
    EXPECT_EQ(button->releasedAt[0].x, 30.0);
    EXPECT_EQ(root.grabbed(), nullptr) << "a keyboard press left the control holding the pointer";
}

TEST_F(Routed, SpaceActivatesTheFocusedControl) {
    tests::Probe *button = popup({10, 20, 40, 30}, "button");

    root.focus(button);

    EXPECT_TRUE(root.key({.code = ttk::Code::Space}));
    EXPECT_EQ(button->releasedAt.size(), 1U);
}

TEST_F(Routed, HeldReturnActsOnce) {
    tests::Probe *button = popup({10, 20, 40, 30}, "button");

    root.focus(button);

    EXPECT_TRUE(root.key({.code = ttk::Code::Return, .repeat = true}));
    EXPECT_TRUE(button->pressedAt.empty());
}

TEST_F(Routed, ReturnRefusedByTheControlIsNotReleased) {
    tests::Probe *button = popup({10, 20, 40, 30}, "button");

    button->takesPress = false;
    root.focus(button);

    EXPECT_TRUE(root.key({.code = ttk::Code::Return}));
    EXPECT_EQ(button->pressedAt.size(), 1U);
    EXPECT_TRUE(button->releasedAt.empty());
}

TEST_F(Routed, ReturnOnAWidgetThatTakesNoFocusIsNotConsumed) {
    tests::Probe *panel = group(root.layer(ttk::Root::POPUPS), {10, 20, 40, 30}, "panel");

    root.focus(panel);

    EXPECT_FALSE(root.key({.code = ttk::Code::Return}));
    EXPECT_TRUE(panel->pressedAt.empty());
}

TEST_F(Routed, ReturnWithNothingFocusedIsNotConsumed) {
    EXPECT_FALSE(root.key({.code = ttk::Code::Return}));
    EXPECT_FALSE(root.key({.code = ttk::Code::Space}));
}

TEST_F(Routed, ReturnConsumedByTheWidgetIsNotAPress) {
    tests::Probe *field = popup({10, 20, 40, 30}, "field");

    field->takesKey = true;
    root.focus(field);
    root.key({.code = ttk::Code::Return});

    EXPECT_TRUE(field->pressedAt.empty());
}

TEST_F(Routed, NoPopupIsRegisteredAtFirst) {
    EXPECT_FALSE(root.has_dismiss());
    EXPECT_FALSE(root.just_dismissed());
}

TEST_F(Routed, SetDismissRegistersAPopup) {
    root.set_dismiss([] {});

    EXPECT_TRUE(root.has_dismiss());
}

TEST_F(Routed, DismissCallsTheCallbackOnceAndClearsIt) {
    int closed = 0;

    root.set_dismiss([&] { ++closed; });
    root.dismiss();
    root.dismiss();

    EXPECT_EQ(closed, 1);
    EXPECT_FALSE(root.has_dismiss());
}

TEST_F(Routed, DismissWithNothingRegisteredDoesNothing) {
    root.dismiss();

    EXPECT_FALSE(root.has_dismiss());
}

TEST_F(Routed, ASecondPopupDismissesTheFirst) {
    int first = 0;
    int second = 0;

    root.set_dismiss([&] { ++first; });
    root.set_dismiss([&] { ++second; });

    EXPECT_EQ(first, 1);
    EXPECT_EQ(second, 0);
    EXPECT_TRUE(root.has_dismiss());

    root.dismiss();

    EXPECT_EQ(first, 1);
    EXPECT_EQ(second, 1);
}

TEST_F(Routed, NullDismissClearsWithoutCalling) {
    int closed = 0;

    root.set_dismiss([&] { ++closed; });
    root.set_dismiss(nullptr);

    EXPECT_EQ(closed, 0);
    EXPECT_FALSE(root.has_dismiss());
}

TEST_F(Routed, ADismissCanOpenTheNextPopup) {
    int reopened = 0;

    root.set_dismiss([&] { root.set_dismiss([&] { ++reopened; }); });
    root.dismiss();

    EXPECT_TRUE(root.has_dismiss()) << "a popup registered while closing was lost";

    root.dismiss();

    EXPECT_EQ(reopened, 1);
}

TEST_F(Routed, PressOnNothingDismissesAndGoesNoFurther) {
    tests::Probe *field = popup({10, 10, 20, 20}, "field");
    int closed = 0;

    root.focus(field);
    root.set_dismiss([&] { ++closed; });
    root.press({.x = 150, .y = 100});

    EXPECT_EQ(closed, 1);
    EXPECT_TRUE(root.just_dismissed());
    EXPECT_EQ(root.focused(), field) << "the closing press went on and cleared focus";
}

TEST_F(Routed, PressOnTheOwnerDismissesWithoutReopening) {
    const tests::Probe *select = on(root.content(), {10, 10, 40, 20}, "select");
    tests::Probe *list = popup({10, 30, 40, 60}, "list");
    int closed = 0;

    root.set_dismiss([&] { ++closed; }, select);
    root.press({.x = 20, .y = 15});

    EXPECT_EQ(closed, 1);
    EXPECT_TRUE(select->pressedAt.empty()) << "the owner was pressed and would reopen the popup";
    EXPECT_EQ(root.grabbed(), nullptr);
    EXPECT_NE(list, nullptr);
}

TEST_F(Routed, PressOnTheOwnersDescendantDismissesWithoutReopening) {
    tests::Probe *select = on(root.content(), {10, 10, 40, 20}, "select");
    const tests::Probe *arrow = on(select, {40, 10, 10, 20}, "arrow");
    int closed = 0;

    root.set_dismiss([&] { ++closed; }, select);
    root.press({.x = 45, .y = 15});

    EXPECT_EQ(closed, 1);
    EXPECT_TRUE(arrow->pressedAt.empty());
}

TEST_F(Routed, PressElsewhereDismissesAndCarriesOn) {
    const tests::Probe *select = on(root.content(), {10, 10, 40, 20}, "select");
    tests::Probe *other = on(root.content(), {100, 10, 40, 20}, "other");
    int closed = 0;

    root.set_dismiss([&] { ++closed; }, select);
    root.press({.x = 110, .y = 15});

    EXPECT_EQ(closed, 1);
    EXPECT_TRUE(root.just_dismissed());
    EXPECT_EQ(root.grabbed(), other);
}

TEST_F(Routed, PressInsideThePopupKeepsItOpen) {
    tests::Probe *item = popup({10, 30, 40, 20}, "item");
    int closed = 0;

    root.set_dismiss([&] { ++closed; });
    root.press({.x = 20, .y = 35});

    EXPECT_EQ(closed, 0);
    EXPECT_FALSE(root.just_dismissed());
    EXPECT_TRUE(root.has_dismiss());
    EXPECT_EQ(root.grabbed(), item);
}

TEST_F(Routed, JustDismissedLastsUntilTheNextPress) {
    root.set_dismiss([] {});
    root.press({.x = 150, .y = 100});

    ASSERT_TRUE(root.just_dismissed());

    root.press({.x = 150, .y = 100});

    EXPECT_FALSE(root.just_dismissed());
}

TEST_F(Routed, TabOffTheOwnerDismissesThePopup) {
    tests::Probe *select = on(root.content(), {10, 10, 40, 20}, "select");
    tests::Probe *next = on(root.content(), {60, 10, 40, 20}, "next");
    int closed = 0;

    root.focus(select);
    root.set_dismiss([&] { ++closed; }, select);
    root.key(tab());

    EXPECT_EQ(closed, 1);
    EXPECT_EQ(root.focused(), next);
}

TEST_F(Routed, TabOntoTheOwnerKeepsThePopup) {
    tests::Probe *before = on(root.content(), {10, 10, 40, 20}, "before");
    tests::Probe *select = on(root.content(), {60, 10, 40, 20}, "select");
    int closed = 0;

    root.focus(before);
    root.set_dismiss([&] { ++closed; }, select);
    root.key(tab());

    EXPECT_EQ(closed, 0);
    EXPECT_EQ(root.focused(), select);
}

TEST_F(Routed, TabWithinThePopupLayerKeepsAnOwnerlessPopup) {
    tests::Probe *one = popup({10, 10, 40, 20}, "one");
    tests::Probe *two = popup({10, 40, 40, 20}, "two");
    int closed = 0;

    root.focus(one);
    root.set_dismiss([&] { ++closed; });
    root.key(tab());

    EXPECT_EQ(closed, 0);
    EXPECT_EQ(root.focused(), two);
}

TEST_F(Routed, TabOffThePopupLayerDismissesAnOwnerlessPopup) {
    tests::Probe *page = on(root.content(), {10, 10, 40, 20}, "page");
    int closed = 0;

    root.set_dismiss([&] { ++closed; });
    root.key(tab());

    EXPECT_EQ(closed, 1);
    EXPECT_EQ(root.focused(), page);
}

TEST_F(Routed, ErasingTheFocusedWidgetForgetsIt) {
    tests::Probe *field = popup({10, 10, 40, 20}, "field");

    root.focus(field);
    root.layer(ttk::Root::POPUPS)->erase(field);

    EXPECT_EQ(root.focused(), nullptr);
    EXPECT_FALSE(root.key({.code = 'a'}));
}

TEST_F(Routed, ErasingTheHoveredWidgetForgetsIt) {
    const tests::Probe *button = popup({10, 10, 40, 20}, "button");

    root.motion(20, 15);
    root.layer(ttk::Root::POPUPS)->erase(button);

    EXPECT_EQ(root.hovered(), nullptr);

    root.motion(150, 100);

    EXPECT_EQ(root.hovered(), nullptr);
}

TEST_F(Routed, ErasingTheHeldWidgetForgetsIt) {
    const tests::Probe *slider = popup({10, 10, 40, 20}, "slider");

    root.press({.x = 20, .y = 15});
    root.layer(ttk::Root::POPUPS)->erase(slider);

    EXPECT_EQ(root.grabbed(), nullptr);

    root.release({.x = 20, .y = 15});
    root.motion(150, 100);

    EXPECT_EQ(root.hovered(), nullptr);
}

TEST_F(Routed, ClearingForgetsDescendantsAtAnyDepth) {
    tests::Probe *box = group(root.layer(ttk::Root::POPUPS), {0, 0, 100, 100}, "box");
    tests::Probe *inner = group(box, {0, 0, 100, 100}, "inner");
    tests::Probe *field = on(inner, {10, 10, 40, 20}, "field");

    root.focus(field);
    root.motion(20, 15);
    root.layer(ttk::Root::POPUPS)->clear();

    EXPECT_EQ(root.focused(), nullptr);
    EXPECT_EQ(root.hovered(), nullptr);
}

TEST_F(Routed, ErasingThePopupOwnerKeepsThePopupButDropsTheOwner) {
    const tests::Probe *select = on(root.content(), {10, 10, 40, 20}, "select");
    tests::Probe *other = on(root.content(), {10, 10, 40, 20}, "other");
    int closed = 0;

    root.set_dismiss([&] { ++closed; }, select);
    root.content()->erase(select);

    EXPECT_TRUE(root.has_dismiss());

    root.press({.x = 20, .y = 15});

    EXPECT_EQ(closed, 1);
    EXPECT_EQ(root.grabbed(), other) << "a press was still swallowed for an owner that is gone";
}
}
