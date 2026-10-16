// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "support/Canvas.h"
#include "toolkit/Probe.h"
#include "ttk/draw/Damage.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Event.h"
#include "ttk/toolkit/Root.h"

namespace {
constexpr int WIDTH = 200;
constexpr int HEIGHT = 120;

constexpr BLRect WINDOW{0, 0, WIDTH, HEIGHT};

class Rooted : public ::testing::Test {
protected:
    ttk::Root root{tests::fonts()};

    void SetUp() override {
        root.resize(WIDTH, HEIGHT);
        clean();
    }

    void clean() {
        root.settle();
        root.take();
        root.take_shifts();
    }

    tests::Probe *popup(const BLRect &box, const char *name = "popup") {
        return tests::mount(root.layer(ttk::Root::POPUPS), box, tests::fonts(), name);
    }

    tests::Probe *page(const char *name = "page") {
        auto *made = root.content()->append(std::make_unique<tests::Probe>(name));

        root.relayout();
        clean();

        return made;
    }
};

std::vector<std::uint32_t> pixels(const BLImage &image) {
    BLImageData data{};
    std::vector<std::uint32_t> copy(static_cast<std::size_t>(image.width()) * static_cast<std::size_t>(image.height()));

    if (image.get_data(&data) != BL_SUCCESS) {
        return copy;
    }

    for (int y = 0; y < image.height(); ++y) {
        std::memcpy(copy.data() + (static_cast<std::ptrdiff_t>(y) * image.width()),
                    static_cast<const std::uint8_t *>(data.pixel_data) + (static_cast<std::ptrdiff_t>(y) * data.stride),
                    static_cast<std::size_t>(image.width()) * 4);
    }

    return copy;
}

std::uint32_t pixel(const BLImage &image, const int x, const int y) {
    return pixels(image)[(static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width()))
                         + static_cast<std::size_t>(x)];
}

BLImage painted(ttk::Root &root, const BLRectI &clip) {
    BLImage image(WIDTH, HEIGHT, BL_FORMAT_PRGB32);
    BLContext context(image);

    context.clear_all();
    root.paint(context, clip);
    context.end();

    return image;
}

TEST(Root, ContentIsOneWidgetForTheLifeOfTheRoot) {
    ttk::Root root(tests::fonts());

    ASSERT_NE(root.content(), nullptr);
    EXPECT_EQ(root.content(), root.content());
}

namespace {
// Repaints as it goes, as a widget that detaches a decorator in its destructor does.
class Leaving final : public ttk::Widget {
public:
    explicit Leaving(bool &rooted) : _rooted(rooted) {}

    ~Leaving() override {
        _rooted = root() != nullptr;
        invalidate();
    }

    Leaving(const Leaving &) = delete;
    Leaving &operator=(const Leaving &) = delete;
    Leaving(Leaving &&) = delete;
    Leaving &operator=(Leaving &&) = delete;

private:
    bool &_rooted;
};
}

TEST(Root, WidgetsTakenDownWithTheRootFindItGone) {
    bool onPage = true;
    bool onLayer = true;

    {
        ttk::Root root(tests::fonts());

        root.resize(100.0, 100.0);
        root.content()->append(std::make_unique<Leaving>(onPage));
        root.layer(ttk::Root::POPUPS)->append(std::make_unique<Leaving>(onLayer));
    }

    EXPECT_FALSE(onPage) << "a widget on the page still saw the root while it was taken down";
    EXPECT_FALSE(onLayer) << "a widget on a layer still saw the root while it was taken down";
}

TEST(Root, EveryLayerIsItsOwnWidget) {
    ttk::Root root(tests::fonts());

    for (std::size_t one = 0; one < ttk::Root::LAYERS; ++one) {
        EXPECT_NE(root.layer(one), root.content());

        for (std::size_t two = one + 1; two < ttk::Root::LAYERS; ++two) {
            EXPECT_NE(root.layer(one), root.layer(two)) << one << " and " << two;
        }
    }
}

TEST(Root, LayerIndexIsClampedToTheLast) {
    ttk::Root root(tests::fonts());

    EXPECT_EQ(root.layer(ttk::Root::LAYERS), root.layer(ttk::Root::LAYERS - 1));
    EXPECT_EQ(root.layer(1000), root.layer(ttk::Root::TIPS));
}

TEST(Root, LayerIndicesAreInPaintOrder) {
    EXPECT_EQ(ttk::Root::LAYERS, 4U);
    EXPECT_LT(ttk::Root::DIALOGS, ttk::Root::POPUPS);
    EXPECT_LT(ttk::Root::POPUPS, ttk::Root::NOTICES);
    EXPECT_LT(ttk::Root::NOTICES, ttk::Root::TIPS);
    EXPECT_LT(ttk::Root::TIPS, ttk::Root::LAYERS);
}

TEST(Root, TypeIsTheTypefaceItWasMadeWith) {
    const ttk::Root root(tests::fonts());

    EXPECT_EQ(&root.type(), &tests::fonts());
}

TEST(Root, ResizeKeepsTheSize) {
    ttk::Root root(tests::fonts());

    EXPECT_EQ(root.width(), 0.0);
    EXPECT_EQ(root.height(), 0.0);

    root.resize(321.5, 123.25);

    EXPECT_EQ(root.width(), 321.5);
    EXPECT_EQ(root.height(), 123.25);
}

TEST(Root, FreshRootIsDirtyUntilItSettles) {
    ttk::Root root(tests::fonts());

    EXPECT_TRUE(root.dirty()) << "a new root owes its first layout";

    root.resize(WIDTH, HEIGHT);

    EXPECT_TRUE(root.settle());
    EXPECT_FALSE(root.settle()) << "a second settle had nothing scheduled";
}

TEST_F(Rooted, ResizeToTheSameSizeSchedulesNothing) {
    root.resize(WIDTH, HEIGHT);

    EXPECT_FALSE(root.dirty());
    EXPECT_FALSE(root.settle());
}

TEST_F(Rooted, ResizeToANewSizeSchedulesALayout) {
    root.resize(WIDTH + 10, HEIGHT);

    EXPECT_TRUE(root.dirty());
    ASSERT_TRUE(root.settle());
    EXPECT_TRUE(tests::same(root.content()->box(), BLRect{0, 0, WIDTH + 10, HEIGHT}));
}

TEST_F(Rooted, ResizeDismissesTheOpenPopup) {
    int closed = 0;

    root.set_dismiss([&] { ++closed; });
    root.resize(WIDTH, HEIGHT);

    EXPECT_EQ(closed, 0) << "an unchanged size dismissed the popup";
    EXPECT_TRUE(root.has_dismiss());

    root.resize(WIDTH, HEIGHT + 1);

    EXPECT_EQ(closed, 1);
    EXPECT_FALSE(root.has_dismiss());
}

TEST_F(Rooted, RelayoutIsRunByTheNextSettle) {
    root.relayout();

    EXPECT_TRUE(root.dirty());
    EXPECT_TRUE(root.settle());
    EXPECT_FALSE(root.settle());
}

TEST_F(Rooted, SettleDamagesTheWholeWindow) {
    root.relayout();
    root.settle();

    EXPECT_TRUE(root.whole());

    const std::vector<BLRect> &taken = root.take();

    ASSERT_EQ(taken.size(), 1U);
    EXPECT_TRUE(tests::same(taken[0], WINDOW));
}

TEST_F(Rooted, SettlePlacesThePageAndEveryLayerOnTheWindow) {
    EXPECT_TRUE(tests::same(root.content()->box(), WINDOW));

    for (std::size_t index = 0; index < ttk::Root::LAYERS; ++index) {
        EXPECT_TRUE(tests::same(root.layer(index)->box(), WINDOW)) << "layer " << index;
    }
}

TEST(Root, ThePageAndLayersBelongToTheRootFromTheStart) {
    ttk::Root root(tests::fonts());

    EXPECT_EQ(root.content()->root(), &root);

    for (std::size_t index = 0; index < ttk::Root::LAYERS; ++index) {
        EXPECT_EQ(root.layer(index)->root(), &root) << "layer " << index;
    }
}

TEST(Root, WidgetsAddedBeforeTheFirstSettleBelongToTheRoot) {
    ttk::Root root(tests::fonts());
    auto *child = root.content()->append(std::make_unique<tests::Probe>());
    const auto *grandchild = child->append(std::make_unique<tests::Probe>());

    EXPECT_EQ(child->root(), &root);
    EXPECT_EQ(grandchild->root(), &root);
}

// A window syncs its state into the tree once before the first frame lays it out.
TEST(Root, AWakeBeforeTheFirstSettleIsKept) {
    ttk::Root root(tests::fonts());
    const auto *child = root.content()->append(std::make_unique<tests::Probe>());

    root.set_now(2.5);
    child->wake();

    EXPECT_TRUE(root.busy());
    EXPECT_DOUBLE_EQ(child->now(), 2.5);
}

TEST_F(Rooted, PageAndPlainLayersStretchTheirChildrenToTheWindow) {
    auto *content = root.content()->append(std::make_unique<tests::Probe>());
    auto *dialog = root.layer(ttk::Root::DIALOGS)->append(std::make_unique<tests::Probe>());
    auto *notice = root.layer(ttk::Root::NOTICES)->append(std::make_unique<tests::Probe>());
    auto *tip = root.layer(ttk::Root::TIPS)->append(std::make_unique<tests::Probe>());

    for (ttk::Widget *each : {
                 static_cast<ttk::Widget *>(content),
                 static_cast<ttk::Widget *>(dialog),
                 static_cast<ttk::Widget *>(notice),
                 static_cast<ttk::Widget *>(tip),
         }) {
        each->place({5, 5, 10, 10}, tests::fonts());
    }

    root.relayout();
    root.settle();

    EXPECT_TRUE(tests::same(content->box(), WINDOW));
    EXPECT_TRUE(tests::same(dialog->box(), WINDOW));
    EXPECT_TRUE(tests::same(notice->box(), WINDOW));
    EXPECT_TRUE(tests::same(tip->box(), WINDOW));
}

TEST_F(Rooted, PopupsKeepTheBoxTheyWerePlacedAt) {
    tests::Probe *menu = popup({30, 40, 50, 25});
    const auto *item = menu->append(std::make_unique<tests::Probe>());
    const int arranged = menu->arranged;

    root.relayout();
    root.settle();

    EXPECT_TRUE(tests::same(menu->box(), BLRect{30, 40, 50, 25}));
    EXPECT_GT(menu->arranged, arranged) << "the popup's insides were not laid out again";
    EXPECT_TRUE(tests::same(item->box(), BLRect{30, 40, 50, 25}));
}

TEST_F(Rooted, SettleMovesTheHoverOffAWidgetThatMovedAway) {
    tests::Probe *moving = popup({10, 10, 40, 40});

    root.motion(20, 20);
    ASSERT_EQ(root.hovered(), moving);

    moving->place({100, 60, 40, 40}, tests::fonts());
    root.relayout();
    root.settle();

    EXPECT_EQ(root.hovered(), nullptr);
    EXPECT_EQ(moving->leaves, 1);
    EXPECT_FALSE(moving->hovered());
}

TEST_F(Rooted, SettleMovesTheHoverOntoWhatIsNowUnderThePointer) {
    tests::Probe *moving = popup({10, 10, 40, 40}, "moving");
    tests::Probe *still = popup({100, 60, 40, 40}, "still");

    root.motion(20, 20);
    moving->place({150, 0, 40, 40}, tests::fonts());
    still->place({0, 0, 40, 40}, tests::fonts());
    root.relayout();
    root.settle();

    EXPECT_EQ(root.hovered(), still);
    EXPECT_EQ(still->enters, 1);
}

TEST_F(Rooted, SettleLeavesTheHoverAloneWhileAPressIsHeld) {
    tests::Probe *moving = popup({10, 10, 40, 40});

    root.motion(20, 20);
    root.press({.x = 20, .y = 20});
    ASSERT_EQ(root.grabbed(), moving);

    moving->place({100, 60, 40, 40}, tests::fonts());
    root.relayout();
    root.settle();

    EXPECT_EQ(root.hovered(), moving);
    EXPECT_EQ(moving->leaves, 0);
}

TEST_F(Rooted, AThemeChangeSchedulesALayout) {
    const ttk::Theme::Mode was = ttk::Theme::mode();

    ttk::Theme::set_mode(ttk::Theme::Mode::Light);
    ttk::Theme::set_mode(ttk::Theme::Mode::Dark);

    EXPECT_TRUE(root.settle()) << "the palette changed and nothing was laid out again";

    ttk::Theme::set_mode(was);
}

TEST_F(Rooted, CleanRootIsNotDirty) {
    EXPECT_FALSE(root.dirty());
    EXPECT_FALSE(root.whole());
    EXPECT_TRUE(root.take().empty());
    EXPECT_TRUE(root.take_shifts().empty());
}

TEST_F(Rooted, EmptyRegionsAreIgnored) {
    root.damage({10, 10, 0, 5});
    root.damage({10, 10, 5, 0});
    root.damage({10, 10, -5, 5});
    root.damage({10, 10, 5, -5});

    EXPECT_FALSE(root.dirty());
    EXPECT_TRUE(root.take().empty());
}

TEST_F(Rooted, DamageMakesTheRootDirty) {
    root.damage({10, 10, 1, 1});

    EXPECT_TRUE(root.dirty());

    root.take();

    EXPECT_FALSE(root.dirty());
}

TEST_F(Rooted, TakeRoundsRegionsOutToWholePixels) {
    root.damage({10.5, 20.25, 5, 5.5});

    const std::vector<BLRect> &taken = root.take();

    ASSERT_EQ(taken.size(), 1U);
    EXPECT_TRUE(tests::same(taken[0], BLRect{10, 20, 6, 6}));
}

TEST_F(Rooted, TakeClipsRegionsToTheWindow) {
    root.damage({-10, -10, 30, 30});
    root.damage({WIDTH - 5, HEIGHT - 5, 50, 50});

    const std::vector<BLRect> &taken = root.take();

    EXPECT_TRUE(tests::covers(taken, 0, 0));
    EXPECT_TRUE(tests::covers(taken, WIDTH - 1, HEIGHT - 1));
    EXPECT_EQ(tests::area(taken), (20.0 * 20.0) + (5.0 * 5.0));

    for (const BLRect &region : taken) {
        EXPECT_TRUE(tests::inside(region, WINDOW));
    }
}

TEST_F(Rooted, RegionsWhollyOutsideTheWindowAreDropped) {
    root.damage({WIDTH + 10, 0, 10, 10});
    root.damage({0, -50, 10, 10});

    EXPECT_TRUE(root.take().empty());
}

TEST(Root, RegionsGivenBeforeTheFirstSizeAreKept) {
    ttk::Root root(tests::fonts());

    root.damage({5, 5, 10, 10});

    EXPECT_TRUE(root.dirty());

    root.resize(WIDTH, HEIGHT);

    const std::vector<BLRect> &taken = root.take();

    ASSERT_EQ(taken.size(), 1U);
    EXPECT_TRUE(tests::same(taken[0], BLRect{5, 5, 10, 10}));
}

TEST(Root, ManyRegionsGivenBeforeTheFirstSizeAreAllKept) {
    ttk::Root root(tests::fonts());
    std::vector<BLPoint> given;

    for (int x = 0; given.size() < ttk::Damage::CROWDED + 8; x += 4) {
        root.damage({static_cast<double>(x), 10, 1, 1});
        given.emplace_back(x, 10);
    }

    root.resize(WIDTH, HEIGHT);

    const std::vector<BLRect> &taken = root.take();

    for (const BLPoint &point : given) {
        ASSERT_TRUE(tests::covers(taken, point.x, point.y)) << "lost " << point.x << "," << point.y;
    }
}

TEST_F(Rooted, ARegionInsideTheLastOneAddsNothing) {
    root.damage({10, 10, 50, 50});

    for (int at = 0; at < 200; ++at) {
        root.damage({20, 20, 5, 5});
    }

    const std::vector<BLRect> &taken = root.take();

    ASSERT_EQ(taken.size(), 1U);
    EXPECT_TRUE(tests::same(taken[0], BLRect{10, 10, 50, 50}));
}

TEST_F(Rooted, ARegionReachingOutOfTheLastOneIsKept) {
    root.damage({10, 10, 20, 20});
    root.damage({25, 25, 20, 20});

    const std::vector<BLRect> &taken = root.take();

    EXPECT_TRUE(tests::covers(taken, 44, 44));
    EXPECT_TRUE(tests::covers(taken, 10, 10));
}

TEST_F(Rooted, ManyRegionsAreBoundedByCrowdedAndLoseNoPixel) {
    constexpr int SPREAD = 9;
    std::vector<BLPoint> given;

    for (int y = 0; y < HEIGHT; y += SPREAD) {
        for (int x = (y / SPREAD) % 2; x < WIDTH; x += SPREAD) {
            root.damage({static_cast<double>(x), static_cast<double>(y), 1, 1});
            given.emplace_back(x, y);
        }
    }

    ASSERT_GT(given.size(), ttk::Damage::CROWDED * 4);

    const std::vector<BLRect> &taken = root.take();

    EXPECT_LE(taken.size(), ttk::Damage::CROWDED);

    for (const BLPoint &point : given) {
        ASSERT_TRUE(tests::covers(taken, point.x, point.y)) << "lost " << point.x << "," << point.y;
    }
}

TEST_F(Rooted, DistantRegionsComeBackApart) {
    root.damage({0, 0, 2, 2});
    root.damage({WIDTH - 2, HEIGHT - 2, 2, 2});

    EXPECT_EQ(root.take().size(), 2U);
}

TEST_F(Rooted, DamageAllReplacesPendingRegionsWithTheWindow) {
    root.damage({10, 10, 5, 5});
    root.damage({100, 100, 5, 5});
    root.damage_all();

    EXPECT_TRUE(root.whole());

    const std::vector<BLRect> &taken = root.take();

    ASSERT_EQ(taken.size(), 1U);
    EXPECT_TRUE(tests::same(taken[0], WINDOW));
}

TEST_F(Rooted, DamageAfterDamageAllStaysWhole) {
    root.damage_all();
    root.damage({10, 10, 5, 5});

    EXPECT_TRUE(root.whole());
    EXPECT_EQ(root.take().size(), 1U);
}

TEST_F(Rooted, TakeClearsWhatItHandsOut) {
    root.damage({10, 10, 5, 5});

    EXPECT_EQ(root.take().size(), 1U);
    EXPECT_TRUE(root.take().empty());
    EXPECT_FALSE(root.dirty());
}

TEST_F(Rooted, TakeHandsOutOneListThatTheNextCallOverwrites) {
    root.damage({10, 10, 5, 5});

    const std::vector<BLRect> &first = root.take();

    ASSERT_EQ(first.size(), 1U);
    EXPECT_TRUE(tests::same(first[0], BLRect{10, 10, 5, 5}));

    root.damage({50, 60, 7, 8});

    const std::vector<BLRect> &second = root.take();

    EXPECT_EQ(&first, &second) << "the list is owned by the root";
    ASSERT_EQ(first.size(), 1U);
    EXPECT_TRUE(tests::same(first[0], BLRect{50, 60, 7, 8}));
}

TEST_F(Rooted, WholeAnswersForMergedRegions) {
    root.damage({0, 0, WIDTH / 2.0, HEIGHT});

    EXPECT_FALSE(root.whole());

    root.damage({WIDTH / 2.0, 0, WIDTH / 2.0, HEIGHT});

    EXPECT_TRUE(root.whole()) << "two halves of the window did not merge into the whole";
}

TEST_F(Rooted, WholeKeepsThePendingRegions) {
    root.damage({10, 10, 5, 5});
    root.whole();

    EXPECT_TRUE(root.dirty());
    EXPECT_EQ(root.take().size(), 1U);
}

TEST_F(Rooted, ShiftIsAcceptedOnThePageWithNothingAbove) {
    const tests::Probe *row = page();

    EXPECT_TRUE(root.shift(row, {0, 20, 100, 60}, -5));

    const std::vector<ttk::Root::Shift> &shifts = root.take_shifts();

    ASSERT_EQ(shifts.size(), 1U);
    EXPECT_EQ(shifts[0].region.x, 0);
    EXPECT_EQ(shifts[0].region.y, 20);
    EXPECT_EQ(shifts[0].region.w, 100);
    EXPECT_EQ(shifts[0].region.h, 60);
    EXPECT_EQ(shifts[0].dy, -5);
}

TEST_F(Rooted, ShiftRoundsItsRegionInward) {
    const tests::Probe *row = page();

    ASSERT_TRUE(root.shift(row, {10.5, 20.2, 50, 30.6}, 3));

    const std::vector<ttk::Root::Shift> &shifts = root.take_shifts();

    ASSERT_EQ(shifts.size(), 1U);
    EXPECT_EQ(shifts[0].region.x, 11);
    EXPECT_EQ(shifts[0].region.y, 21);
    EXPECT_EQ(shifts[0].region.w, 49);
    EXPECT_EQ(shifts[0].region.h, 29);
    EXPECT_EQ(shifts[0].dy, 3);
}

TEST_F(Rooted, AcceptedShiftMakesTheRootDirty) {
    const tests::Probe *row = page();

    ASSERT_TRUE(root.shift(row, {0, 0, 50, 50}, 2));

    EXPECT_TRUE(root.dirty()) << "a pending pixel move did not count";

    root.take_shifts();

    EXPECT_FALSE(root.dirty());
}

TEST_F(Rooted, ShiftsAreKeptInOrderUntilTaken) {
    const tests::Probe *row = page();

    ASSERT_TRUE(root.shift(row, {0, 0, 50, 50}, 2));
    ASSERT_TRUE(root.shift(row, {0, 60, 50, 50}, -4));

    const std::vector<ttk::Root::Shift> &shifts = root.take_shifts();

    ASSERT_EQ(shifts.size(), 2U);
    EXPECT_EQ(shifts[0].dy, 2);
    EXPECT_EQ(shifts[1].dy, -4);
    EXPECT_EQ(shifts[1].region.y, 60);
    EXPECT_TRUE(root.take_shifts().empty());
}

TEST_F(Rooted, TakeShiftsHandsOutAListTheNextCallOverwrites) {
    const tests::Probe *row = page();

    ASSERT_TRUE(root.shift(row, {0, 0, 50, 50}, 2));

    const std::vector<ttk::Root::Shift> &first = root.take_shifts();

    ASSERT_EQ(first.size(), 1U);

    ASSERT_TRUE(root.shift(row, {0, 10, 20, 20}, 7));

    const std::vector<ttk::Root::Shift> &second = root.take_shifts();

    EXPECT_EQ(&first, &second) << "the list is owned by the root";
    ASSERT_EQ(second.size(), 1U);
    EXPECT_EQ(second[0].dy, 7);
}

TEST_F(Rooted, ShiftIsRefusedUnderAClippingAncestor) {
    tests::Probe *viewport = page("viewport");
    const auto *row = viewport->append(std::make_unique<tests::Probe>("row"));

    viewport->clipping = true;
    viewport->clipRegion = WINDOW;

    EXPECT_FALSE(root.shift(row, {0, 0, 50, 50}, 2));
    EXPECT_TRUE(root.take_shifts().empty());
    EXPECT_FALSE(root.dirty());
}

TEST_F(Rooted, ShiftIsRefusedUnderAnyClippingAncestorUpTheTree) {
    tests::Probe *outer = page("outer");
    auto *middle = outer->append(std::make_unique<tests::Probe>("middle"));
    const auto *row = middle->append(std::make_unique<tests::Probe>("row"));

    outer->clipping = true;
    outer->clipRegion = WINDOW;

    EXPECT_FALSE(root.shift(row, {0, 0, 50, 50}, 2));
}

TEST_F(Rooted, ShiftIgnoresAClipOnTheWidgetItself) {
    tests::Probe *viewport = page("viewport");

    viewport->clipping = true;
    viewport->clipRegion = WINDOW;

    EXPECT_TRUE(root.shift(viewport, {0, 0, 50, 50}, 2));
}

TEST_F(Rooted, ShiftIsRefusedUnderAPopupOverItsRegion) {
    const tests::Probe *row = page();

    popup({40, 40, 30, 30});

    EXPECT_FALSE(root.shift(row, {0, 0, 50, 50}, 2));
    EXPECT_TRUE(root.take_shifts().empty());
}

TEST_F(Rooted, ShiftIsAcceptedBesideAPopup) {
    const tests::Probe *row = page();

    popup({50, 50, 30, 30});

    EXPECT_TRUE(root.shift(row, {0, 0, 50, 50}, 2)) << "a popup only touching the edge refused the move";
}

TEST_F(Rooted, ShiftIsAcceptedUnderAHiddenPopup) {
    const tests::Probe *row = page();

    popup({10, 10, 30, 30})->set_visible(false);

    EXPECT_TRUE(root.shift(row, {0, 0, 50, 50}, 2));
}

TEST_F(Rooted, ShiftTestsWhereAPopupDrawsRatherThanItsBox) {
    const tests::Probe *row = page();
    tests::Probe *shadowed = popup({60, 60, 20, 20});

    shadowed->spread = true;
    shadowed->spreadBox = {40, 40, 40, 40};

    EXPECT_FALSE(root.shift(row, {0, 0, 50, 50}, 2)) << "a shadow over the region was ignored";
}

TEST_F(Rooted, ShiftIsRefusedUnderEveryLayerAboveThePage) {
    for (std::size_t index = 0; index < ttk::Root::LAYERS; ++index) {
        const tests::Probe *row = page();
        const tests::Probe *over = tests::mount(root.layer(index), {10, 10, 10, 10}, tests::fonts());

        EXPECT_FALSE(root.shift(row, {0, 0, 50, 50}, 2)) << "layer " << index;

        root.layer(index)->erase(over);
        root.content()->clear();
    }
}

TEST_F(Rooted, ShiftOnALayerIgnoresTheLayersBelowIt) {
    tests::mount(root.layer(ttk::Root::DIALOGS), {0, 0, 100, 100}, tests::fonts(), "dialog");
    const tests::Probe *menu = popup({0, 0, 100, 100}, "menu");
    tests::Probe *sibling = popup({10, 10, 20, 20}, "sibling");

    EXPECT_TRUE(root.shift(menu, {0, 0, 50, 50}, 2)) << "the dialog below or a sibling refused the move";
    EXPECT_NE(sibling, nullptr);
}

TEST_F(Rooted, ShiftOnALayerIsRefusedUnderALayerAbove) {
    const tests::Probe *menu = popup({0, 0, 100, 100});

    tests::mount(root.layer(ttk::Root::TIPS), {20, 20, 10, 10}, tests::fonts(), "tip");

    EXPECT_FALSE(root.shift(menu, {0, 0, 50, 50}, 2));
}

TEST_F(Rooted, ShiftMovesPendingDamageWithThePixels) {
    const tests::Probe *row = page();

    root.damage({10, 50, 10, 10});

    ASSERT_TRUE(root.shift(row, {0, 20, 100, 60}, -20));

    const std::vector<BLRect> &taken = root.take();

    EXPECT_TRUE(tests::covers(taken, 10, 30)) << "pending damage did not move with the pixels";
    EXPECT_TRUE(tests::covers(taken, 19, 39));
    EXPECT_TRUE(tests::covers(taken, 10, 50)) << "the damage where it was was dropped";
}

TEST_F(Rooted, RefusedShiftLeavesPendingDamageWhereItWas) {
    const tests::Probe *row = page();

    popup({0, 0, 10, 10});
    root.damage({10, 50, 10, 10});

    ASSERT_FALSE(root.shift(row, {0, 0, 100, 100}, -20));

    const std::vector<BLRect> &taken = root.take();

    EXPECT_FALSE(tests::covers(taken, 10, 30));
    EXPECT_TRUE(tests::covers(taken, 10, 50));
}

TEST_F(Rooted, PaintDrawsThePageThenEveryLayerInOrder) {
    tests::Probe *back = page("back");

    back->tone = BLRgba32(0xff102030);

    const BLRgba32 tones[] = {
            BLRgba32(0xffff0000),
            BLRgba32(0xff00ff00),
            BLRgba32(0xff0000ff),
            BLRgba32(0xffffff00),
    };

    for (std::size_t index = 0; index < ttk::Root::LAYERS; ++index) {
        const double at = 20.0 + (static_cast<double>(index) * 20.0);

        tests::mount(root.layer(index), {at, at, 60, 60}, tests::fonts())->tone = tones[index];
    }

    const BLImage image = painted(root, {0, 0, WIDTH, HEIGHT});

    EXPECT_EQ(pixel(image, 5, 5), 0xff102030U);
    EXPECT_EQ(pixel(image, 25, 25), tones[0].value);
    EXPECT_EQ(pixel(image, 45, 45), tones[1].value);
    EXPECT_EQ(pixel(image, 65, 65), tones[2].value);
    EXPECT_EQ(pixel(image, 85, 85), tones[3].value) << "a lower layer was painted over a higher one";
}

TEST_F(Rooted, PaintSkipsWidgetsOutsideTheClip) {
    tests::Probe *box = popup({10, 10, 20, 20});

    box->tone = BLRgba32(0xffff0000);

    const BLImage image = painted(root, {100, 60, 20, 20});

    EXPECT_EQ(box->painted, 0);
    EXPECT_EQ(pixel(image, 15, 15), 0U);
}

TEST_F(Rooted, PaintDrawsWidgetsTouchingTheClip) {
    tests::Probe *box = popup({10, 10, 20, 20});

    box->tone = BLRgba32(0xffff0000);
    painted(root, {29, 29, 20, 20});

    EXPECT_EQ(box->painted, 1);
}

TEST_F(Rooted, PaintSkipsHiddenWidgets) {
    tests::Probe *box = popup({10, 10, 20, 20});

    box->tone = BLRgba32(0xffff0000);
    box->set_visible(false);

    const BLImage image = painted(root, {0, 0, WIDTH, HEIGHT});

    EXPECT_EQ(box->painted, 0);
    EXPECT_EQ(pixel(image, 15, 15), 0U);
}

TEST_F(Rooted, PaintDrawsAFocusRingAfterTab) {
    tests::Probe *field = popup({20, 20, 80, 40});
    const BLRect box = field->box();

    ASSERT_TRUE(root.key({.code = ttk::Code::Tab}));
    ASSERT_EQ(root.focused(), field);

    const BLImage image = painted(root, {0, 0, WIDTH, HEIGHT});

    BLImage expected(WIDTH, HEIGHT, BL_FORMAT_PRGB32);
    BLContext context(expected);

    context.clear_all();

    {
        const ttk::Painter painter(context, tests::fonts(), {0, 0, WIDTH, HEIGHT});

        painter.outline(box, ttk::Theme::radiusSmall, 1.0, ttk::Theme::palette().accent);
    }

    context.end();

    EXPECT_EQ(pixels(image), pixels(expected));
    EXPECT_NE(pixel(image, 60, 20), 0U) << "no ring along the top edge";
    EXPECT_EQ(pixel(image, 60, 40), 0U) << "the ring was filled";
}

TEST_F(Rooted, FocusGivenDirectlyShowsNoRing) {
    tests::Probe *field = popup({20, 20, 80, 40});

    root.focus(field);

    EXPECT_EQ(pixel(painted(root, {0, 0, WIDTH, HEIGHT}), 60, 20), 0U);
}

TEST_F(Rooted, APressHidesTheFocusRing) {
    tests::Probe *field = popup({20, 20, 80, 40});

    root.key({.code = ttk::Code::Tab});
    ASSERT_NE(pixel(painted(root, {0, 0, WIDTH, HEIGHT}), 60, 20), 0U);

    root.motion(50, 30);
    root.take();
    root.press({.x = 50, .y = 30});

    EXPECT_EQ(root.focused(), field) << "the press itself should not have moved focus";
    EXPECT_EQ(pixel(painted(root, {0, 0, WIDTH, HEIGHT}), 60, 20), 0U);
    EXPECT_TRUE(tests::covers(root.take(), 60, 20)) << "hiding the ring did not repaint it";
}

TEST_F(Rooted, AWidgetThatPaintsItsOwnFocusGetsNoRing) {
    tests::Probe *field = popup({20, 20, 80, 40});

    field->focusPainted = true;
    root.key({.code = ttk::Code::Tab});
    ASSERT_EQ(root.focused(), field);

    EXPECT_EQ(pixel(painted(root, {0, 0, WIDTH, HEIGHT}), 60, 20), 0U);
}

TEST_F(Rooted, AHiddenFocusedWidgetGetsNoRing) {
    tests::Probe *field = popup({20, 20, 80, 40});

    root.key({.code = ttk::Code::Tab});
    ASSERT_EQ(root.focused(), field);
    field->set_visible(false);

    EXPECT_EQ(pixel(painted(root, {0, 0, WIDTH, HEIGHT}), 60, 20), 0U);
}

TEST_F(Rooted, NoWidgetIsLiveAtFirst) {
    EXPECT_FALSE(root.busy());
    EXPECT_EQ(root.waking(), -1.0);
}

TEST_F(Rooted, WakeMakesAWidgetLive) {
    const tests::Probe *clock = page();

    clock->wake();

    EXPECT_TRUE(root.busy());
}

TEST_F(Rooted, AdvanceStepsLiveWidgetsAndDropsThoseThatStop) {
    tests::Probe *clock = page();

    clock->frames = 2;
    clock->wake();

    root.advance(1.0);

    EXPECT_EQ(clock->advancedTo, 1.0);
    EXPECT_TRUE(root.busy());

    root.advance(2.0);

    EXPECT_EQ(clock->advancedTo, 2.0);
    EXPECT_TRUE(root.busy());

    root.advance(3.0);

    EXPECT_EQ(clock->advancedTo, 3.0);
    EXPECT_FALSE(root.busy());

    clock->advancedTo = -1.0;
    root.advance(4.0);

    EXPECT_EQ(clock->advancedTo, -1.0) << "a widget that stopped was still stepped";
}

TEST_F(Rooted, LiveTwiceStepsOncePerFrame) {
    std::vector<std::string> journal;
    auto *clock = root.content()->append(std::make_unique<tests::Probe>("clock", &journal));

    clean();
    root.live(clock);
    root.live(clock);
    root.advance(1.0);

    EXPECT_EQ(journal, std::vector<std::string>{"clock:advance"});
}

TEST_F(Rooted, WakeAtKeepsTheEarlierTime) {
    tests::Probe *clock = page();

    root.wake_at(clock, 5.0);

    EXPECT_EQ(root.waking(), 5.0);

    root.wake_at(clock, 3.0);

    EXPECT_EQ(root.waking(), 3.0);

    root.wake_at(clock, 7.0);

    EXPECT_EQ(root.waking(), 3.0);
    EXPECT_FALSE(root.busy()) << "a scheduled widget is not live yet";
}

TEST_F(Rooted, WakingIsTheEarliestOfEveryScheduledWidget) {
    tests::Probe *one = page("one");
    tests::Probe *two = page("two");

    root.wake_at(one, 4.0);
    root.wake_at(two, 2.5);

    EXPECT_EQ(root.waking(), 2.5);
}

TEST_F(Rooted, ScheduledWidgetBecomesLiveWhenDue) {
    tests::Probe *clock = page();

    clock->frames = 1;
    root.wake_at(clock, 2.0);
    root.advance(1.5);

    EXPECT_EQ(clock->advancedTo, -1.0) << "stepped before it was due";
    EXPECT_FALSE(root.busy());

    root.advance(2.0);

    EXPECT_EQ(clock->advancedTo, 2.0);
    EXPECT_TRUE(root.busy());
    EXPECT_EQ(root.waking(), -1.0);
}

TEST_F(Rooted, LiveCancelsAScheduledWakeUp) {
    tests::Probe *clock = page();

    root.wake_at(clock, 5.0);
    root.live(clock);

    EXPECT_EQ(root.waking(), -1.0);
    EXPECT_TRUE(root.busy());
}

TEST_F(Rooted, WakeAtLeavesALiveWidgetLive) {
    tests::Probe *clock = page();

    clock->frames = 5;
    clock->wake();
    root.wake_at(clock, 9.0);

    EXPECT_TRUE(root.busy());
    EXPECT_EQ(root.waking(), 9.0);
}

TEST_F(Rooted, SleepUntilFromAdvanceSchedulesTheNextWakeUp) {
    tests::Probe *caret = page();

    caret->sleepTo = 4.0;
    caret->wake();
    root.advance(1.0);

    EXPECT_FALSE(root.busy());
    EXPECT_EQ(root.waking(), 4.0);

    root.advance(4.0);

    EXPECT_EQ(caret->advancedTo, 4.0);
}

TEST_F(Rooted, SetNowSetsTheClockWidgetsRead) {
    const tests::Probe *clock = page();

    root.set_now(12.5);

    EXPECT_EQ(root.now(), 12.5);
    EXPECT_EQ(clock->now(), 12.5);
}

TEST_F(Rooted, AdvanceLeavesTheClockAlone) {
    root.set_now(1.0);
    root.advance(3.0);

    EXPECT_EQ(root.now(), 1.0);
}

TEST_F(Rooted, ForgetDropsLiveAndScheduledWidgets) {
    tests::Probe *live = page("live");
    tests::Probe *sleeping = page("sleeping");

    live->frames = 10;
    live->wake();
    root.wake_at(sleeping, 3.0);
    root.forget(live);
    root.forget(sleeping);

    EXPECT_FALSE(root.busy());
    EXPECT_EQ(root.waking(), -1.0);
}

TEST_F(Rooted, ForgetReachesDescendants) {
    tests::Probe *parent = page("parent");
    auto *child = parent->append(std::make_unique<tests::Probe>("child"));

    child->frames = 10;
    child->wake();
    root.focus(child);
    root.forget(parent);

    EXPECT_FALSE(root.busy());
    EXPECT_EQ(root.focused(), nullptr);
}

TEST_F(Rooted, ForgetOfNullDoesNothing) {
    tests::Probe *clock = page();

    clock->frames = 1;
    clock->wake();
    root.forget(nullptr);

    EXPECT_TRUE(root.busy());
}

TEST_F(Rooted, ActiveUntilToldOtherwise) {
    EXPECT_TRUE(root.active());

    root.set_active(false);

    EXPECT_FALSE(root.active());

    root.set_active(true);

    EXPECT_TRUE(root.active());
}

TEST_F(Rooted, ActivityChangeWakesTheFocusedWidget) {
    tests::Probe *field = page();

    root.focus(field);
    root.set_active(false);

    EXPECT_TRUE(root.busy());

    root.advance(1.0);

    EXPECT_EQ(field->advancedTo, 1.0);
}

TEST_F(Rooted, UnchangedActivityWakesNothing) {
    tests::Probe *field = page();

    root.focus(field);
    root.set_active(true);

    EXPECT_FALSE(root.busy());
}

TEST_F(Rooted, ActivityChangeWithNothingFocusedWakesNothing) {
    page();
    root.set_active(false);

    EXPECT_FALSE(root.busy());
}
}
