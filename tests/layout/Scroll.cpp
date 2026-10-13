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
#include <cstdint>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "Blocks.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/layout/Box.h"
#include "ttk/toolkit/layout/Scroll.h"

namespace {
    using tests::Block;
    using tests::boxed;

    constexpr double ROW = 20.0;
    constexpr std::uint32_t PLAIN = 0xff204060U;
    constexpr double FRAME = 1.0 / 60.0;
    constexpr int FRAMES = 400;

    ttk::Typeface &type() {
        return tests::fonts();
    }

    std::uint32_t stripe(const int row) {
        return 0xff000000U | ((static_cast<std::uint32_t>(row + 1) * 0x0a0b0cU) & 0xffffffU);
    }

    // Rows ROW pixels tall, each its own colour, so a pixel tells which row of the content is under it.
    class Stripes final : public ttk::Widget {
    public:
        explicit Stripes(const double height) : tall(height) {}

        double tall;
        int marked = -1;

        double natural_height(ttk::Typeface & /*type*/, double /*width*/) override { return tall; }

        void paint(const ttk::Painter &painter) override {
            const int rows = static_cast<int>(std::ceil(tall / ROW));

            for (int row = 0; row < rows; ++row) {
                const BLRect band{_box.x, _box.y + (row * ROW), _box.w, ROW};

                if (painter.needed(band)) {
                    painter.fill(band, BLRgba32(row == marked ? 0xffffffffU : stripe(row)));
                }
            }
        }
    };

    // Takes its children's leftovers: a wheel nobody below consumed, and a clip of its own when asked.
    class Holder final : public ttk::Widget {
    public:
        int wheeled = 0;
        bool clipping = false;

        bool wheel(double /*steps*/, const ttk::Pointer & /*at*/) override {
            ++wheeled;

            return true;
        }

        bool clips(BLRect &region) const override {
            region = _box;

            return clipping;
        }
    };

    struct Mounted {
        ttk::Scroll *scroll;
        Stripes *content;
    };

    Mounted page(tests::Canvas &canvas, const double tall) {
        auto *scroll = canvas.root().content()->append(std::make_unique<ttk::Scroll>());
        auto *content = static_cast<Stripes *>(scroll->hold(std::make_unique<Stripes>(tall)));

        canvas.frame();

        return {.scroll = scroll, .content = content};
    }

    // A scroller at `box`, on the popup layer, which keeps the box it is placed at.
    Mounted floating(tests::Canvas &canvas, const BLRect &box, const double tall) {
        auto *scroll = canvas.root().layer(ttk::Root::POPUPS)->append(std::make_unique<ttk::Scroll>());
        auto *content = static_cast<Stripes *>(scroll->hold(std::make_unique<Stripes>(tall)));

        canvas.frame();
        scroll->place(box, type());
        canvas.root().damage_all();
        canvas.frame();

        return {.scroll = scroll, .content = content};
    }

    std::unique_ptr<ttk::Scroll> loose(const BLRect &box, const double tall) {
        auto scroll = std::make_unique<ttk::Scroll>();

        scroll->hold(std::make_unique<Stripes>(tall));
        scroll->place(box, type());

        return scroll;
    }

    // Runs frames until nothing is live, and answers the time reached.
    double glide(tests::Canvas &canvas, double now) {
        for (int frame = 0; frame < FRAMES && canvas.root().busy(); ++frame) {
            now += FRAME;
            canvas.frame_at(now);
        }

        return now;
    }

    int row_at(const tests::Canvas &canvas, const int x, const int y) {
        const std::uint32_t pixel = canvas.at(x, y);

        for (int row = 0; row < 1000; ++row) {
            if (stripe(row) == pixel) {
                return row;
            }
        }

        return -1;
    }

    TEST(Scroll, StartsEmpty) {
        const ttk::Scroll scroll;

        EXPECT_EQ(scroll.content(), nullptr);
        EXPECT_EQ(scroll.offset(), 0.0);
        EXPECT_EQ(scroll.reach(), 0.0);
        EXPECT_FALSE(scroll.scrollable());
        EXPECT_TRUE(scroll.at_end());
        EXPECT_FALSE(scroll.takes_focus());
    }

    TEST(Scroll, HoldReturnsTheContent) {
        ttk::Scroll scroll;
        auto stripes = std::make_unique<Stripes>(100.0);
        Stripes *raw = stripes.get();

        EXPECT_EQ(scroll.hold(std::move(stripes)), raw);
        EXPECT_EQ(scroll.content(), raw);
        EXPECT_EQ(raw->parent(), &scroll);
    }

    TEST(Scroll, HoldDestroysEveryOtherChild) {
        ttk::Scroll scroll;

        scroll.add(std::make_unique<Block>(10.0, 10.0));
        scroll.hold(std::make_unique<Stripes>(100.0));

        ttk::Widget *second = scroll.hold(std::make_unique<Stripes>(200.0));

        ASSERT_EQ(scroll.children().size(), 1U);
        EXPECT_EQ(scroll.children().front().get(), second);
    }

    TEST(Scroll, ArrangeWithoutContentKeepsTheReach) {
        ttk::Scroll scroll;

        scroll.set_reach(500.0);
        scroll.place({0, 0, 100, 100}, type());

        EXPECT_EQ(scroll.reach(), 500.0);
        EXPECT_TRUE(scroll.scrollable());
    }

    TEST(Scroll, ContentTakesTheViewWidthAndItsOwnHeight) {
        const auto scroll = loose({10, 20, 300, 100}, 1000.0);

        EXPECT_TRUE(boxed(scroll->content()->box(), {10, 20, 300, 1000}));
        EXPECT_EQ(scroll->reach(), 1000.0);
        EXPECT_TRUE(scroll->scrollable());
        EXPECT_FALSE(scroll->at_end());
    }

    TEST(Scroll, ContentWiderFloorIsKept) {
        ttk::Scroll scroll;
        Block *content = static_cast<Block *>(scroll.hold(std::make_unique<Block>(10.0)));

        content->area = 50000.0;
        content->minWidth = 500.0;
        scroll.place({0, 0, 200, 50}, type());

        EXPECT_TRUE(boxed(content->box(), {0, 0, 500, 100})) << "measured or laid out at the view width";
        EXPECT_EQ(scroll.reach(), 100.0);
    }

    TEST(Scroll, ShortContentIsStretchedToTheView) {
        const auto scroll = loose({0, 0, 200, 100}, 50.0);

        EXPECT_TRUE(boxed(scroll->content()->box(), {0, 0, 200, 100}));
        EXPECT_EQ(scroll->reach(), 50.0);
        EXPECT_FALSE(scroll->scrollable());
        EXPECT_TRUE(scroll->at_end());

        scroll->scroll_to(30.0);

        EXPECT_EQ(scroll->offset(), 0.0);
    }

    TEST(Scroll, ScrollableOnlyPastAPixelOfOverflow) {
        EXPECT_FALSE(loose({0, 0, 200, 100}, 100.0)->scrollable());
        EXPECT_FALSE(loose({0, 0, 200, 100}, 101.0)->scrollable());
        EXPECT_TRUE(loose({0, 0, 200, 100}, 101.5)->scrollable());
    }

    TEST(Scroll, ScrollToClampsAtBothEnds) {
        tests::Canvas canvas;
        const Mounted mounted = floating(canvas, {10, 20, 300, 100}, 1000.0);

        mounted.scroll->scroll_to(-50.0);

        EXPECT_EQ(mounted.scroll->offset(), 0.0);

        mounted.scroll->scroll_to(5000.0);

        EXPECT_EQ(mounted.scroll->offset(), 900.0);
        EXPECT_TRUE(mounted.scroll->at_end());
        EXPECT_TRUE(boxed(mounted.content->box(), {10, -880, 300, 1000}));

        mounted.scroll->scroll_to(-1.0);

        EXPECT_EQ(mounted.scroll->offset(), 0.0);
        EXPECT_TRUE(boxed(mounted.content->box(), {10, 20, 300, 1000}));
    }

    TEST(Scroll, ScrollToRoundsToWholePixels) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        mounted.scroll->scroll_to(10.4);

        EXPECT_EQ(mounted.scroll->offset(), 10.0);

        mounted.scroll->scroll_to(10.6);

        EXPECT_EQ(mounted.scroll->offset(), 11.0);
        EXPECT_EQ(mounted.content->box().y, -11.0);
    }

    TEST(Scroll, EndIsAWholePixelForAFractionalView) {
        const auto scroll = loose({0, 0, 200, 100.5}, 1000.0);

        scroll->scroll_to(1000.0);

        EXPECT_EQ(scroll->offset(), 899.0);
        EXPECT_TRUE(scroll->at_end());
    }

    TEST(Scroll, ScrollToWorksWithoutARoot) {
        const auto scroll = loose({0, 0, 200, 100}, 1000.0);

        scroll->scroll_to(250.0);

        EXPECT_EQ(scroll->offset(), 250.0);
    }

    TEST(Scroll, AtEndIsWithinAPixel) {
        const auto scroll = loose({0, 0, 200, 100}, 1000.0);

        scroll->scroll_to(899.0);

        EXPECT_TRUE(scroll->at_end());

        scroll->scroll_to(898.0);

        EXPECT_FALSE(scroll->at_end());
    }

    TEST(Scroll, SetReachClampsTheOffset) {
        ttk::Scroll scroll;

        scroll.place({0, 0, 100, 100}, type());
        scroll.set_reach(1000.0);
        scroll.scroll_to(800.0);

        EXPECT_EQ(scroll.offset(), 800.0);

        scroll.set_reach(500.0);

        EXPECT_EQ(scroll.reach(), 500.0);
        EXPECT_EQ(scroll.offset(), 400.0);

        scroll.set_reach(50.0);

        EXPECT_EQ(scroll.offset(), 0.0);
        EXPECT_FALSE(scroll.scrollable());
    }

    TEST(Scroll, ArrangeReplacesAReachSetByHand) {
        const auto scroll = loose({0, 0, 200, 100}, 1000.0);

        scroll->set_reach(5000.0);
        scroll->place({0, 0, 200, 100}, type());

        EXPECT_EQ(scroll->reach(), 1000.0);
    }

    TEST(Scroll, RefitMeasuresContentThatGrew) {
        const auto scroll = loose({0, 0, 200, 100}, 1000.0);
        auto *content = static_cast<Stripes *>(scroll->content());

        scroll->scroll_to(100.0);
        content->tall = 1500.0;
        scroll->refit(type());

        EXPECT_EQ(scroll->reach(), 1500.0);
        EXPECT_EQ(scroll->offset(), 100.0);
        EXPECT_EQ(content->box().h, 1500.0);
    }

    TEST(Scroll, RefitKeepsAViewAtTheEndAtTheEnd) {
        const auto scroll = loose({0, 0, 200, 100}, 1000.0);
        auto *content = static_cast<Stripes *>(scroll->content());

        scroll->scroll_to(900.0);
        content->tall = 1500.0;
        scroll->refit(type(), 40.0);

        EXPECT_EQ(scroll->offset(), 1400.0);
        EXPECT_TRUE(scroll->at_end());
    }

    TEST(Scroll, RefitLowersTheOffsetByWhatWasLostAbove) {
        const auto scroll = loose({0, 0, 200, 100}, 1000.0);
        auto *content = static_cast<Stripes *>(scroll->content());

        scroll->scroll_to(300.0);
        content->tall = 900.0;
        scroll->refit(type(), 100.0);

        EXPECT_EQ(scroll->offset(), 200.0);
        EXPECT_EQ(content->box().y, -200.0);
    }

    TEST(Scroll, RefitNeverGoesAboveTheTop) {
        const auto scroll = loose({0, 0, 200, 100}, 1000.0);

        scroll->scroll_to(300.0);
        scroll->refit(type(), 500.0);

        EXPECT_EQ(scroll->offset(), 0.0);
    }

    TEST(Scroll, RefitWithoutContentDoesNothing) {
        ttk::Scroll scroll;

        scroll.place({0, 0, 100, 100}, type());
        scroll.set_reach(1000.0);
        scroll.scroll_to(500.0);
        scroll.refit(type(), 100.0);

        EXPECT_EQ(scroll.reach(), 1000.0);
        EXPECT_EQ(scroll.offset(), 500.0);
    }

    TEST(Scroll, RefitRepaintsTheView) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        mounted.content->tall = 1400.0;
        mounted.scroll->refit(type());

        EXPECT_TRUE(canvas.root().dirty());
        EXPECT_GT(canvas.frame(), 0U);
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST(Scroll, RevealInViewDoesNothing) {
        const auto scroll = loose({0, 0, 200, 100}, 1000.0);

        scroll->scroll_to(50.0);
        scroll->reveal({0, 10, 20, 80});

        EXPECT_EQ(scroll->offset(), 50.0);
    }

    TEST(Scroll, RevealBelowScrollsTheLeastDistance) {
        const auto scroll = loose({0, 0, 200, 100}, 1000.0);

        scroll->reveal({0, 150, 20, 20});

        EXPECT_EQ(scroll->offset(), 70.0);
    }

    TEST(Scroll, RevealAboveScrollsTheLeastDistance) {
        const auto scroll = loose({0, 0, 200, 100}, 1000.0);

        scroll->scroll_to(300.0);
        scroll->reveal({0, -50, 20, 20});

        EXPECT_EQ(scroll->offset(), 250.0);
    }

    TEST(Scroll, RevealOfATallBoxPutsTheEdgeItCrossedOnTheView) {
        const auto scroll = loose({0, 0, 200, 100}, 1000.0);

        scroll->reveal({0, 50, 20, 300});

        EXPECT_EQ(scroll->offset(), 250.0) << "the bottom edge was not brought to the bottom of the view";

        scroll->reveal({0, -100, 20, 300});

        EXPECT_EQ(scroll->offset(), 150.0) << "the top edge was not brought to the top of the view";
    }

    TEST(Scroll, RevealIsClampedToTheContent) {
        const auto scroll = loose({0, 0, 200, 100}, 1000.0);

        scroll->reveal({0, 5000, 20, 20});

        EXPECT_EQ(scroll->offset(), 900.0);
    }

    TEST(Scroll, TabFocusRevealsTheFocusedChild) {
        tests::Canvas canvas;
        auto *scroll = canvas.root().content()->append(std::make_unique<ttk::Scroll>());
        auto *column = static_cast<ttk::Box *>(scroll->hold(ttk::Box::column()));
        std::vector<Block *> blocks;

        for (int at = 0; at < 10; ++at) {
            blocks.push_back(column->append(std::make_unique<Block>(100.0, 100.0)));
            blocks.back()->answers();
        }

        canvas.frame();

        for (int press = 0; press < 3; ++press) {
            canvas.root().key(ttk::Key{.code = ttk::Code::Tab});
        }

        ASSERT_EQ(canvas.root().focused(), blocks[2]);
        EXPECT_EQ(scroll->offset(), 60.0);
        EXPECT_TRUE(boxed(blocks[2]->box(), {0, 140, 400, 100}));

        canvas.frame();

        EXPECT_TRUE(canvas.matches_full());
    }

    TEST(Scroll, ClipsToTheView) {
        const auto scroll = loose({10, 20, 300, 100}, 1000.0);
        BLRect region{};

        ASSERT_TRUE(scroll->clips(region));
        EXPECT_TRUE(boxed(region, {10, 20, 300, 100}));
    }

    TEST(Scroll, PaintsTheContentOnlyInsideTheView) {
        tests::Canvas canvas;

        floating(canvas, {50, 40, 200, 100}, 1000.0);

        const std::uint32_t background = ttk::Theme::palette().background.value;

        EXPECT_EQ(row_at(canvas, 100, 40), 0);
        EXPECT_EQ(row_at(canvas, 100, 139), 4);
        EXPECT_EQ(canvas.at(100, 39), background);
        EXPECT_EQ(canvas.at(100, 140), background) << "content below the view was painted";
        EXPECT_EQ(canvas.at(100, 200), background);
        EXPECT_EQ(canvas.at(49, 60), background);
        EXPECT_EQ(canvas.at(250, 60), background);
    }

    TEST(Scroll, ScrolledContentShowsTheRowsUnderTheOffset) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        mounted.scroll->scroll_to(130.0);
        canvas.frame();

        EXPECT_EQ(row_at(canvas, 100, 0), 6);
        EXPECT_EQ(row_at(canvas, 100, 9), 6);
        EXPECT_EQ(row_at(canvas, 100, 10), 7);
        EXPECT_EQ(row_at(canvas, 100, 239), 18);
    }

    TEST(Scroll, ContentInvalidateIsClippedToTheView) {
        tests::Canvas canvas;
        const Mounted mounted = floating(canvas, {50, 40, 200, 100}, 1000.0);

        mounted.content->invalidate({0, 0, 400, 400});

        const std::vector<BLRect> &taken = canvas.root().take();

        ASSERT_FALSE(taken.empty());

        for (const BLRect &region : taken) {
            EXPECT_GE(region.x, 50.0);
            EXPECT_GE(region.y, 40.0);
            EXPECT_LE(region.x + region.w, 250.0);
            EXPECT_LE(region.y + region.h, 140.0);
        }
    }

    TEST(Scroll, InvalidatingContentOutOfViewDamagesNothing) {
        tests::Canvas canvas;
        const Mounted mounted = floating(canvas, {50, 40, 200, 100}, 1000.0);

        mounted.content->invalidate({50, 300, 200, 40});

        EXPECT_FALSE(canvas.root().dirty()) << "a row scrolled out of view asked for a repaint";
        EXPECT_EQ(canvas.frame(), 0U);
    }

    TEST(Scroll, ThumbIsShownOnlyWhenScrollable) {
        tests::Canvas canvas;
        auto *scroll = canvas.root().content()->append(std::make_unique<ttk::Scroll>());
        Block *content = static_cast<Block *>(scroll->hold(std::make_unique<Block>(10.0, 1200.0, PLAIN)));

        canvas.frame();

        EXPECT_NE(canvas.at(395, 24), PLAIN) << "no thumb at the top of the lane";
        EXPECT_EQ(canvas.at(395, 216), PLAIN);
        EXPECT_EQ(canvas.at(388, 24), PLAIN) << "the thumb left its lane";

        content->tall = 100.0;
        canvas.root().relayout();
        canvas.frame();

        EXPECT_EQ(canvas.at(395, 24), PLAIN) << "a thumb was drawn over content that fits";
    }

    TEST(Scroll, ThumbTravelsWithTheOffset) {
        tests::Canvas canvas;
        auto *scroll = canvas.root().content()->append(std::make_unique<ttk::Scroll>());

        scroll->hold(std::make_unique<Block>(10.0, 1200.0, PLAIN));
        canvas.frame();
        scroll->scroll_to(480.0);
        canvas.frame();

        EXPECT_EQ(canvas.at(395, 24), PLAIN);
        EXPECT_NE(canvas.at(395, 120), PLAIN) << "the thumb is not halfway down at half the offset";
        EXPECT_EQ(canvas.at(395, 216), PLAIN);

        scroll->scroll_to(960.0);
        canvas.frame();

        EXPECT_EQ(canvas.at(395, 120), PLAIN);
        EXPECT_NE(canvas.at(395, 216), PLAIN) << "the thumb is not at the bottom at the end";
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST(Scroll, ThumbIsSizedByTheShareInView) {
        tests::Canvas canvas;
        auto *scroll = canvas.root().content()->append(std::make_unique<ttk::Scroll>());

        scroll->hold(std::make_unique<Block>(10.0, 480.0, PLAIN));
        canvas.frame();

        EXPECT_NE(canvas.at(395, 100), PLAIN) << "the thumb is shorter than half the lane";
        EXPECT_EQ(canvas.at(395, 140), PLAIN) << "the thumb is longer than half the lane";
    }

    TEST(Scroll, ThumbIsNeverShorterThanItsFloor) {
        tests::Canvas canvas;
        auto *scroll = canvas.root().content()->append(std::make_unique<ttk::Scroll>());

        scroll->hold(std::make_unique<Block>(10.0, 24000.0, PLAIN));
        canvas.frame();

        EXPECT_NE(canvas.at(395, 20), PLAIN) << "the thumb shrank to its share of the lane";
        EXPECT_EQ(canvas.at(395, 34), PLAIN);
    }

    TEST(Scroll, ThumbIsStrongerWhileDragged) {
        tests::Canvas canvas;
        auto *scroll = canvas.root().content()->append(std::make_unique<ttk::Scroll>());

        scroll->hold(std::make_unique<Block>(10.0, 1200.0, PLAIN));
        canvas.frame();

        const std::uint32_t resting = canvas.at(395, 24);

        canvas.root().press(ttk::Pointer{.x = 395, .y = 24});
        canvas.frame();

        const std::uint32_t held = canvas.at(395, 24);

        EXPECT_NE(held, resting);
        EXPECT_TRUE(canvas.matches_full());

        canvas.root().release(ttk::Pointer{.x = 395, .y = 24});
        canvas.frame();

        EXPECT_EQ(canvas.at(395, 24), resting);
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST(Scroll, LaneIsHitAheadOfTheContent) {
        tests::Canvas canvas;
        auto *scroll = canvas.root().content()->append(std::make_unique<ttk::Scroll>());
        Block *content = static_cast<Block *>(scroll->hold(std::make_unique<Block>(10.0, 1200.0)));

        content->answers();
        canvas.frame();

        EXPECT_EQ(scroll->at(395, 100), scroll);
        EXPECT_EQ(scroll->at(391, 100), scroll);
        EXPECT_EQ(scroll->at(390.5, 100), content);
        EXPECT_EQ(scroll->at(200, 100), content);
        EXPECT_EQ(scroll->at(395, 300), nullptr) << "a point below the view was hit";
    }

    TEST(Scroll, LaneIsContentWhenNotScrollable) {
        tests::Canvas canvas;
        auto *scroll = canvas.root().content()->append(std::make_unique<ttk::Scroll>());
        Block *content = static_cast<Block *>(scroll->hold(std::make_unique<Block>(10.0, 100.0)));

        content->answers();
        canvas.frame();

        EXPECT_EQ(scroll->at(395, 50), content);
    }

    TEST(Scroll, EmptyRoomAnswersThePointer) {
        tests::Canvas canvas;
        auto *scroll = canvas.root().content()->append(std::make_unique<ttk::Scroll>());

        scroll->hold(std::make_unique<Block>(10.0, 100.0));
        canvas.frame();

        EXPECT_EQ(scroll->at(200, 50), scroll);
    }

    TEST(Scroll, HiddenScrollIsNotHit) {
        tests::Canvas canvas;
        auto *scroll = canvas.root().content()->append(std::make_unique<ttk::Scroll>());

        scroll->hold(std::make_unique<Block>(10.0, 1200.0));
        canvas.frame();
        scroll->set_visible(false);

        EXPECT_EQ(scroll->at(395, 50), nullptr);
    }

    TEST(Scroll, PressesOutsideTheLaneAreNotTaken) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        EXPECT_FALSE(mounted.scroll->press(ttk::Pointer{.x = 390, .y = 100}));
        EXPECT_EQ(mounted.scroll->offset(), 0.0);
    }

    TEST(Scroll, PressesAreNotTakenWhileNotScrollable) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 200.0);

        EXPECT_FALSE(mounted.scroll->press(ttk::Pointer{.x = 395, .y = 100}));
    }

    TEST(Scroll, PressOnTheTrackPagesByTheViewHeight) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        EXPECT_TRUE(mounted.scroll->press(ttk::Pointer{.x = 395, .y = 200}));
        EXPECT_EQ(mounted.scroll->offset(), 240.0);

        EXPECT_TRUE(mounted.scroll->press(ttk::Pointer{.x = 395, .y = 200}));
        EXPECT_EQ(mounted.scroll->offset(), 480.0);

        EXPECT_TRUE(mounted.scroll->press(ttk::Pointer{.x = 395, .y = 10}));
        EXPECT_EQ(mounted.scroll->offset(), 240.0);
    }

    TEST(Scroll, PagingStopsAtTheEnds) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        mounted.scroll->scroll_to(900.0);
        mounted.scroll->press(ttk::Pointer{.x = 395, .y = 236});

        EXPECT_EQ(mounted.scroll->offset(), 960.0);
    }

    TEST(Scroll, ThumbDragFollowsThePointer) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);
        ttk::Root &root = canvas.root();

        root.press(ttk::Pointer{.x = 395, .y = 24});

        ASSERT_EQ(root.grabbed(), mounted.scroll);

        root.motion(395, 24 + 96);

        EXPECT_EQ(mounted.scroll->offset(), 480.0);

        root.motion(395, 24 + 48);

        EXPECT_EQ(mounted.scroll->offset(), 240.0);

        root.motion(395, 24 + 192);

        EXPECT_EQ(mounted.scroll->offset(), 960.0);

        root.motion(395, 900);

        EXPECT_EQ(mounted.scroll->offset(), 960.0) << "dragging past the track ran past the end";

        root.motion(395, -500);

        EXPECT_EQ(mounted.scroll->offset(), 0.0) << "dragging above the track ran past the top";

        root.motion(100, 24 + 96);

        EXPECT_EQ(mounted.scroll->offset(), 480.0) << "the drag stopped when the pointer left the lane";

        root.release(ttk::Pointer{.x = 100, .y = 120});
        root.motion(395, 200);

        EXPECT_EQ(mounted.scroll->offset(), 480.0) << "motion after the release still dragged";
    }

    TEST(Scroll, DragWithoutAPressDoesNothing) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        mounted.scroll->drag(ttk::Pointer{.x = 395, .y = 200});

        EXPECT_EQ(mounted.scroll->offset(), 0.0);

        mounted.scroll->press(ttk::Pointer{.x = 395, .y = 200});
        mounted.scroll->drag(ttk::Pointer{.x = 395, .y = 10});

        EXPECT_EQ(mounted.scroll->offset(), 240.0) << "a track press started a thumb drag";
    }

    TEST(Scroll, DraggedThumbRepaintsMatchAFullRepaint) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);
        ttk::Root &root = canvas.root();

        root.press(ttk::Pointer{.x = 395, .y = 24});
        canvas.frame();

        for (const double y : {30.0, 61.0, 140.0, 90.0, 230.0}) {
            root.motion(395, y);
            canvas.frame();

            EXPECT_TRUE(canvas.matches_full()) << "at " << y;
        }

        root.release(ttk::Pointer{.x = 395, .y = 230});
        canvas.frame();

        EXPECT_TRUE(canvas.matches_full());
        EXPECT_GT(mounted.scroll->offset(), 0.0);
    }

    TEST(Scroll, WheelIsNotConsumedWhileNotScrollable) {
        tests::Canvas canvas;
        auto *holder = canvas.root().content()->append(std::make_unique<Holder>());
        auto *scroll = holder->append(std::make_unique<ttk::Scroll>());

        scroll->hold(std::make_unique<Stripes>(100.0));
        canvas.frame();

        EXPECT_FALSE(scroll->wheel(-1.0, ttk::Pointer{.x = 200, .y = 100}));

        canvas.root().wheel(-1.0, 200, 100);

        EXPECT_EQ(holder->wheeled, 1) << "the wheel did not go on to the parent";
        EXPECT_FALSE(canvas.root().busy());
    }

    TEST(Scroll, WheelIsConsumedWhileScrollable) {
        tests::Canvas canvas;
        auto *holder = canvas.root().content()->append(std::make_unique<Holder>());
        auto *scroll = holder->append(std::make_unique<ttk::Scroll>());

        scroll->hold(std::make_unique<Stripes>(1200.0));
        canvas.frame();
        canvas.root().wheel(-1.0, 200, 100);

        EXPECT_EQ(holder->wheeled, 0);
        EXPECT_TRUE(canvas.root().busy()) << "the wheel did not start a glide";
        EXPECT_EQ(scroll->offset(), 0.0) << "the wheel jumped instead of gliding";
    }

    TEST(Scroll, WheelNotchGlidesFiftyTwoPixels) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        canvas.root().wheel(-1.0, 200, 100);
        glide(canvas, 0.0);

        EXPECT_EQ(mounted.scroll->offset(), 52.0);
        EXPECT_FALSE(canvas.root().busy()) << "the glide never settled";
    }

    TEST(Scroll, PositiveStepsScrollTowardTheTop) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        mounted.scroll->scroll_to(300.0);
        canvas.root().wheel(2.0, 200, 100);
        glide(canvas, 0.0);

        EXPECT_EQ(mounted.scroll->offset(), 196.0);
    }

    TEST(Scroll, QuickNotchesRunTogether) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        canvas.root().wheel(-1.0, 200, 100);
        canvas.frame_at(FRAME);
        canvas.root().wheel(-1.0, 200, 100);
        canvas.frame_at(2 * FRAME);
        canvas.root().wheel(-1.0, 200, 100);
        glide(canvas, 2 * FRAME);

        EXPECT_EQ(mounted.scroll->offset(), 156.0);
    }

    TEST(Scroll, GlideMovesInWholePixelsTowardTheGoal) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);
        double now = 0.0;
        double last = 0.0;
        bool between = false;

        canvas.root().wheel(-2.0, 200, 100);

        for (int frame = 0; frame < FRAMES && canvas.root().busy(); ++frame) {
            now += FRAME;
            canvas.frame_at(now);

            const double offset = mounted.scroll->offset();

            EXPECT_EQ(offset, std::round(offset));
            EXPECT_GE(offset, last) << "the glide went backwards at " << now;
            EXPECT_LE(offset, 104.0) << "the glide overshot at " << now;

            between = between || (offset > 0.0 && offset < 104.0);
            last = offset;
        }

        EXPECT_TRUE(between) << "the glide jumped straight to the goal";
        EXPECT_EQ(last, 104.0);
    }

    TEST(Scroll, WheelStopsAtTheEnd) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        mounted.scroll->scroll_to(940.0);
        canvas.root().wheel(-5.0, 200, 100);
        glide(canvas, 0.0);

        EXPECT_EQ(mounted.scroll->offset(), 960.0);
    }

    TEST(Scroll, WheelAtTheTopIsStillConsumed) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        EXPECT_TRUE(mounted.scroll->wheel(1.0, ttk::Pointer{.x = 200, .y = 100}));

        glide(canvas, 0.0);

        EXPECT_EQ(mounted.scroll->offset(), 0.0);
        EXPECT_FALSE(canvas.root().busy());
    }

    TEST(Scroll, ScrollToStopsAGlide) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        canvas.root().wheel(-4.0, 200, 100);
        canvas.frame_at(FRAME);
        canvas.frame_at(2 * FRAME);
        mounted.scroll->scroll_to(10.0);
        glide(canvas, 2 * FRAME);

        EXPECT_EQ(mounted.scroll->offset(), 10.0);
        EXPECT_FALSE(mounted.scroll->advance(1.0));
    }

    TEST(Scroll, AdvanceWithoutAGlideIsIdle) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        EXPECT_FALSE(mounted.scroll->advance(1.0));
        EXPECT_EQ(mounted.scroll->offset(), 0.0);
    }

    TEST(Scroll, GlideFramesRepaintStripsThatMatchAFullRepaint) {
        tests::Canvas canvas;

        page(canvas, 1200.0);
        canvas.root().wheel(-3.0, 200, 100);

        double now = 0.0;

        for (int frame = 0; frame < FRAMES && canvas.root().busy(); ++frame) {
            now += FRAME;

            EXPECT_LT(canvas.frame_at(now), static_cast<std::size_t>(400 * 240 / 3))
                    << "a glide frame repainted the viewport at " << now;

            ASSERT_TRUE(canvas.matches_full()) << "glide partial/full mismatch at " << now;
        }
    }

    TEST(Scroll, ScrollMovesThePaintedPixels) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        canvas.root().take_shifts();
        mounted.scroll->scroll_to(30.0);

        const std::vector<ttk::Root::Shift> shifts = canvas.root().take_shifts();

        ASSERT_EQ(shifts.size(), 1U);
        EXPECT_EQ(shifts[0].dy, -30);
        EXPECT_EQ(shifts[0].region.x, 0);
        EXPECT_EQ(shifts[0].region.y, 0);
        EXPECT_EQ(shifts[0].region.w, 400);
        EXPECT_EQ(shifts[0].region.h, 240);

        mounted.scroll->scroll_to(10.0);

        EXPECT_EQ(canvas.root().take_shifts().at(0).dy, 20);
    }

    TEST(Scroll, FractionalViewShiftsOnlyWholePixels) {
        tests::Canvas canvas;
        const Mounted mounted = floating(canvas, {20.5, 30.25, 300, 150}, 1000.0);

        mounted.scroll->scroll_to(25.0);

        const std::vector<ttk::Root::Shift> shifts = canvas.root().take_shifts();

        ASSERT_EQ(shifts.size(), 1U);
        EXPECT_EQ(shifts[0].region.x, 21);
        EXPECT_EQ(shifts[0].region.y, 31);
        EXPECT_EQ(shifts[0].region.w, 299);
        EXPECT_EQ(shifts[0].region.h, 149);
    }

    TEST(Scroll, ViewOffTheRowGridScrollsMatchAFullRepaint) {
        tests::Canvas canvas;
        const Mounted mounted = floating(canvas, {20, 30.25, 300, 150.5}, 1000.0);

        for (const double offset : {3.0, 17.0, 9.0, 160.0, 140.0, 850.0, 0.0}) {
            mounted.scroll->scroll_to(offset);
            canvas.frame();

            EXPECT_TRUE(canvas.matches_full()) << "at " << offset;
        }
    }

    TEST(Scroll, ClippingAncestorRefusesTheShift) {
        tests::Canvas canvas;
        auto *holder = canvas.root().content()->append(std::make_unique<Holder>());
        auto *scroll = holder->append(std::make_unique<ttk::Scroll>());

        holder->clipping = true;
        scroll->hold(std::make_unique<Stripes>(1200.0));
        canvas.frame();
        scroll->scroll_to(30.0);

        EXPECT_TRUE(canvas.root().take_shifts().empty());

        scroll->scroll_to(45.0);

        EXPECT_GE(canvas.frame(), static_cast<std::size_t>(400 * 240)) << "a refused shift did not repaint the view";
        EXPECT_EQ(row_at(canvas, 100, 0), 2);
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST(Scroll, OverlayAboveRefusesTheShift) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);
        Block *overlay = canvas.root().layer(ttk::Root::POPUPS)->append(std::make_unique<Block>(0.0, 0.0, PLAIN));

        overlay->place({300, 100, 50, 50}, type());
        canvas.root().damage_all();
        canvas.frame();
        mounted.scroll->scroll_to(30.0);

        EXPECT_TRUE(canvas.root().take_shifts().empty()) << "pixels under a popup were moved";

        mounted.scroll->scroll_to(50.0);
        canvas.frame();

        EXPECT_EQ(canvas.at(320, 120), PLAIN) << "the popup was scrolled with the page";
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST(Scroll, HiddenOverlayAllowsTheShift) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);
        Block *overlay = canvas.root().layer(ttk::Root::POPUPS)->append(std::make_unique<Block>(0.0, 0.0, PLAIN));

        overlay->place({300, 100, 50, 50}, type());
        overlay->set_visible(false);
        canvas.frame();
        mounted.scroll->scroll_to(30.0);

        EXPECT_EQ(canvas.root().take_shifts().size(), 1U);
    }

    TEST(Scroll, ShortContentNeverShifts) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 100.0);

        mounted.scroll->scroll_to(50.0);

        EXPECT_EQ(mounted.scroll->offset(), 0.0);
        EXPECT_TRUE(canvas.root().take_shifts().empty());
        EXPECT_FALSE(canvas.root().dirty());
    }

    TEST(Scroll, RepeatedOffsetDoesNothing) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        mounted.scroll->scroll_to(40.0);
        canvas.frame();
        mounted.scroll->scroll_to(40.2);

        EXPECT_FALSE(canvas.root().dirty());
        EXPECT_EQ(canvas.frame(), 0U);
    }

    TEST(Scroll, TallerViewClampsTheOffset) {
        tests::Canvas canvas;
        const Mounted mounted = floating(canvas, {0, 0, 300, 100}, 1000.0);

        mounted.scroll->scroll_to(900.0);
        mounted.scroll->place({0, 0, 300, 200}, type());

        EXPECT_EQ(mounted.scroll->offset(), 800.0);
        EXPECT_TRUE(mounted.scroll->at_end());
        EXPECT_TRUE(boxed(mounted.content->box(), {0, -800, 300, 1000}));

        mounted.scroll->place({0, 0, 300, 1500}, type());

        EXPECT_EQ(mounted.scroll->offset(), 0.0);
        EXPECT_FALSE(mounted.scroll->scrollable());
        EXPECT_TRUE(boxed(mounted.content->box(), {0, 0, 300, 1500}));
    }

    TEST(Scroll, ResizedViewRepaintsToMatchAFullRepaint) {
        tests::Canvas canvas;
        const Mounted mounted = floating(canvas, {0, 0, 300, 100}, 1000.0);

        mounted.scroll->scroll_to(900.0);
        canvas.frame();
        mounted.scroll->invalidate();
        mounted.scroll->place({0, 0, 300, 220}, type());
        mounted.scroll->invalidate();
        canvas.frame();

        EXPECT_EQ(row_at(canvas, 100, 219), 49);
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST(Scroll, ShrunkContentClampsTheOffset) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        mounted.scroll->scroll_to(900.0);
        canvas.frame();
        mounted.content->tall = 500.0;
        canvas.root().relayout();
        canvas.frame();

        EXPECT_EQ(mounted.scroll->offset(), 260.0);
        EXPECT_EQ(row_at(canvas, 100, 239), 24);
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST(Scroll, ContentChangeAfterAScrollIsPainted) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        mounted.scroll->scroll_to(100.0);
        mounted.content->marked = 8;
        mounted.content->invalidate({0, mounted.content->box().y + (8 * ROW), 400, ROW});
        canvas.frame();

        EXPECT_EQ(canvas.at(100, 70), 0xffffffffU) << "the change was painted where the row was before the scroll";
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST(Scroll, ContentChangeThenScrollInOneFrame) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        mounted.scroll->scroll_to(100.0);
        canvas.frame();
        mounted.content->marked = 8;
        mounted.content->invalidate({0, mounted.content->box().y + (8 * ROW), 400, ROW});
        mounted.scroll->scroll_to(130.0);
        mounted.scroll->scroll_to(115.0);
        canvas.frame();

        EXPECT_EQ(canvas.at(100, 50), 0xffffffffU);
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST(Scroll, RefitAfterAScrollMatchesAFullRepaint) {
        tests::Canvas canvas;
        const Mounted mounted = page(canvas, 1200.0);

        mounted.scroll->scroll_to(300.0);
        mounted.content->tall = 2000.0;
        mounted.scroll->refit(type(), 0.0);
        mounted.scroll->scroll_to(320.0);
        canvas.frame();

        EXPECT_EQ(mounted.scroll->reach(), 2000.0);
        EXPECT_TRUE(canvas.matches_full());
    }
}
