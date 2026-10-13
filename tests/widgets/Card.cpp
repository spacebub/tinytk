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
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Screen.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/system/Env.h"
#include "ttk/toolkit/controls/Button.h"
#include "ttk/toolkit/controls/Card.h"
#include "ttk/toolkit/controls/Pill.h"

namespace {
using widgets::Painted;

constexpr BLRect BOX{20, 20, 300, 180};
constexpr BLRectI PIXELS{20, 20, 300, 180};

// Left of every text and clear of the border and corners.
constexpr int PLAIN_X = 26;
constexpr int PLAIN_Y = 110;

constexpr BLRectI TITLE{36, 36, 200, 22};
constexpr BLRectI SUBTITLE{36, 66, 268, 40};
constexpr BLRectI TOLD{36, 126, 268, 16};
constexpr int TRACK_Y = 134;

// Inside the right cap of the rightmost badge, past its text.
constexpr int BADGE_X = 301;
constexpr int BADGE_Y = 47;

struct Mounted {
    tests::Canvas canvas;
    ttk::Card *card = nullptr;

    int opened = 0;
    int started = 0;
    int moved = 0;
    int ended = 0;
    double startX = 0.0;
    double startY = 0.0;
    double movedX = 0.0;
    double movedY = 0.0;

    explicit Mounted(const std::string &title = "Eviternity") : Mounted(prepared(title)) {}

    explicit Mounted(std::unique_ptr<ttk::Card> made) {
        made->opened = [this] { ++opened; };
        made->drag_started = [this](const double x, const double y) {
            ++started;
            startX = x;
            startY = y;
        };
        made->drag_moved = [this](const double x, const double y) {
            ++moved;
            movedX = x;
            movedY = y;
        };
        made->drag_ended = [this] { ++ended; };
        card = widgets::mount(canvas, std::move(made), BOX);
    }

    static std::unique_ptr<ttk::Card> prepared(const std::string &title) {
        auto made = std::make_unique<ttk::Card>();

        made->title = title;

        return made;
    }

    [[nodiscard]] std::uint32_t plain() const { return canvas.at(PLAIN_X, PLAIN_Y); }
    [[nodiscard]] std::uint32_t edge() const { return canvas.at(PIXELS.x, PLAIN_Y); }

    [[nodiscard]] std::uint32_t peak(const BLRectI &region) const {
        return widgets::peak(canvas, region, widgets::palette().surface.value);
    }

    void press(const double x, const double y) {
        canvas.root().motion(x, y);
        canvas.root().press(ttk::Pointer{.x = x, .y = y});
    }

    void release(const double x, const double y) { canvas.root().release(ttk::Pointer{.x = x, .y = y}); }
};

ttk::Card::Tag tag(std::string text, const ttk::Pill::Kind kind, const bool dot = false, std::string hint = {}) {
    return ttk::Card::Tag{.text = std::move(text), .kind = kind, .dot = dot, .hint = std::move(hint)};
}

bool nearer(const std::uint32_t pixel, const BLRgba32 wanted, const BLRgba32 other) {
    return widgets::distance(pixel, wanted.value) < widgets::distance(pixel, other.value);
}

TEST(Card, StartsUnplacedAndStill) {
    const ttk::Card card;

    EXPECT_FALSE(card.draggable);
    EXPECT_EQ(card.slot(), -1);
    EXPECT_FALSE(card.carrying());
    EXPECT_FALSE(card.sliding());
    EXPECT_DOUBLE_EQ(card.slide_x(), 0.0);
    EXPECT_DOUBLE_EQ(card.slide_y(), 0.0);
    EXPECT_EQ(card.cursor_at(0, 0), ttk::Cursor::Default);
    EXPECT_TRUE(card.tags.empty());
    EXPECT_FALSE(card.working);
    EXPECT_FALSE(card.trouble);
}

TEST(Card, ButtonRowStartsEmpty) {
    const ttk::Card card;

    ASSERT_NE(card.buttons(), nullptr);
    EXPECT_TRUE(card.buttons()->children().empty());
    EXPECT_EQ(card.buttons()->parent(), &card);
}

TEST(Card, SetSlotIsRemembered) {
    ttk::Card card;

    card.set_slot(7);

    EXPECT_EQ(card.slot(), 7);

    card.set_slot(0);

    EXPECT_EQ(card.slot(), 0);
}

TEST(Card, IsALitHoverablePanel) {
    const ttk::Card card;

    EXPECT_TRUE(card.hoverable);
    EXPECT_FALSE(card.lit);
}

TEST(Card, SlideFromEasesBackToItsCell) {
    ttk::Card card;

    card.slide_from(30, -20, 1.0);

    EXPECT_TRUE(card.sliding());
    EXPECT_DOUBLE_EQ(card.slide_x(), 30.0);
    EXPECT_DOUBLE_EQ(card.slide_y(), -20.0);

    EXPECT_TRUE(card.advance(1.05));
    EXPECT_GT(card.slide_x(), 0.0);
    EXPECT_LT(card.slide_x(), 30.0);
    EXPECT_LT(card.slide_y(), 0.0);
    EXPECT_GT(card.slide_y(), -20.0);

    EXPECT_FALSE(card.advance(1.2)) << "the slide ran past 0.19 seconds";
    EXPECT_FALSE(card.sliding());
    EXPECT_DOUBLE_EQ(card.slide_x(), 0.0);
    EXPECT_DOUBLE_EQ(card.slide_y(), 0.0);
}

TEST(Card, ArrangePlacesTheButtonRowAlongTheBottom) {
    ttk::Card card;

    card.place(BOX, tests::fonts());

    const BLRect &row = card.buttons()->box();

    EXPECT_DOUBLE_EQ(row.x, BOX.x + 16);
    EXPECT_DOUBLE_EQ(row.y, BOX.y + BOX.h - 16 - ttk::Theme::controlSmall);
    EXPECT_DOUBLE_EQ(row.w, BOX.w - 32);
    EXPECT_DOUBLE_EQ(row.h, ttk::Theme::controlSmall);
}

TEST_F(Painted, CardPaintsItsSurfaceAndBorder) {
    const Mounted mounted;

    EXPECT_EQ(mounted.plain(), widgets::palette().surface.value);
    EXPECT_EQ(mounted.edge(), widgets::palette().border.value);
    EXPECT_EQ(widgets::outside(mounted.canvas, PIXELS), 0U);
}

TEST_F(Painted, CardPaintsTheTitleInText) {
    const Mounted mounted;

    EXPECT_TRUE(nearer(mounted.peak(TITLE), widgets::palette().text, widgets::palette().danger))
            << std::hex << mounted.peak(TITLE);
    EXPECT_LT(widgets::distance(mounted.peak(TITLE), widgets::palette().text.value), 40);
}

TEST_F(Painted, CardTroublePaintsTitleAndStatusInDanger) {
    auto made = Mounted::prepared("Eviternity");

    made->trouble = true;
    made->told = "Exited with code 1";

    const Mounted mounted(std::move(made));

    EXPECT_TRUE(nearer(mounted.peak(TITLE), widgets::palette().danger, widgets::palette().text))
            << std::hex << mounted.peak(TITLE);
    EXPECT_TRUE(nearer(mounted.peak(TOLD), widgets::palette().danger, widgets::palette().faint))
            << std::hex << mounted.peak(TOLD);
}

TEST_F(Painted, CardToldIsFaintAboveTheButtons) {
    auto made = Mounted::prepared("Eviternity");

    made->told = "Last played yesterday";

    const Mounted mounted(std::move(made));
    const widgets::Ink drawn = widgets::ink(mounted.canvas, TOLD, widgets::palette().surface.value);

    EXPECT_GT(drawn.pixels, 0U);
    EXPECT_TRUE(nearer(mounted.peak(TOLD), widgets::palette().faint, widgets::palette().text))
            << std::hex << mounted.peak(TOLD);
}

TEST_F(Painted, CardSubtitleIsWrappedFaintText) {
    auto made = Mounted::prepared("Eviternity");

    made->subtitle = "A megawad of thirty two maps over six chapters, each one with its own sky and music.";

    const Mounted mounted(std::move(made));
    const widgets::Ink drawn = widgets::ink(mounted.canvas, SUBTITLE, widgets::palette().surface.value);

    EXPECT_GT(drawn.bottom - drawn.top, 20) << "the subtitle did not wrap to a second line";
    EXPECT_LE(drawn.right, SUBTITLE.x + SUBTITLE.w);
    EXPECT_TRUE(nearer(mounted.peak(SUBTITLE), widgets::palette().faint, widgets::palette().text));
}

TEST_F(Painted, CardFileWritesTheHomeAsTilde) {
    const std::string home = ttk::Env::get("HOME");

    ASSERT_FALSE(home.empty());

    auto atHome = Mounted::prepared("Eviternity");
    auto tilde = Mounted::prepared("Eviternity");

    atHome->file = home + "/wads/eviternity.wad";
    tilde->file = "~/wads/eviternity.wad";

    const Mounted one(std::move(atHome));
    const Mounted two(std::move(tilde));
    const Mounted none;

    EXPECT_EQ(one.canvas.pixels(), two.canvas.pixels());
    EXPECT_NE(one.canvas.pixels(), none.canvas.pixels());
}

TEST_F(Painted, CardLongFileStaysInsideTheCard) {
    auto made = Mounted::prepared("Eviternity");

    made->file = "/srv/a/very/deep/tree/of/folders/that/goes/on/and/on/for/a/long/while/eviternity.wad";

    const Mounted mounted(std::move(made));
    const widgets::Ink drawn = widgets::ink(mounted.canvas, BLRectI{21, 60, 298, 24}, widgets::palette().surface.value);

    EXPECT_GT(drawn.pixels, 0U);
    EXPECT_LE(drawn.right, 304) << "the file ran into the right inset";
}

TEST_F(Painted, CardWorkingDrawsAProgressBar) {
    auto made = Mounted::prepared("Eviternity");

    made->working = true;
    made->progress = 0.5;

    const Mounted mounted(std::move(made));

    EXPECT_EQ(mounted.canvas.at(40, TRACK_Y), widgets::palette().accent.value);
    EXPECT_EQ(mounted.canvas.at(160, TRACK_Y), widgets::palette().accent.value);
    EXPECT_EQ(mounted.canvas.at(180, TRACK_Y), widgets::palette().sunken.value);
    EXPECT_EQ(mounted.canvas.at(300, TRACK_Y), widgets::palette().sunken.value);
}

TEST_F(Painted, CardWorkingHidesTheStatusLine) {
    auto told = Mounted::prepared("Eviternity");
    auto silent = Mounted::prepared("Eviternity");

    told->working = true;
    told->told = "Downloading";
    silent->working = true;

    const Mounted one(std::move(told));
    const Mounted two(std::move(silent));

    EXPECT_EQ(one.canvas.pixels(), two.canvas.pixels());
}

TEST_F(Painted, CardProgressIsClamped) {
    auto over = Mounted::prepared("Eviternity");
    auto under = Mounted::prepared("Eviternity");

    over->working = true;
    over->progress = 3.0;
    under->working = true;
    under->progress = -1.0;

    const Mounted full(std::move(over));
    const Mounted empty(std::move(under));

    EXPECT_EQ(full.canvas.at(300, TRACK_Y), widgets::palette().accent.value);
    EXPECT_EQ(widgets::outside(full.canvas, PIXELS), 0U) << "an overfull bar ran out of the card";
    EXPECT_EQ(empty.canvas.at(40, TRACK_Y), widgets::palette().sunken.value);
}

TEST_F(Painted, CardTagsAreBadgesFromTheRight) {
    auto made = Mounted::prepared("Eviternity");

    made->tags = {tag("Mod", ttk::Pill::Kind::Muted), tag("Ready", ttk::Pill::Kind::Success, true)};

    const Mounted mounted(std::move(made));

    EXPECT_EQ(mounted.canvas.at(BADGE_X, BADGE_Y), ttk::Pill::wash_of(ttk::Pill::Kind::Success).value)
            << "the last badge is not rightmost";

    const widgets::Ink badges =
            widgets::ink(mounted.canvas, BLRectI{100, 36, 205, 22}, widgets::palette().surface.value);

    EXPECT_LE(badges.right, 304) << "a badge ran into the right inset";
    EXPECT_LT(badges.left, 230) << "the first badge was not drawn left of the second";
}

TEST_F(Painted, CardTagDotIsDrawnInTheTone) {
    auto made = Mounted::prepared("Eviternity");

    made->tags = {tag("", ttk::Pill::Kind::Danger, true)};

    const Mounted mounted(std::move(made));

    // An empty badge with a dot is 36 wide, so its dot sits 15 in from its left.
    EXPECT_EQ(mounted.canvas.at(304 - 36 + 15, BADGE_Y), ttk::Pill::tone_of(ttk::Pill::Kind::Danger).value);
}

TEST_F(Painted, CardTagsDarkenInTheLightPalette) {
    ttk::Theme::set_mode(ttk::Theme::Mode::Light);

    auto made = Mounted::prepared("Eviternity");

    made->tags = {tag("", ttk::Pill::Kind::Danger, true)};

    const Mounted mounted(std::move(made));

    EXPECT_EQ(mounted.canvas.at(304 - 36 + 15, BADGE_Y),
              ttk::Theme::darker(ttk::Pill::tone_of(ttk::Pill::Kind::Danger), 0.35).value);
}

TEST_F(Painted, CardHoverLightsIt) {
    Mounted mounted;

    widgets::hover(mounted.canvas, 150, 110);

    const std::size_t painted = mounted.canvas.frame();

    EXPECT_LE(painted, widgets::area(BOX));
    EXPECT_TRUE(widgets::near(mounted.plain(), widgets::over(widgets::palette().surface, widgets::palette().hover)))
            << std::hex << mounted.plain();
    EXPECT_EQ(mounted.edge(), widgets::palette().borderStrong.value);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, CardLeaveUnlightsIt) {
    Mounted mounted;
    const std::vector<std::uint32_t> rest = mounted.canvas.pixels();

    widgets::hover(mounted.canvas, 150, 110);
    mounted.canvas.frame();
    widgets::hover(mounted.canvas, 380, 220);

    const std::size_t painted = mounted.canvas.frame();

    EXPECT_LE(painted, widgets::area(BOX));
    EXPECT_EQ(mounted.canvas.pixels(), rest);
}

TEST_F(Painted, CardStaysLitWhileThePointerIsOnItsButtons) {
    Mounted mounted;

    mounted.card->buttons()->add(std::make_unique<ttk::Button>("Play", nullptr));
    mounted.canvas.root().relayout();
    mounted.canvas.frame();

    const BLRect button = mounted.card->buttons()->children().front()->box();

    widgets::hover(mounted.canvas, 150, 110);
    mounted.canvas.frame();
    widgets::hover(mounted.canvas, button.x + 10, button.y + (button.h / 2));
    mounted.canvas.frame_at(0.5);

    EXPECT_EQ(mounted.canvas.root().hovered(), mounted.card->buttons()->children().front().get());
    EXPECT_TRUE(widgets::near(mounted.plain(), widgets::over(widgets::palette().surface, widgets::palette().hover)))
            << "the card went dark when the pointer moved onto its button";
    EXPECT_TRUE(mounted.canvas.matches_full());

    widgets::hover(mounted.canvas, 380, 220);
    mounted.canvas.frame_at(1.0);

    EXPECT_EQ(mounted.plain(), widgets::palette().surface.value);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, CardRestingOnABadgeShowsItsHint) {
    auto made = Mounted::prepared("Eviternity");

    made->tags = {tag("Ready", ttk::Pill::Kind::Success, false, "Every file is in place")};

    Mounted mounted(std::move(made));

    widgets::hover(mounted.canvas, BADGE_X, BADGE_Y);

    EXPECT_EQ(mounted.card->hint, "Every file is in place");

    widgets::hover(mounted.canvas, 150, 110);

    EXPECT_TRUE(mounted.card->hint.empty());

    widgets::hover(mounted.canvas, BADGE_X, BADGE_Y);
    mounted.canvas.root().leave();

    EXPECT_TRUE(mounted.card->hint.empty()) << "leaving the card kept the badge hint";
}

TEST_F(Painted, CardTakesEveryPress) {
    Mounted mounted;

    mounted.press(150, 110);

    EXPECT_EQ(mounted.canvas.root().grabbed(), mounted.card);

    mounted.release(150, 110);

    EXPECT_EQ(mounted.canvas.root().grabbed(), nullptr);
}

TEST_F(Painted, CardClickAboveTheButtonsOpensIt) {
    Mounted mounted;

    mounted.press(150, 60);
    mounted.release(150, 60);

    EXPECT_EQ(mounted.opened, 1);
    EXPECT_EQ(mounted.started, 0);
    EXPECT_EQ(mounted.ended, 0);
}

TEST_F(Painted, CardClickOnTheButtonRowDoesNotOpenIt) {
    Mounted mounted;
    const double row = mounted.card->buttons()->box().y;

    mounted.press(250, row + 5);
    mounted.release(250, row + 5);

    EXPECT_EQ(mounted.opened, 0);
}

TEST_F(Painted, CardReleaseOutsideDoesNotOpenIt) {
    Mounted mounted;

    mounted.press(150, 60);
    mounted.canvas.root().motion(380, 220);
    mounted.release(380, 220);

    EXPECT_EQ(mounted.opened, 0);
}

TEST_F(Painted, CardWithoutCallbacksTakesClicksQuietly) {
    tests::Canvas canvas;
    auto made = std::make_unique<ttk::Card>();

    made->draggable = true;
    widgets::mount(canvas, std::move(made), BOX);
    canvas.root().press(ttk::Pointer{.x = 150, .y = 60});
    canvas.root().motion(190, 60);
    canvas.root().release(ttk::Pointer{.x = 190, .y = 60});
    widgets::click(canvas, 150, 60);

    EXPECT_EQ(canvas.root().grabbed(), nullptr);
}

TEST_F(Painted, CardDragStartsOnlyPastTheSlack) {
    Mounted mounted;

    mounted.card->draggable = true;
    mounted.press(150, 60);
    mounted.canvas.root().motion(155, 64);

    EXPECT_EQ(mounted.started, 0) << "a drag started inside the 6 pixel slack";
    EXPECT_EQ(mounted.moved, 0);
    EXPECT_FALSE(mounted.card->carrying());

    mounted.canvas.root().motion(156, 64);

    EXPECT_EQ(mounted.started, 1);
    EXPECT_DOUBLE_EQ(mounted.startX, 150);
    EXPECT_DOUBLE_EQ(mounted.startY, 60);
    EXPECT_EQ(mounted.moved, 1) << "the motion that starts a drag is reported too";
    EXPECT_DOUBLE_EQ(mounted.movedX, 156);
    EXPECT_DOUBLE_EQ(mounted.movedY, 64);
    EXPECT_TRUE(mounted.card->carrying());
}

TEST_F(Painted, CardDragStartsOnAVerticalMove) {
    Mounted mounted;

    mounted.card->draggable = true;
    mounted.press(150, 60);
    mounted.canvas.root().motion(150, 54);

    EXPECT_EQ(mounted.started, 1);
}

TEST_F(Painted, CardDragReportsEveryMotionAndStartsOnce) {
    Mounted mounted;

    mounted.card->draggable = true;
    mounted.press(150, 60);

    for (int step = 1; step <= 5; ++step) {
        mounted.canvas.root().motion(150 + (step * 10), 60);
    }

    EXPECT_EQ(mounted.started, 1);
    EXPECT_EQ(mounted.moved, 5);
    EXPECT_DOUBLE_EQ(mounted.movedX, 200);

    mounted.canvas.root().motion(500, 300);

    EXPECT_EQ(mounted.moved, 6) << "a drag stops reporting once it leaves the card";
    EXPECT_DOUBLE_EQ(mounted.movedX, 500);
}

TEST_F(Painted, CardDragShowsTheGrabbingCursor) {
    Mounted mounted;

    mounted.card->draggable = true;
    mounted.press(150, 60);

    EXPECT_EQ(mounted.canvas.root().cursor(), ttk::Cursor::Default);

    mounted.canvas.root().motion(170, 60);

    EXPECT_EQ(mounted.card->cursor_at(170, 60), ttk::Cursor::Grabbing);
    EXPECT_EQ(mounted.canvas.root().cursor(), ttk::Cursor::Grabbing);

    mounted.release(170, 60);

    EXPECT_EQ(mounted.card->cursor_at(170, 60), ttk::Cursor::Default);
}

TEST_F(Painted, CardDragReleaseEndsItWithoutOpening) {
    Mounted mounted;

    mounted.card->draggable = true;
    mounted.press(150, 60);
    mounted.canvas.root().motion(190, 60);
    mounted.release(190, 60);

    EXPECT_EQ(mounted.ended, 1);
    EXPECT_EQ(mounted.opened, 0) << "a drag released inside the card also opened it";
    EXPECT_FALSE(mounted.card->carrying());
}

TEST_F(Painted, CardNextPressAfterADragStartsFresh) {
    Mounted mounted;

    mounted.card->draggable = true;
    mounted.press(150, 60);
    mounted.canvas.root().motion(190, 60);
    mounted.release(190, 60);
    mounted.press(150, 60);
    mounted.release(150, 60);

    EXPECT_EQ(mounted.started, 1);
    EXPECT_EQ(mounted.ended, 1);
    EXPECT_EQ(mounted.opened, 1);
}

TEST_F(Painted, CardNotDraggableNeverDrags) {
    Mounted mounted;

    mounted.press(150, 60);
    mounted.canvas.root().motion(250, 120);

    EXPECT_EQ(mounted.started, 0);
    EXPECT_EQ(mounted.moved, 0);
    EXPECT_FALSE(mounted.card->carrying());

    mounted.release(150, 60);

    EXPECT_EQ(mounted.ended, 0);
    EXPECT_EQ(mounted.opened, 1);
}

TEST_F(Painted, CardSlideRepaintsOnlyTheCard) {
    Mounted mounted;

    mounted.card->slide_from(40, 0, 0.0);

    for (const double now : {0.03, 0.08, 0.15}) {
        const std::size_t painted = mounted.canvas.frame_at(now);

        EXPECT_GT(painted, 0U) << "at " << now;
        EXPECT_LE(painted, widgets::area(BOX)) << "at " << now;
    }

    mounted.canvas.frame_at(0.3);

    EXPECT_FALSE(mounted.card->sliding());
    EXPECT_FALSE(mounted.canvas.root().busy());
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, CardFieldsShowOnlyAfterInvalidate) {
    Mounted mounted;
    const std::vector<std::uint32_t> before = mounted.canvas.pixels();

    mounted.card->title = "Sunlust";

    EXPECT_FALSE(mounted.canvas.root().dirty()) << "a field change repainted by itself";

    mounted.card->invalidate();

    const std::size_t painted = mounted.canvas.frame();

    EXPECT_GT(painted, 0U);
    EXPECT_LE(painted, widgets::area(BOX));
    EXPECT_NE(mounted.canvas.pixels(), before);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, CardTitleIsElidedLeftOfTheBadges) {
    auto made = Mounted::prepared("A title so long that it would run straight under the badges at the right");

    made->tags = {tag("Ready", ttk::Pill::Kind::Success)};

    const Mounted mounted(std::move(made));

    EXPECT_EQ(mounted.canvas.at(BADGE_X, BADGE_Y), ttk::Pill::wash_of(ttk::Pill::Kind::Success).value);
    EXPECT_EQ(widgets::outside(mounted.canvas, PIXELS), 0U);
}

TEST_F(Painted, CardUtf8TitlePaints) {
    const Mounted mounted("Größe é й ✓");

    EXPECT_GT(widgets::ink(mounted.canvas, TITLE, widgets::palette().surface.value).pixels, 0U);
}

TEST_F(Painted, CardFollowsAPaletteChange) {
    Mounted mounted;

    ttk::Theme::set_mode(ttk::Theme::Mode::Light);
    mounted.canvas.frame();

    EXPECT_EQ(mounted.plain(), widgets::palette().surface.value);
    EXPECT_EQ(mounted.edge(), widgets::palette().border.value);
    EXPECT_TRUE(mounted.canvas.matches_full());
}
}
