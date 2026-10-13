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

#include "support/Canvas.h"
#include "toolkit/Probe.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Event.h"
#include "ttk/toolkit/Painter.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/Widget.h"

namespace {
constexpr int WIDTH = 200;
constexpr int HEIGHT = 120;

constexpr std::uint32_t RED = 0xffff0000;
constexpr std::uint32_t BLUE = 0xff0000ff;

class Tracked final : public ttk::Widget {
public:
    explicit Tracked(bool *gone) : _gone(gone) {}

    ~Tracked() override { *_gone = true; }

    Tracked(const Tracked &) = delete;
    Tracked &operator=(const Tracked &) = delete;
    Tracked(Tracked &&) = delete;
    Tracked &operator=(Tracked &&) = delete;

private:
    bool *_gone;
};

class Sized final : public ttk::Widget {
public:
    Sized(const double wide, const double tall) : _wide(wide), _tall(tall) {}

    double natural_width(ttk::Typeface & /*type*/) override { return _wide; }

    double natural_height(ttk::Typeface & /*type*/, const double width) override {
        asked = width;

        return _tall;
    }

    double asked = -1.0;

private:
    double _wide;
    double _tall;
};

class Hooked final : public ttk::Widget {
public:
    std::vector<std::string> calls;

    void arrange(ttk::Typeface &type) override {
        calls.emplace_back("arrange");
        Widget::arrange(type);
    }

protected:
    void moved() override { calls.emplace_back("moved"); }

    void restyle() override { calls.emplace_back("restyle"); }
};

class Restyling {
public:
    Restyling() : _was(ttk::Theme::mode()) {
        ttk::Theme::set_mode(ttk::Theme::Mode::Light);
        ttk::Theme::set_mode(ttk::Theme::Mode::Dark);
    }

    ~Restyling() { ttk::Theme::set_mode(_was); }

    Restyling(const Restyling &) = delete;
    Restyling &operator=(const Restyling &) = delete;
    Restyling(Restyling &&) = delete;
    Restyling &operator=(Restyling &&) = delete;

private:
    ttk::Theme::Mode _was;
};

class Attached : public ::testing::Test {
protected:
    ttk::Root root{tests::fonts()};

    void SetUp() override {
        root.resize(WIDTH, HEIGHT);
        root.settle();
        root.take();
    }

    tests::Probe *popup(const BLRect &box, const char *name = "probe") {
        return tests::mount(root.layer(ttk::Root::POPUPS), box, tests::fonts(), name);
    }
};

std::uint32_t pixel(const BLImage &image, const int x, const int y) {
    BLImageData data{};

    if (image.get_data(&data) != BL_SUCCESS) {
        return 0;
    }

    return static_cast<const std::uint32_t *>(data.pixel_data)[(static_cast<std::ptrdiff_t>(y) * data.stride / 4) + x];
}

BLImage painted(ttk::Widget &widget, const BLRectI &clip) {
    BLImage image(WIDTH, HEIGHT, BL_FORMAT_PRGB32);
    BLContext context(image);

    context.clear_all();

    {
        const ttk::Painter painter(context, tests::fonts(), clip);

        widget.paint(painter);
    }

    context.end();

    return image;
}

TEST(Widget, StartsDetachedAndAlone) {
    const ttk::Widget widget;

    EXPECT_EQ(widget.parent(), nullptr);
    EXPECT_EQ(widget.root(), nullptr);
    EXPECT_TRUE(widget.children().empty());
    EXPECT_TRUE(tests::same(widget.box(), BLRect{}));
}

TEST(Widget, StartsVisibleEnabledAndIdle) {
    const ttk::Widget widget;

    EXPECT_TRUE(widget.visible());
    EXPECT_TRUE(widget.enabled());
    EXPECT_FALSE(widget.hovered());
    EXPECT_FALSE(widget.pressed());
    EXPECT_FALSE(widget.focused());
    EXPECT_FALSE(widget.holds_pointer());
    EXPECT_EQ(widget.tab_index(), 0);
    EXPECT_EQ(widget.cursor, ttk::Cursor::Default);
    EXPECT_TRUE(widget.hint.empty());
}

TEST(Widget, SizingFieldsStartUnset) {
    const ttk::Widget widget;

    EXPECT_EQ(widget.stretch, 0.0);
    EXPECT_LT(widget.fixedWidth, 0.0);
    EXPECT_LT(widget.fixedHeight, 0.0);
    EXPECT_EQ(widget.minWidth, 0.0);
    EXPECT_EQ(widget.minHeight, 0.0);
}

TEST(Widget, DefaultHandlersTakeNothing) {
    ttk::Widget widget;

    EXPECT_FALSE(widget.press({.x = 1, .y = 1}));
    EXPECT_FALSE(widget.wheel(1.0, {.x = 1, .y = 1}));
    EXPECT_FALSE(widget.key({.code = 'a'}));
    EXPECT_FALSE(widget.advance(1.0));
    EXPECT_FALSE(widget.takes_focus()) << "a plain container is not a Tab stop";
    EXPECT_FALSE(widget.paints_focus());

    BLRect region{1, 2, 3, 4};

    EXPECT_FALSE(widget.clips(region));
}

TEST(Widget, DefaultHandlersWithoutAReactionAreSafe) {
    ttk::Widget widget;

    widget.drag({});
    widget.release({});
    widget.hover({});
    widget.within(true);
    widget.wrote("text");
    widget.reveal({0, 0, 1, 1});
    widget.gained_focus();
    widget.lost_focus();

    EXPECT_FALSE(widget.focused());
}

TEST(Widget, CursorAtDefaultsToTheCursor) {
    ttk::Widget widget;

    widget.cursor = ttk::Cursor::Resize;

    EXPECT_EQ(widget.cursor_at(0, 0), ttk::Cursor::Resize);
    EXPECT_EQ(widget.cursor_at(-100, 1e6), ttk::Cursor::Resize);
}

TEST(Widget, AControlIsATabStopWhileEnabled) {
    tests::Probe control;

    EXPECT_TRUE(control.takes_focus());

    control.set_enabled(false);

    EXPECT_FALSE(control.takes_focus());
}

TEST(Widget, SetTabIndexIsRead) {
    ttk::Widget widget;

    widget.set_tab_index(7);

    EXPECT_EQ(widget.tab_index(), 7);

    widget.set_tab_index(-2);

    EXPECT_EQ(widget.tab_index(), -2);
}

TEST(Widget, AddAppendsAndReturnsTheChild) {
    ttk::Widget parent;
    auto first = std::make_unique<ttk::Widget>();
    auto second = std::make_unique<ttk::Widget>();
    ttk::Widget *one = first.get();
    ttk::Widget *two = second.get();

    EXPECT_EQ(parent.add(std::move(first)), one);
    EXPECT_EQ(parent.add(std::move(second)), two);
    ASSERT_EQ(parent.children().size(), 2U);
    EXPECT_EQ(parent.children()[0].get(), one);
    EXPECT_EQ(parent.children()[1].get(), two);
    EXPECT_EQ(one->parent(), &parent);
    EXPECT_EQ(two->parent(), &parent);
}

TEST(Widget, AppendReturnsTheChildAsItsOwnType) {
    ttk::Widget parent;
    tests::Probe *probe = parent.append(std::make_unique<tests::Probe>("typed"));

    EXPECT_EQ(probe->name, "typed");
    EXPECT_EQ(probe->parent(), &parent);
    EXPECT_EQ(parent.children().back().get(), probe);
}

TEST(Widget, AChildOfADetachedParentIsDetached) {
    ttk::Widget parent;
    const ttk::Widget *child = parent.add(std::make_unique<ttk::Widget>());

    EXPECT_EQ(child->root(), nullptr);
}

TEST_F(Attached, AChildAddedToAnAttachedParentGetsTheRoot) {
    auto made = std::make_unique<ttk::Widget>();
    const ttk::Widget *grandchild = made->add(std::make_unique<ttk::Widget>());
    const ttk::Widget *child = root.content()->add(std::move(made));

    EXPECT_EQ(child->root(), &root);
    EXPECT_EQ(grandchild->root(), &root) << "the subtree added was not attached all the way down";
}

TEST_F(Attached, ARootReachesWidgetsAddedOnEveryLayer) {
    for (std::size_t index = 0; index < ttk::Root::LAYERS; ++index) {
        const ttk::Widget *child = root.layer(index)->add(std::make_unique<ttk::Widget>());

        EXPECT_EQ(child->root(), &root) << "layer " << index;
    }
}

TEST(Widget, EraseDestroysADirectChild) {
    bool gone = false;
    ttk::Widget parent;
    const ttk::Widget *child = parent.add(std::make_unique<Tracked>(&gone));

    parent.add(std::make_unique<ttk::Widget>());
    parent.erase(child);

    EXPECT_TRUE(gone);
    EXPECT_EQ(parent.children().size(), 1U);
}

TEST(Widget, EraseDestroysTheChildsSubtree) {
    bool gone = false;
    ttk::Widget parent;
    ttk::Widget *child = parent.add(std::make_unique<ttk::Widget>());

    child->add(std::make_unique<Tracked>(&gone));
    parent.erase(child);

    EXPECT_TRUE(gone);
}

TEST(Widget, EraseOfAGrandchildDoesNothing) {
    bool gone = false;
    ttk::Widget parent;
    ttk::Widget *child = parent.add(std::make_unique<ttk::Widget>());
    const ttk::Widget *grandchild = child->add(std::make_unique<Tracked>(&gone));

    parent.erase(grandchild);

    EXPECT_FALSE(gone);
    EXPECT_EQ(child->children().size(), 1U);
}

TEST(Widget, EraseOfAStrangerOrNullDoesNothing) {
    ttk::Widget parent;
    const ttk::Widget stranger;

    parent.add(std::make_unique<ttk::Widget>());
    parent.erase(&stranger);
    parent.erase(nullptr);
    parent.erase(&parent);

    EXPECT_EQ(parent.children().size(), 1U);
}

TEST(Widget, EraseKeepsTheOrderOfTheRest) {
    ttk::Widget parent;
    ttk::Widget *one = parent.add(std::make_unique<ttk::Widget>());
    const ttk::Widget *two = parent.add(std::make_unique<ttk::Widget>());
    ttk::Widget *three = parent.add(std::make_unique<ttk::Widget>());

    parent.erase(two);

    ASSERT_EQ(parent.children().size(), 2U);
    EXPECT_EQ(parent.children()[0].get(), one);
    EXPECT_EQ(parent.children()[1].get(), three);
}

TEST(Widget, ClearDestroysEveryChild) {
    ttk::Widget parent;
    bool one = false;
    bool two = false;

    parent.add(std::make_unique<Tracked>(&one));
    parent.add(std::make_unique<Tracked>(&two));
    parent.clear();

    EXPECT_TRUE(one);
    EXPECT_TRUE(two);
    EXPECT_TRUE(parent.children().empty());
}

TEST(Widget, ClearOfAChildlessWidgetIsSafe) {
    ttk::Widget parent;

    parent.clear();

    EXPECT_TRUE(parent.children().empty());
}

TEST_F(Attached, EraseForgetsWhatTheRootHeld) {
    tests::Probe *field = popup({10, 10, 40, 20});

    field->frames = 10;
    field->wake();
    root.focus(field);
    root.layer(ttk::Root::POPUPS)->erase(field);

    EXPECT_EQ(root.focused(), nullptr);
    EXPECT_FALSE(root.busy());
}

TEST_F(Attached, ClearForgetsWhatTheRootHeld) {
    tests::Probe *field = popup({10, 10, 40, 20});

    root.wake_at(field, 3.0);
    root.motion(20, 15);
    root.layer(ttk::Root::POPUPS)->clear();

    EXPECT_EQ(root.hovered(), nullptr);
    EXPECT_EQ(root.waking(), -1.0);
}

TEST(Widget, PlaceSetsTheBoxThenMovesThenArranges) {
    tests::Probe probe;

    probe.place({1.5, 2.5, 30, 40}, tests::fonts());

    EXPECT_TRUE(tests::same(probe.box(), BLRect{1.5, 2.5, 30, 40}));
    EXPECT_EQ(probe.moves, 1);
    EXPECT_EQ(probe.arranged, 1);
}

TEST(Widget, PlaceCallsMovedBeforeArrange) {
    Hooked widget;

    widget.place({0, 0, 10, 10}, tests::fonts());

    EXPECT_EQ(widget.calls, (std::vector<std::string>{"moved", "arrange"}));
}

TEST_F(Attached, AttachingAfterAThemeChangeRestyles) {
    auto made = std::make_unique<Hooked>();
    const Hooked *widget = made.get();
    const Restyling change;

    root.content()->add(std::move(made));

    EXPECT_EQ(widget->calls, std::vector<std::string>{"restyle"});
}

TEST_F(Attached, AttachingWithoutAThemeChangeDoesNotRestyle) {
    const Hooked *widget = root.content()->append(std::make_unique<Hooked>());

    EXPECT_TRUE(widget->calls.empty());
}

TEST_F(Attached, ALayoutAfterAThemeChangeRestylesTheTreeOnce) {
    Hooked *widget = root.content()->append(std::make_unique<Hooked>());
    const Restyling change;

    root.settle();
    root.relayout();
    root.settle();

    EXPECT_EQ(std::ranges::count(widget->calls, std::string("restyle")), 1);
}

TEST(Widget, DefaultArrangeStacksEveryChildOnTheBox) {
    ttk::Widget parent;
    const ttk::Widget *one = parent.add(std::make_unique<ttk::Widget>());
    ttk::Widget *two = parent.add(std::make_unique<ttk::Widget>());
    const ttk::Widget *deep = two->add(std::make_unique<ttk::Widget>());

    parent.place({10, 20, 30, 40}, tests::fonts());

    EXPECT_TRUE(tests::same(one->box(), BLRect{10, 20, 30, 40}));
    EXPECT_TRUE(tests::same(two->box(), BLRect{10, 20, 30, 40}));
    EXPECT_TRUE(tests::same(deep->box(), BLRect{10, 20, 30, 40}));
}

TEST(Widget, DefaultArrangePlacesHiddenChildrenToo) {
    ttk::Widget parent;
    ttk::Widget *hidden = parent.add(std::make_unique<ttk::Widget>());

    hidden->set_visible(false);
    parent.place({10, 20, 30, 40}, tests::fonts());

    EXPECT_TRUE(tests::same(hidden->box(), BLRect{10, 20, 30, 40}));
}

TEST(Widget, AChildlessWidgetWantsNothing) {
    ttk::Widget widget;

    EXPECT_EQ(widget.natural_width(tests::fonts()), 0.0);
    EXPECT_EQ(widget.natural_height(tests::fonts(), 100), 0.0);
    EXPECT_EQ(widget.wanted_width(tests::fonts()), 0.0);
    EXPECT_EQ(widget.wanted_height(tests::fonts(), 100), 0.0);
}

TEST(Widget, NaturalSizeIsTheLargestVisibleChild) {
    ttk::Widget parent;

    parent.add(std::make_unique<Sized>(30, 10));
    parent.add(std::make_unique<Sized>(20, 50));

    EXPECT_EQ(parent.natural_width(tests::fonts()), 30.0);
    EXPECT_EQ(parent.natural_height(tests::fonts(), 100), 50.0);
}

TEST(Widget, NaturalSizeIgnoresHiddenChildren) {
    ttk::Widget parent;

    parent.add(std::make_unique<Sized>(30, 10));
    parent.add(std::make_unique<Sized>(90, 90))->set_visible(false);

    EXPECT_EQ(parent.natural_width(tests::fonts()), 30.0);
    EXPECT_EQ(parent.natural_height(tests::fonts(), 100), 10.0);
}

TEST(Widget, NaturalHeightAsksChildrenAtTheSameWidth) {
    ttk::Widget parent;
    const Sized *child = parent.append(std::make_unique<Sized>(30, 10));

    parent.natural_height(tests::fonts(), 77);

    EXPECT_EQ(child->asked, 77.0);
}

TEST(Widget, FixedSizeOverridesTheNaturalSize) {
    ttk::Widget parent;

    parent.add(std::make_unique<Sized>(30, 10));
    parent.fixedWidth = 5;
    parent.fixedHeight = 0;

    EXPECT_EQ(parent.natural_width(tests::fonts()), 5.0);
    EXPECT_EQ(parent.natural_height(tests::fonts(), 100), 0.0) << "zero is a fixed size, not unset";
    EXPECT_EQ(parent.wanted_width(tests::fonts()), 5.0);
    EXPECT_EQ(parent.wanted_height(tests::fonts(), 100), 0.0);
}

TEST(Widget, WantedSizeIsRaisedToTheFloor) {
    ttk::Widget parent;

    parent.add(std::make_unique<Sized>(30, 10));
    parent.minWidth = 50;
    parent.minHeight = 25;

    EXPECT_EQ(parent.wanted_width(tests::fonts()), 50.0);
    EXPECT_EQ(parent.wanted_height(tests::fonts(), 100), 25.0);
    EXPECT_EQ(parent.natural_width(tests::fonts()), 30.0) << "the floor changed the natural size";
}

TEST(Widget, WantedSizeAboveTheFloorIsTheNaturalSize) {
    ttk::Widget parent;

    parent.add(std::make_unique<Sized>(30, 10));
    parent.minWidth = 20;
    parent.minHeight = 5;

    EXPECT_EQ(parent.wanted_width(tests::fonts()), 30.0);
    EXPECT_EQ(parent.wanted_height(tests::fonts(), 100), 10.0);
}

TEST(Widget, FixedSizeWinsOverTheFloor) {
    ttk::Widget widget;

    widget.fixedWidth = 10;
    widget.fixedHeight = 4;
    widget.minWidth = 50;
    widget.minHeight = 40;

    EXPECT_EQ(widget.wanted_width(tests::fonts()), 10.0);
    EXPECT_EQ(widget.wanted_height(tests::fonts(), 100), 4.0);
}

TEST(Widget, DrawnAndLitBoxDefaultToTheBox) {
    ttk::Widget widget;

    widget.place({3, 4, 5, 6}, tests::fonts());

    EXPECT_TRUE(tests::same(widget.drawn(), BLRect{3, 4, 5, 6}));
    EXPECT_TRUE(tests::same(widget.lit_box(), BLRect{3, 4, 5, 6}));
}

TEST(Widget, LitBoxFollowsDrawn) {
    tests::Probe probe;

    probe.place({10, 10, 5, 5}, tests::fonts());
    probe.spread = true;
    probe.spreadBox = {8, 8, 9, 9};

    EXPECT_TRUE(tests::same(probe.lit_box(), BLRect{8, 8, 9, 9}));
}

TEST(Widget, HoldsIsHalfOpen) {
    ttk::Widget widget;

    widget.place({10, 20, 30, 40}, tests::fonts());

    EXPECT_TRUE(widget.holds(10, 20));
    EXPECT_TRUE(widget.holds(39.9, 59.9));
    EXPECT_FALSE(widget.holds(40, 30));
    EXPECT_FALSE(widget.holds(20, 60));
    EXPECT_FALSE(widget.holds(9.9, 30));
}

TEST(Widget, AnEmptyBoxHoldsNothing) {
    ttk::Widget widget;

    widget.place({10, 20, 0, 0}, tests::fonts());

    EXPECT_FALSE(widget.holds(10, 20));
}

TEST(Widget, AtFindsTheDeepestWidgetThatTakesThePointer) {
    tests::Probe outer;

    outer.place({0, 0, 100, 100}, tests::fonts());

    auto *inner = tests::mount(&outer, {10, 10, 50, 50}, tests::fonts(), "inner");
    auto *leaf = tests::mount(inner, {20, 20, 10, 10}, tests::fonts(), "leaf");

    EXPECT_EQ(outer.at(25, 25), leaf);
    EXPECT_EQ(outer.at(15, 15), inner);
    EXPECT_EQ(outer.at(80, 80), &outer);
    EXPECT_EQ(outer.at(150, 150), nullptr);
}

TEST(Widget, AtPassesThroughAPlainContainer) {
    ttk::Widget container;

    container.place({0, 0, 100, 100}, tests::fonts());

    EXPECT_EQ(container.at(50, 50), nullptr);
}

TEST(Widget, AtTestsTheLastChildFirst) {
    ttk::Widget container;

    container.place({0, 0, 100, 100}, tests::fonts());
    tests::mount(&container, {0, 0, 60, 60}, tests::fonts(), "under");

    auto *over = tests::mount(&container, {40, 40, 60, 60}, tests::fonts(), "over");

    EXPECT_EQ(container.at(50, 50), over);
}

TEST(Widget, AtSkipsHiddenWidgetsAndTheirChildren) {
    ttk::Widget container;

    container.place({0, 0, 100, 100}, tests::fonts());

    auto *hidden = tests::mount(&container, {0, 0, 60, 60}, tests::fonts(), "hidden");

    tests::mount(hidden, {0, 0, 30, 30}, tests::fonts(), "inside");
    hidden->set_visible(false);

    EXPECT_EQ(container.at(10, 10), nullptr);
}

TEST(Widget, AtSkipsADisabledWidget) {
    ttk::Widget container;

    container.place({0, 0, 100, 100}, tests::fonts());

    auto *under = tests::mount(&container, {0, 0, 60, 60}, tests::fonts(), "under");
    auto *disabled = tests::mount(&container, {0, 0, 60, 60}, tests::fonts(), "disabled");

    disabled->set_enabled(false);

    EXPECT_EQ(container.at(10, 10), under) << "a disabled widget blocked the one under it";
}

TEST(Widget, AtFindsAWidgetWithAHint) {
    ttk::Widget container;

    container.place({0, 0, 100, 100}, tests::fonts());
    container.hint = "Explains the area";

    EXPECT_EQ(container.at(50, 50), &container);

    container.set_enabled(false);

    EXPECT_EQ(container.at(50, 50), nullptr);
}

TEST(Widget, AtHonoursTheClipRegion) {
    tests::Probe viewport;

    viewport.place({0, 0, 100, 100}, tests::fonts());
    viewport.clipping = true;
    viewport.clipRegion = {0, 0, 100, 50};

    auto *row = tests::mount(&viewport, {0, 40, 100, 30}, tests::fonts(), "row");

    EXPECT_EQ(viewport.at(10, 45), row);
    EXPECT_EQ(viewport.at(10, 55), nullptr) << "a row scrolled out of view was hit";
}

TEST(Widget, NowIsZeroWhileDetached) {
    const ttk::Widget widget;

    EXPECT_EQ(widget.now(), 0.0);
}

TEST(Widget, WakeAndSleepUntilWhileDetachedAreSafe) {
    const ttk::Widget widget;

    widget.wake();

    EXPECT_FALSE(widget.sleep_until(5.0));
}

TEST_F(Attached, SleepUntilSchedulesAndReturnsFalse) {
    const tests::Probe *caret = popup({10, 10, 2, 14});

    EXPECT_FALSE(caret->sleep_until(2.5));
    EXPECT_EQ(root.waking(), 2.5);
    EXPECT_FALSE(root.busy());
}

TEST_F(Attached, NowReadsTheRootsClock) {
    const tests::Probe *clock = popup({10, 10, 2, 14});

    root.set_now(3.75);

    EXPECT_EQ(clock->now(), 3.75);
}

TEST(Widget, InvalidateWhileDetachedDoesNothing) {
    ttk::Widget parent;
    ttk::Widget *child = parent.add(std::make_unique<ttk::Widget>());

    child->place({0, 0, 10, 10}, tests::fonts());
    child->invalidate();
    child->invalidate({0, 0, 5, 5});
    child->invalidate({0, 0, 5, 5}, {0, 0, 2, 2});

    EXPECT_EQ(child->root(), nullptr);
}

TEST_F(Attached, InvalidateDamagesTheDrawnArea) {
    tests::Probe *shadowed = popup({20, 20, 10, 10});

    shadowed->spread = true;
    shadowed->spreadBox = {15, 15, 25, 25};
    root.take();
    shadowed->invalidate();

    const std::vector<BLRect> &taken = root.take();

    ASSERT_EQ(taken.size(), 1U);
    EXPECT_TRUE(tests::same(taken[0], BLRect{15, 15, 25, 25}));
}

TEST_F(Attached, InvalidateRegionDamagesThatRegion) {
    const tests::Probe *probe = popup({20, 20, 50, 50});

    root.take();
    probe->invalidate({100, 80, 5, 6});

    const std::vector<BLRect> &taken = root.take();

    ASSERT_EQ(taken.size(), 1U);
    EXPECT_TRUE(tests::same(taken[0], BLRect{100, 80, 5, 6})) << "a region need not lie in the box";
}

TEST_F(Attached, InvalidateIsClippedByAScrollingAncestor) {
    tests::Probe *viewport = popup({0, 0, 50, 50}, "viewport");
    const tests::Probe *row = tests::mount(viewport, {0, 0, 50, 200}, tests::fonts(), "row");

    viewport->clipping = true;
    viewport->clipRegion = {0, 0, 50, 50};
    root.take();
    row->invalidate({25, 25, 50, 50});

    const std::vector<BLRect> &taken = root.take();

    ASSERT_EQ(taken.size(), 1U);
    EXPECT_TRUE(tests::same(taken[0], BLRect{25, 25, 25, 25}));
}

TEST_F(Attached, InvalidateOutsideAScrollingAncestorDamagesNothing) {
    tests::Probe *viewport = popup({0, 0, 50, 50}, "viewport");
    const tests::Probe *row = tests::mount(viewport, {0, 100, 50, 20}, tests::fonts(), "row");

    viewport->clipping = true;
    viewport->clipRegion = {0, 0, 50, 50};
    root.take();
    row->invalidate();

    EXPECT_FALSE(root.dirty()) << "a row scrolled out of view was repainted";
}

TEST_F(Attached, InvalidateIsClippedByEveryScrollingAncestor) {
    tests::Probe *outer = popup({0, 0, 100, 100}, "outer");
    tests::Probe *inner = tests::mount(outer, {0, 0, 100, 100}, tests::fonts(), "inner");
    const tests::Probe *row = tests::mount(inner, {0, 0, 100, 100}, tests::fonts(), "row");

    outer->clipping = true;
    outer->clipRegion = {0, 0, 60, 100};
    inner->clipping = true;
    inner->clipRegion = {0, 30, 100, 70};
    root.take();
    row->invalidate();

    const std::vector<BLRect> &taken = root.take();

    ASSERT_EQ(taken.size(), 1U);
    EXPECT_TRUE(tests::same(taken[0], BLRect{0, 30, 60, 70}));
}

TEST_F(Attached, InvalidateIgnoresTheWidgetsOwnClip) {
    tests::Probe *viewport = popup({0, 0, 50, 50}, "viewport");

    viewport->clipping = true;
    viewport->clipRegion = {0, 0, 10, 10};
    root.take();
    viewport->invalidate();

    const std::vector<BLRect> &taken = root.take();

    ASSERT_EQ(taken.size(), 1U);
    EXPECT_TRUE(tests::same(taken[0], BLRect{0, 0, 50, 50}));
}

TEST_F(Attached, InvalidateWithAClipDamagesOnlyThePartInside) {
    const tests::Probe *probe = popup({0, 0, 100, 100});

    root.take();
    probe->invalidate({10, 10, 50, 50}, {30, 0, 100, 40});

    const std::vector<BLRect> &taken = root.take();

    ASSERT_EQ(taken.size(), 1U);
    EXPECT_TRUE(tests::same(taken[0], BLRect{30, 10, 30, 30}));
}

TEST_F(Attached, InvalidateWithADisjointClipDamagesNothing) {
    const tests::Probe *probe = popup({0, 0, 100, 100});

    root.take();
    probe->invalidate({10, 10, 20, 20}, {30, 0, 100, 40});
    probe->invalidate({10, 10, 20, 20}, {0, 0, 0, 0});

    EXPECT_FALSE(root.dirty());
}

TEST_F(Attached, InvalidateWithAClipIsAlsoClippedByAncestors) {
    tests::Probe *viewport = popup({0, 0, 50, 50}, "viewport");
    const tests::Probe *row = tests::mount(viewport, {0, 0, 50, 200}, tests::fonts(), "row");

    viewport->clipping = true;
    viewport->clipRegion = {0, 0, 50, 50};
    root.take();
    row->invalidate({0, 0, 100, 100}, {20, 20, 100, 100});

    const std::vector<BLRect> &taken = root.take();

    ASSERT_EQ(taken.size(), 1U);
    EXPECT_TRUE(tests::same(taken[0], BLRect{20, 20, 30, 30}));
}

TEST_F(Attached, HidingRepaintsAndAsksForALayout) {
    tests::Probe *probe = popup({10, 10, 20, 20});

    root.take();
    probe->set_visible(false);

    EXPECT_FALSE(probe->visible());
    EXPECT_TRUE(tests::covers(root.take(), 10, 10)) << "hiding did not repaint where it was";
    EXPECT_TRUE(root.settle()) << "hiding did not ask for a layout";
}

TEST_F(Attached, ShowingAsksForALayout) {
    tests::Probe *probe = popup({10, 10, 20, 20});

    probe->set_visible(false);
    root.settle();
    root.take();
    probe->set_visible(true);

    EXPECT_TRUE(probe->visible());
    EXPECT_TRUE(root.settle());
}

TEST_F(Attached, SetVisibleToTheSameValueDoesNothing) {
    tests::Probe *probe = popup({10, 10, 20, 20});

    root.take();
    probe->set_visible(true);

    EXPECT_FALSE(root.dirty());
}

TEST_F(Attached, DisablingRepaintsWithoutALayout) {
    tests::Probe *probe = popup({10, 10, 20, 20});

    root.take();
    probe->set_enabled(false);

    EXPECT_FALSE(probe->enabled());
    EXPECT_TRUE(tests::covers(root.take(), 29, 29));
    EXPECT_FALSE(root.settle()) << "enabling is no reason to lay out again";

    probe->set_enabled(true);

    EXPECT_TRUE(probe->enabled());
    EXPECT_TRUE(tests::covers(root.take(), 10, 10));
}

TEST_F(Attached, SetEnabledToTheSameValueDoesNothing) {
    tests::Probe *probe = popup({10, 10, 20, 20});

    root.take();
    probe->set_enabled(true);

    EXPECT_FALSE(root.dirty());
}

TEST(Widget, VisibilityTogglesWhileDetachedAreKept) {
    ttk::Widget widget;

    widget.set_visible(false);
    widget.set_enabled(false);

    EXPECT_FALSE(widget.visible());
    EXPECT_FALSE(widget.enabled());
}

TEST_F(Attached, EnterSetsHoveredAndRepaintsTheLitBox) {
    tests::Probe *probe = popup({10, 10, 20, 20});

    probe->spread = true;
    probe->spreadBox = {5, 5, 30, 30};
    root.take();
    probe->enter();

    EXPECT_TRUE(probe->hovered());

    const std::vector<BLRect> &taken = root.take();

    ASSERT_EQ(taken.size(), 1U);
    EXPECT_TRUE(tests::same(taken[0], BLRect{5, 5, 30, 30}));
}

TEST_F(Attached, LeaveClearsHoveredAndPressed) {
    tests::Probe *probe = popup({10, 10, 20, 20});

    root.press({.x = 15, .y = 15});
    ASSERT_TRUE(probe->pressed());
    root.grab(nullptr);
    root.take();
    probe->leave();

    EXPECT_FALSE(probe->hovered());
    EXPECT_FALSE(probe->pressed());
    EXPECT_TRUE(tests::covers(root.take(), 10, 10));
}

TEST_F(Attached, FocusChangesRepaintTheWidget) {
    tests::Probe *probe = popup({10, 10, 20, 20});

    root.focus(probe);

    EXPECT_TRUE(probe->focused());
    EXPECT_TRUE(tests::covers(root.take(), 10, 10));

    root.focus(nullptr);

    EXPECT_FALSE(probe->focused());
    EXPECT_TRUE(tests::covers(root.take(), 10, 10));
}

TEST_F(Attached, HoldsPointerWhileTheHoverIsInside) {
    tests::Probe *card = popup({0, 0, 100, 100}, "card");
    const tests::Probe *button = tests::mount(card, {10, 10, 20, 20}, tests::fonts(), "button");

    root.motion(15, 15);

    EXPECT_TRUE(card->holds_pointer());
    EXPECT_TRUE(button->holds_pointer());
    EXPECT_FALSE(card->hovered());

    root.motion(150, 110);

    EXPECT_FALSE(card->holds_pointer());
}

TEST_F(Attached, WakeMakesTheWidgetLive) {
    const tests::Probe *probe = popup({10, 10, 20, 20});

    probe->wake();

    EXPECT_TRUE(root.busy());
}

TEST(Widget, PaintDrawsVisibleChildrenInOrder) {
    ttk::Widget container;

    container.place({0, 0, WIDTH, HEIGHT}, tests::fonts());
    tests::mount(&container, {10, 10, 50, 50}, tests::fonts())->tone = BLRgba32(RED);
    tests::mount(&container, {30, 30, 50, 50}, tests::fonts())->tone = BLRgba32(BLUE);

    const BLImage image = painted(container, {0, 0, WIDTH, HEIGHT});

    EXPECT_EQ(pixel(image, 15, 15), RED);
    EXPECT_EQ(pixel(image, 40, 40), BLUE) << "the last child is painted on top";
    EXPECT_EQ(pixel(image, 100, 100), 0U);
}

TEST(Widget, PaintSkipsHiddenChildren) {
    ttk::Widget container;

    container.place({0, 0, WIDTH, HEIGHT}, tests::fonts());

    tests::Probe *hidden = tests::mount(&container, {10, 10, 50, 50}, tests::fonts());

    hidden->tone = BLRgba32(RED);
    hidden->set_visible(false);

    const BLImage image = painted(container, {0, 0, WIDTH, HEIGHT});

    EXPECT_EQ(hidden->painted, 0);
    EXPECT_EQ(pixel(image, 15, 15), 0U);
}

TEST(Widget, PaintSkipsChildrenOutsideTheClip) {
    ttk::Widget container;

    container.place({0, 0, WIDTH, HEIGHT}, tests::fonts());

    const tests::Probe *away = tests::mount(&container, {10, 10, 20, 20}, tests::fonts(), "away");
    const tests::Probe *near = tests::mount(&container, {100, 50, 20, 20}, tests::fonts(), "near");

    painted(container, {90, 40, 20, 20});

    EXPECT_EQ(away->painted, 0);
    EXPECT_EQ(near->painted, 1);
}

TEST(Widget, PaintCullsByWhereAChildDraws) {
    ttk::Widget container;

    container.place({0, 0, WIDTH, HEIGHT}, tests::fonts());

    tests::Probe *shadowed = tests::mount(&container, {10, 10, 20, 20}, tests::fonts());

    shadowed->spread = true;
    shadowed->spreadBox = {0, 0, 40, 40};
    painted(container, {35, 35, 10, 10});

    EXPECT_EQ(shadowed->painted, 1) << "a shadow reaching into the clip was culled";
}

TEST(Widget, PaintClipsAChildToItsClipRegion) {
    ttk::Widget container;

    container.place({0, 0, WIDTH, HEIGHT}, tests::fonts());

    tests::Probe *viewport = tests::mount(&container, {10, 10, 100, 100}, tests::fonts(), "viewport");
    tests::Probe *row = tests::mount(viewport, {10, 10, 100, 100}, tests::fonts(), "row");

    viewport->clipping = true;
    viewport->clipRegion = {20.5, 20.5, 30, 30};
    row->tone = BLRgba32(RED);

    const BLImage image = painted(container, {0, 0, WIDTH, HEIGHT});

    EXPECT_EQ(pixel(image, 15, 15), 0U) << "painted outside the clip region";
    EXPECT_EQ(pixel(image, 20, 20), RED) << "the clip was not rounded out to whole pixels";
    EXPECT_EQ(pixel(image, 50, 50), RED);
    EXPECT_EQ(pixel(image, 51, 51), 0U);
}

TEST(Widget, PaintRestoresTheClipAfterAClippingChild) {
    ttk::Widget container;

    container.place({0, 0, WIDTH, HEIGHT}, tests::fonts());

    tests::Probe *viewport = tests::mount(&container, {0, 0, 20, 20}, tests::fonts(), "viewport");

    viewport->clipping = true;
    viewport->clipRegion = {0, 0, 20, 20};
    tests::mount(&container, {100, 50, 20, 20}, tests::fonts(), "after")->tone = BLRgba32(BLUE);

    const BLImage image = painted(container, {0, 0, WIDTH, HEIGHT});

    EXPECT_EQ(pixel(image, 110, 60), BLUE) << "a sibling after a clipping child was still clipped";
}
}
