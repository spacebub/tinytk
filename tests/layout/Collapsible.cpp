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

#include "Blocks.h"
#include "support/Canvas.h"
#include "ttk/draw/Glyphs.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/layout/Box.h"
#include "ttk/toolkit/layout/Collapsible.h"

namespace {
    using tests::Block;
    using tests::boxed;

    constexpr double START = 1.0;
    constexpr double SLIDE = 0.22;
    constexpr double BODY = 100.0;
    constexpr std::uint32_t INSIDE = 0xffc02040U;
    constexpr std::uint32_t UNDER = 0xff20c040U;

    // A CubicOut slide halfway through its time has covered 1 - 0.5^3 of the way.
    constexpr double HALFWAY = 0.875;

    constexpr double PANEL_CLOSED = 42.0 + 32.0;
    constexpr double PANEL_OPEN = PANEL_CLOSED + BODY + 16.0;
    constexpr double HEADING_CLOSED = 18.0;
    constexpr double HEADING_OPEN = HEADING_CLOSED + BODY + 12.0;

    ttk::Typeface &type() {
        return tests::fonts();
    }

    // A folding widget at the top of a column, with a block under it that moves as it slides.
    template <typename Folding>
    class Mounted : public ::testing::Test {
    protected:
        void SetUp() override {
            if (!tests::fonts_loaded()) {
                GTEST_SKIP() << "no font installed";
            }

            ttk::Box *column = canvas.root().content()->append(ttk::Box::column());

            folding = column->append(make());
            inside = folding->body()->append(std::make_unique<Block>(10.0, BODY, INSIDE));
            under = column->append(std::make_unique<Block>(10.0, 40.0, UNDER));
            under->fixedWidth = 100.0;

            // The first layout hides the body of the closed widget, which asks for one more.
            canvas.frame_at(START);
            canvas.frame_at(START);
        }

        virtual std::unique_ptr<Folding> make() = 0;

        void fold(const bool open, const double at) {
            canvas.root().set_now(at);
            folding->set_open(open);
            canvas.frame_at(at);
        }

        void run(const double from, const double to) {
            for (int frame = 0; from + (frame / 60.0) < to; ++frame) {
                canvas.frame_at(from + (frame / 60.0));
            }

            canvas.frame_at(to);
        }

        tests::Canvas canvas;
        Folding *folding = nullptr;
        Block *inside = nullptr;
        Block *under = nullptr;
        int calls = 0;
        std::vector<bool> asked;
    };

    class Panel : public Mounted<ttk::CollapsiblePanel> {
    protected:
        std::unique_ptr<ttk::CollapsiblePanel> make() override {
            return std::make_unique<ttk::CollapsiblePanel>("Section", [this](const bool open) {
                ++calls;
                asked.push_back(open);
            });
        }
    };

    class Heading : public Mounted<ttk::DisclosureHeading> {
    protected:
        std::unique_ptr<ttk::DisclosureHeading> make() override {
            return std::make_unique<ttk::DisclosureHeading>("Section", [this] { ++calls; });
        }
    };

    TEST_F(Panel, StartsClosed) {
        EXPECT_EQ(folding->natural_height(type(), 400.0), PANEL_CLOSED);
        EXPECT_EQ(folding->natural_height(type(), 150.0), PANEL_CLOSED);
        EXPECT_EQ(folding->box().h, PANEL_CLOSED);
        EXPECT_FALSE(folding->body()->visible()) << "a closed panel shows its body";
        EXPECT_EQ(under->box().y, PANEL_CLOSED);
    }

    TEST_F(Panel, OffersItsParts) {
        ASSERT_NE(folding->tools(), nullptr);
        ASSERT_NE(folding->body(), nullptr);
        ASSERT_NE(folding->pill(), nullptr);
        EXPECT_FALSE(folding->pill()->visible()) << "the pill shows before the caller asks for it";
        EXPECT_TRUE(folding->tools()->children().empty());
    }

    TEST_F(Panel, BodyIsInsetSixteenPixels) {
        fold(true, START);
        run(START, START + SLIDE);

        EXPECT_TRUE(boxed(folding->body()->box(), {16, 16 + 42 + 16, 400 - 32, BODY}));
        EXPECT_TRUE(boxed(inside->box(), {16, 74, 368, BODY}));
    }

    TEST_F(Panel, OpenHeightAddsTheBody) {
        fold(true, START);
        run(START, START + SLIDE);

        EXPECT_EQ(folding->natural_height(type(), 400.0), PANEL_OPEN);
        EXPECT_EQ(folding->box().h, PANEL_OPEN);
        EXPECT_TRUE(folding->body()->visible());
        EXPECT_EQ(under->box().y, PANEL_OPEN);
    }

    TEST_F(Panel, OpeningSlidesTheHeight) {
        fold(true, START);

        EXPECT_EQ(folding->box().h, PANEL_CLOSED) << "the panel jumped at the start of the slide";
        EXPECT_FALSE(folding->body()->visible());

        canvas.frame_at(START + (SLIDE / 2.0));

        EXPECT_DOUBLE_EQ(folding->box().h, PANEL_CLOSED + ((BODY + 16.0) * HALFWAY));
        EXPECT_TRUE(folding->body()->visible());
        EXPECT_EQ(under->box().y, folding->box().h) << "the block under did not move with the slide";

        canvas.frame_at(START + SLIDE);

        EXPECT_EQ(folding->box().h, PANEL_OPEN);
    }

    TEST_F(Panel, ClosingSlidesOverTheHeightItHad) {
        fold(true, START);
        run(START, START + SLIDE);
        fold(false, START + 1.0);
        folding->body()->clear();
        canvas.root().relayout();
        canvas.frame_at(START + 1.0 + (SLIDE / 2.0));

        EXPECT_DOUBLE_EQ(folding->box().h, PANEL_CLOSED + ((BODY + 16.0) * (1.0 - HALFWAY)))
                << "an emptied body collapsed the slide";

        canvas.frame_at(START + 1.0 + SLIDE);

        EXPECT_EQ(folding->box().h, PANEL_CLOSED);
        EXPECT_FALSE(folding->body()->visible());
    }

    TEST_F(Panel, SetOpenToTheSameStateDoesNothing) {
        ASSERT_FALSE(canvas.root().dirty());

        folding->set_open(false);

        EXPECT_FALSE(canvas.root().dirty());
        EXPECT_FALSE(canvas.root().busy());
    }

    TEST_F(Panel, SetOpenAsksForALayoutAndWakes) {
        folding->set_open(true);

        EXPECT_TRUE(canvas.root().dirty());
        EXPECT_TRUE(canvas.root().busy());
    }

    TEST_F(Panel, AdvanceStopsWhenTheSlideEnds) {
        fold(true, START);

        EXPECT_TRUE(folding->advance(START + 0.1));
        EXPECT_FALSE(folding->advance(START + SLIDE + 0.001));

        canvas.frame_at(START + SLIDE + 0.1);

        EXPECT_FALSE(canvas.root().busy());
    }

    TEST_F(Panel, BodyIsCutOffAtThePanelEdgeWhileSliding) {
        fold(true, START);
        canvas.frame_at(START + 0.05);

        const BLRect box = folding->box();

        ASSERT_LT(box.y + box.h, inside->box().y + inside->box().h) << "the slide was already past the body";
        ASSERT_GT(box.y + box.h, inside->box().y + 10.0);

        EXPECT_EQ(canvas.at(200, static_cast<int>(inside->box().y + 4.0)), INSIDE);
        EXPECT_EQ(canvas.at(50, static_cast<int>(box.y + box.h + 3.0)), UNDER);
        EXPECT_EQ(canvas.at(200, static_cast<int>(box.y + box.h + 3.0)), ttk::Theme::palette().background.value)
                << "the body was painted past the panel";
    }

    TEST_F(Panel, ClipsToItsBox) {
        BLRect region{};

        ASSERT_TRUE(folding->clips(region));
        EXPECT_TRUE(boxed(region, folding->box()));
    }

    TEST_F(Panel, EveryFrameOfTheSlideMatchesAFullRepaint) {
        fold(true, START);

        for (int frame = 1; frame <= 15; ++frame) {
            canvas.frame_at(START + (frame / 60.0));

            ASSERT_TRUE(canvas.matches_full()) << "opening, frame " << frame;
        }

        fold(false, START + 1.0);

        for (int frame = 1; frame <= 15; ++frame) {
            canvas.frame_at(START + 1.0 + (frame / 60.0));

            ASSERT_TRUE(canvas.matches_full()) << "closing, frame " << frame;
        }
    }

    TEST_F(Panel, ClickOnTheHeadingAsksForTheOtherState) {
        canvas.root().press(ttk::Pointer{.x = 100, .y = 30});

        EXPECT_EQ(canvas.root().grabbed(), folding);

        canvas.root().release(ttk::Pointer{.x = 100, .y = 30});

        ASSERT_EQ(calls, 1);
        EXPECT_TRUE(asked[0]);

        canvas.frame_at(START + 1.0);

        EXPECT_EQ(folding->box().h, PANEL_CLOSED) << "the panel opened by itself";
    }

    TEST_F(Panel, ClickOnAnOpenHeadingAsksToClose) {
        fold(true, START);
        run(START, START + SLIDE);
        canvas.root().press(ttk::Pointer{.x = 100, .y = 30});
        canvas.root().release(ttk::Pointer{.x = 100, .y = 30});

        ASSERT_EQ(calls, 1);
        EXPECT_FALSE(asked[0]);
    }

    TEST_F(Panel, ReleaseOffTheHeadingDoesNotFold) {
        canvas.root().press(ttk::Pointer{.x = 100, .y = 30});
        canvas.root().release(ttk::Pointer{.x = 100, .y = 200});

        EXPECT_EQ(calls, 0);
    }

    TEST_F(Panel, PressOnTheBodyIsNotTaken) {
        fold(true, START);
        run(START, START + SLIDE);

        EXPECT_FALSE(folding->press(ttk::Pointer{.x = 200, .y = 120}));

        canvas.root().press(ttk::Pointer{.x = 200, .y = 120});

        EXPECT_EQ(canvas.root().grabbed(), nullptr);
    }

    TEST_F(Panel, ClicksOnTheToolsDoNotFold) {
        Block *tool = folding->tools()->append(std::make_unique<Block>(30.0, 30.0));

        canvas.root().relayout();
        canvas.frame_at(START);

        const double x = tool->box().x + 10.0;

        ASSERT_GT(tool->box().w, 0.0);
        EXPECT_FALSE(folding->press(ttk::Pointer{.x = x, .y = 30}));

        canvas.root().press(ttk::Pointer{.x = x, .y = 30});
        canvas.root().release(ttk::Pointer{.x = x, .y = 30});

        EXPECT_EQ(calls, 0);
        EXPECT_EQ(folding->cursor_at(x, 30), ttk::Cursor::Default);
    }

    TEST_F(Panel, PointingHandOnlyOverTheHeading) {
        EXPECT_EQ(folding->cursor_at(100, 30), ttk::Cursor::Pointer);
        EXPECT_EQ(folding->cursor_at(100, 60), ttk::Cursor::Default);
    }

    TEST_F(Panel, HoverRepaintsOnlyWhenItCrossesTheHeading) {
        folding->hover(ttk::Pointer{.x = 100, .y = 30});

        EXPECT_TRUE(canvas.root().dirty());

        canvas.frame_at(START);
        folding->hover(ttk::Pointer{.x = 120, .y = 32});

        EXPECT_FALSE(canvas.root().dirty()) << "moving within the heading repainted";

        folding->hover(ttk::Pointer{.x = 120, .y = 65});

        EXPECT_TRUE(canvas.root().dirty());

        canvas.frame_at(START);

        EXPECT_TRUE(canvas.matches_full());
    }

    TEST_F(Panel, LeaveClearsTheHeadingHighlight) {
        folding->hover(ttk::Pointer{.x = 100, .y = 30});
        canvas.frame_at(START);

        const std::vector<std::uint32_t> lit = canvas.pixels();

        folding->leave();
        canvas.frame_at(START);
        folding->hover(ttk::Pointer{.x = 100, .y = 30});

        EXPECT_TRUE(canvas.root().dirty()) << "the highlight was still set after leaving";

        canvas.frame_at(START);

        EXPECT_EQ(canvas.pixels(), lit);
    }

    TEST_F(Panel, ChevronBrightensOverTheHeading) {
        const std::vector<std::uint32_t> resting = canvas.pixels();

        canvas.root().motion(100, 30);
        canvas.frame_at(START);

        EXPECT_NE(canvas.pixels(), resting);
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST_F(Panel, SaidTextIsPaintedInItsTone) {
        const std::vector<std::uint32_t> bare = canvas.pixels();

        folding->set_said("Saved", false);
        folding->invalidate();
        canvas.frame_at(START);

        const std::vector<std::uint32_t> faint = canvas.pixels();

        EXPECT_NE(faint, bare) << "the status text was not drawn";

        folding->set_said("Saved", true);
        folding->invalidate();
        canvas.frame_at(START);

        EXPECT_NE(canvas.pixels(), faint) << "the warning tone looks like the faint one";
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST_F(Panel, ShownPillTakesRoomInTheHeading) {
        folding->pill()->set_text("New");
        folding->pill()->set_visible(true);
        canvas.frame_at(START);

        EXPECT_GT(folding->pill()->box().w, 0.0);
        EXPECT_EQ(folding->pill()->box().h, 22.0);
        EXPECT_GT(folding->pill()->box().x, folding->title_width(type())) << "the pill sits over the title";
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST_F(Panel, TitleWidthCoversTheChevronGapAndTitle) {
        const ttk::CollapsiblePanel longer("A much longer section title", nullptr);

        EXPECT_GT(folding->title_width(type()), ttk::Glyphs::span(1.0F) + 10.0);
        EXPECT_GT(longer.title_width(type()), folding->title_width(type()));
    }

    TEST_F(Heading, StartsClosed) {
        EXPECT_EQ(folding->natural_height(type(), 400.0), HEADING_CLOSED);
        EXPECT_EQ(folding->box().h, HEADING_CLOSED);
        EXPECT_FALSE(folding->body()->visible());
        EXPECT_EQ(under->box().y, HEADING_CLOSED);
    }

    TEST_F(Heading, BodyIsAsWideAsTheHeading) {
        fold(true, START);
        run(START, START + SLIDE);

        EXPECT_TRUE(boxed(folding->body()->box(), {0, 30, 400, BODY}));
        EXPECT_TRUE(boxed(inside->box(), {0, 30, 400, BODY}));
    }

    TEST_F(Heading, OpenHeightAddsTheGapAndBody) {
        fold(true, START);
        run(START, START + SLIDE);

        EXPECT_EQ(folding->natural_height(type(), 400.0), HEADING_OPEN);
        EXPECT_EQ(folding->box().h, HEADING_OPEN);
        EXPECT_TRUE(folding->body()->visible());
    }

    TEST_F(Heading, OpeningSlidesTheHeight) {
        fold(true, START);

        EXPECT_EQ(folding->box().h, HEADING_CLOSED);

        canvas.frame_at(START + (SLIDE / 2.0));

        EXPECT_DOUBLE_EQ(folding->box().h, HEADING_CLOSED + ((BODY + 12.0) * HALFWAY));
        EXPECT_EQ(under->box().y, folding->box().h);

        canvas.frame_at(START + SLIDE);

        EXPECT_EQ(folding->box().h, HEADING_OPEN);
    }

    TEST_F(Heading, ClosingSlidesOverTheHeightItHad) {
        fold(true, START);
        run(START, START + SLIDE);
        fold(false, START + 1.0);
        folding->body()->clear();
        canvas.root().relayout();
        canvas.frame_at(START + 1.0 + (SLIDE / 2.0));

        EXPECT_DOUBLE_EQ(folding->box().h, HEADING_CLOSED + ((BODY + 12.0) * (1.0 - HALFWAY)));

        canvas.frame_at(START + 1.0 + SLIDE);

        EXPECT_EQ(folding->box().h, HEADING_CLOSED);
        EXPECT_FALSE(folding->body()->visible());
        EXPECT_FALSE(canvas.root().busy());
    }

    TEST_F(Heading, SetOpenToTheSameStateDoesNothing) {
        folding->set_open(false);

        EXPECT_FALSE(canvas.root().dirty());
        EXPECT_FALSE(canvas.root().busy());
    }

    TEST_F(Heading, BodyIsCutOffAtTheHeadingEdgeWhileSliding) {
        fold(true, START);
        canvas.frame_at(START + 0.05);

        const double bottom = folding->box().y + folding->box().h;

        ASSERT_LT(bottom, inside->box().y + inside->box().h);
        ASSERT_GT(bottom, inside->box().y + 10.0);

        EXPECT_EQ(canvas.at(200, static_cast<int>(inside->box().y + 4.0)), INSIDE);
        EXPECT_EQ(canvas.at(50, static_cast<int>(bottom + 3.0)), UNDER);
        EXPECT_EQ(canvas.at(200, static_cast<int>(bottom + 3.0)), ttk::Theme::palette().background.value)
                << "the body was painted past the heading";
    }

    TEST_F(Heading, ClipsToItsBox) {
        BLRect region{};

        ASSERT_TRUE(folding->clips(region));
        EXPECT_TRUE(boxed(region, folding->box()));
    }

    TEST_F(Heading, ClickAnywhereOnTheRowTurnsIt) {
        for (const double x : {5.0, 200.0, 395.0}) {
            canvas.root().press(ttk::Pointer{.x = x, .y = 9});
            canvas.root().release(ttk::Pointer{.x = x, .y = 9});
        }

        EXPECT_EQ(calls, 3);

        canvas.frame_at(START + 1.0);

        EXPECT_EQ(folding->box().h, HEADING_CLOSED) << "the heading opened by itself";
    }

    TEST_F(Heading, ClickOnTheBodyDoesNotTurnIt) {
        fold(true, START);
        run(START, START + SLIDE);

        EXPECT_FALSE(folding->press(ttk::Pointer{.x = 200, .y = 60}));

        canvas.root().press(ttk::Pointer{.x = 200, .y = 9});
        canvas.root().release(ttk::Pointer{.x = 200, .y = 60});

        EXPECT_EQ(calls, 0) << "a release over the body turned the heading";
    }

    TEST_F(Heading, PointingHandOnlyOverTheRow) {
        EXPECT_EQ(folding->cursor_at(390, 10), ttk::Cursor::Pointer);
        EXPECT_EQ(folding->cursor_at(10, 18), ttk::Cursor::Default);
    }

    TEST_F(Heading, HoverRepaintsOnlyWhenItCrossesTheRow) {
        folding->hover(ttk::Pointer{.x = 100, .y = 9});

        EXPECT_TRUE(canvas.root().dirty());

        canvas.frame_at(START);
        folding->hover(ttk::Pointer{.x = 300, .y = 12});

        EXPECT_FALSE(canvas.root().dirty());

        folding->hover(ttk::Pointer{.x = 300, .y = 40});

        EXPECT_TRUE(canvas.root().dirty());
    }

    TEST_F(Heading, HoverBrightensAndLeaveDims) {
        const std::vector<std::uint32_t> resting = canvas.pixels();

        folding->hover(ttk::Pointer{.x = 100, .y = 9});
        canvas.frame_at(START);

        EXPECT_NE(canvas.pixels(), resting);

        folding->leave();
        canvas.frame_at(START);

        EXPECT_EQ(canvas.pixels(), resting) << "leaving did not dim the heading";
    }

    TEST_F(Heading, SaidTextIsPaintedAfterALayout) {
        const std::vector<std::uint32_t> bare = canvas.pixels();

        folding->set_said("3 items");
        canvas.root().relayout();
        canvas.frame_at(START);

        EXPECT_NE(canvas.pixels(), bare);
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST_F(Heading, EveryFrameOfTheSlideMatchesAFullRepaint) {
        fold(true, START);

        for (int frame = 1; frame <= 15; ++frame) {
            canvas.frame_at(START + (frame / 60.0));

            ASSERT_TRUE(canvas.matches_full()) << "opening, frame " << frame;
        }

        fold(false, START + 1.0);

        for (int frame = 1; frame <= 15; ++frame) {
            canvas.frame_at(START + 1.0 + (frame / 60.0));

            ASSERT_TRUE(canvas.matches_full()) << "closing, frame " << frame;
        }
    }
}
