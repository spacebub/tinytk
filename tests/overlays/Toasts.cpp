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
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Overlays.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/notices/Notice.h"
#include "ttk/notices/Notifier.h"
#include "ttk/notices/Toasts.h"

namespace {
using overlays::Taker;

constexpr double WIDTH = 392.0;
constexpr double INSET = 18.0;
constexpr double APART = 10.0;
constexpr double FADE = 0.16;
constexpr double START = 10.0;

double lasting(const std::string &text) {
    return (3200.0 + (18.0 * static_cast<double>(text.size()))) / 1000.0;
}

class Stacked : public overlays::Painted {
protected:
    tests::Canvas canvas{640, 400};
    ttk::Notifier notifier;
    ttk::Toasts *toasts = nullptr;
    std::vector<int> gone;
    std::vector<std::uint32_t> blank;

    void SetUp() override {
        overlays::Painted::SetUp();

        if (IsSkipped()) {
            return;
        }

        canvas.frame();
        toasts = canvas.root().layer(ttk::Root::NOTICES)->append(std::make_unique<ttk::Toasts>([this](const int id) {
            gone.push_back(id);
        }));
        notifier.changed = [this] { toasts->set_messages(notifier.messages()); };
        canvas.root().relayout();
        canvas.frame();
        blank = canvas.pixels();
        canvas.root().set_now(START);
    }

    void at(const double now) { canvas.frame_at(now); }

    // Deferred past the frame, since the toast that called back is still running.
    void settle_gone() {
        for (const int id : gone) {
            notifier.dismiss(id);
        }

        gone.clear();
        canvas.frame();
    }

    [[nodiscard]] ttk::Toast *toast(const std::size_t index) const {
        return dynamic_cast<ttk::Toast *>(toasts->children().at(index).get());
    }

    [[nodiscard]] std::vector<int> shown() const {
        std::vector<int> out;

        for (const ttk::Widget::Ptr &child : toasts->children()) {
            out.push_back(dynamic_cast<const ttk::Toast *>(child.get())->id());
        }

        return out;
    }

    ttk::Toast *posted(const std::string &text = "Saved",
                       const ttk::Severity severity = ttk::Severity::Info,
                       const std::string &title = {}) {
        const double now = canvas.root().now();

        notifier.post(severity, text, title);
        canvas.frame();
        at(now);
        at(now + FADE + 0.02);

        return toast(toasts->children().size() - 1);
    }

    [[nodiscard]] std::uint32_t pixel(const double x, const double y) const {
        return canvas.at(static_cast<int>(x), static_cast<int>(y));
    }
};

TEST_F(Stacked, StartsEmpty) {
    EXPECT_TRUE(toasts->children().empty());
}

TEST_F(Stacked, AddsAToastForEachNoticeInOrder) {
    notifier.info("one");
    notifier.warning("two");

    EXPECT_EQ(shown(), (std::vector<int>{0, 1}));
}

TEST_F(Stacked, FirstToastSitsTopRightUnderTheTitleBar) {
    ttk::Toast *first = posted();
    const BLRect box = first->box();

    EXPECT_DOUBLE_EQ(box.x, canvas.width() - INSET - WIDTH);
    EXPECT_DOUBLE_EQ(box.y, ttk::Theme::barHeight + INSET);
    EXPECT_DOUBLE_EQ(box.w, WIDTH);
    EXPECT_DOUBLE_EQ(box.h, first->natural_height(canvas.root().type(), WIDTH));
}

TEST_F(Stacked, ToastsStackDownwardsNewestAtTheBottom) {
    const ttk::Toast *first = posted("First");
    const ttk::Toast *second = posted("Second, with a title", ttk::Severity::Success, "Done");

    EXPECT_DOUBLE_EQ(second->box().x, first->box().x);
    EXPECT_DOUBLE_EQ(second->box().y, first->box().y + first->box().h + APART);
}

TEST_F(Stacked, MissingIdsAreRemovedAndTheRestMoveUp) {
    const double top = posted("One")->box().y;

    posted("Two");

    const ttk::Toast *third = posted("Three");

    notifier.dismiss(0);
    notifier.dismiss(1);
    canvas.frame();

    EXPECT_EQ(shown(), (std::vector<int>{2}));
    EXPECT_DOUBLE_EQ(third->box().y, top);
}

TEST_F(Stacked, ToastsAlreadyShownAreKept) {
    const ttk::Toast *first = posted("One");

    posted("Two");

    EXPECT_EQ(toast(0), first) << "an existing toast was rebuilt, restarting its countdown";
}

TEST_F(Stacked, AnUnchangedListAsksForNoLayout) {
    posted();
    canvas.frame();
    toasts->set_messages(notifier.messages());

    EXPECT_FALSE(canvas.root().settle());
}

TEST_F(Stacked, AChangedListAsksForALayout) {
    posted();
    canvas.frame();
    notifier.info("More");

    EXPECT_TRUE(canvas.root().settle());
}

TEST_F(Stacked, AFifthNoticeDropsTheOldestToast) {
    for (int at = 0; at < 5; ++at) {
        notifier.info(std::to_string(at));
    }

    EXPECT_EQ(shown(), (std::vector<int>{1, 2, 3, 4}));
}

TEST_F(Stacked, ToastFadesInFromNothing) {
    notifier.info("Fading");
    canvas.frame();

    EXPECT_EQ(canvas.pixels(), blank) << "the toast painted before it began to animate";

    at(START);
    at(START + (FADE / 3.0));

    const std::vector<std::uint32_t> part = canvas.pixels();

    at(START + FADE + 0.02);

    EXPECT_NE(part, blank);
    EXPECT_NE(part, canvas.pixels()) << "the toast did not fade in";
}

TEST_F(Stacked, InfoClosesItselfWhenItsCountdownEnds) {
    const std::string text = "Saved";

    posted(text);
    at(START + lasting(text) - 0.1);

    EXPECT_TRUE(gone.empty()) << "the toast closed early";

    at(START + lasting(text) + 0.05);
    at(START + lasting(text) + 0.05 + FADE + 0.02);

    EXPECT_EQ(gone, (std::vector<int>{0}));
}

TEST_F(Stacked, CountdownStartingAtTimeZeroCountsItsFirstInterval) {
    const std::string text = "Saved";

    canvas.root().set_now(0.0);
    notifier.info(text);
    canvas.frame();
    at(0.0);
    at(0.5);
    at(lasting(text) + 0.05);
    at(lasting(text) + 0.05 + FADE + 0.02);

    EXPECT_EQ(gone, (std::vector<int>{0})) << "the first interval of the countdown was lost";
}

TEST_F(Stacked, ClosedToastPaintsNothingUntilTheListDropsIt) {
    const BLRect box = posted("Saved")->box();

    toast(0)->close();
    at(START + 1.0);
    at(START + 1.0 + FADE + 0.02);

    ASSERT_EQ(gone, (std::vector<int>{0}));
    EXPECT_EQ(shown(), (std::vector<int>{0}));
    EXPECT_EQ(canvas.pixels(), blank) << "a faded toast is still drawn";

    settle_gone();

    EXPECT_TRUE(toasts->children().empty());
    EXPECT_EQ(pixel(box.x + 5.0, box.y + 5.0), blank.at(0));
}

TEST_F(Stacked, WarningAndSuccessCountDownToo) {
    posted("w", ttk::Severity::Warning);
    posted("s", ttk::Severity::Success);
    at(START + 4.0);
    at(START + 4.0 + FADE + 0.02);
    std::ranges::sort(gone);

    EXPECT_EQ(gone, (std::vector<int>{0, 1}));
}

TEST_F(Stacked, ErrorStaysUntilDismissed) {
    posted("Broken", ttk::Severity::Error);
    at(START + 1000.0);
    at(START + 2000.0);

    EXPECT_TRUE(gone.empty());
    EXPECT_EQ(shown(), (std::vector<int>{0}));
}

TEST_F(Stacked, ErrorSleepsUntilSomethingWakesIt) {
    posted("Broken", ttk::Severity::Error);
    at(START + 1.0);

    EXPECT_FALSE(canvas.root().busy());
    EXPECT_LT(canvas.root().waking(), 0.0) << "an error toast scheduled a wake for nothing";
}

TEST_F(Stacked, CountdownSleepsUntilTheBarLosesAPixel) {
    posted("Saved");
    at(START + 1.0);

    EXPECT_FALSE(canvas.root().busy()) << "the countdown ran frames while only the bar moves";
    EXPECT_GT(canvas.root().waking(), START + 1.0);
    EXPECT_LT(canvas.root().waking(), START + 1.1);
}

TEST_F(Stacked, HoverHoldsTheCountdownAndLeavingResumesIt) {
    const std::string text = "Saved";
    const BLRect box = posted(text)->box();

    at(START + 1.0);
    canvas.root().motion(box.x + 40.0, box.y + 10.0);
    at(START + 1.1);
    at(START + 100.0);

    EXPECT_TRUE(gone.empty()) << "the toast counted down under the pointer";

    canvas.root().set_now(START + 100.0);
    canvas.root().motion(5.0, 390.0);
    at(START + 100.0);
    at(START + 100.0 + lasting(text) - 1.2);

    EXPECT_TRUE(gone.empty()) << "the countdown did not resume from where it was held";

    at(START + 100.0 + lasting(text) - 0.95);
    at(START + 100.0 + lasting(text) + FADE);

    EXPECT_EQ(gone, (std::vector<int>{0}));
}

TEST_F(Stacked, CloseButtonClosesTheToast) {
    const BLRect box = posted("Broken", ttk::Severity::Error)->box();

    canvas.root().set_now(START + 1.0);
    overlays::click(canvas.root(), box.x + box.w - 32.0 + 12.0, box.y + 9.0 + 12.0);
    at(START + 1.0);
    at(START + 1.0 + FADE + 0.02);

    EXPECT_EQ(gone, (std::vector<int>{0}));
}

TEST_F(Stacked, ClosingTwiceCallsBackOnce) {
    ttk::Toast *one = posted("Saved");

    one->close();
    one->close();
    at(START + 0.5);
    at(START + 1.0);
    at(START + 10.0);

    EXPECT_EQ(gone, (std::vector<int>{0}));
}

TEST_F(Stacked, PressesOnAToastDoNotReachThePage) {
    auto *page = static_cast<Taker *>(canvas.root().content()->add(std::make_unique<Taker>()));

    canvas.root().relayout();

    const BLRect box = posted()->box();

    overlays::click(canvas.root(), box.x + 40.0, box.y + 10.0);

    EXPECT_EQ(page->presses, 0);
    EXPECT_TRUE(gone.empty()) << "a click on the body closed the toast";

    overlays::click(canvas.root(), 5.0, 390.0);

    EXPECT_EQ(page->presses, 1);
}

TEST_F(Stacked, ToastDoesNotTakeFocus) {
    EXPECT_FALSE(posted()->takes_focus());
}

TEST_F(Stacked, ToastKeepsItsNoticeId) {
    posted("a");
    posted("b");

    EXPECT_EQ(toast(1)->id(), 1);
}

TEST_F(Stacked, NaturalHeightIgnoresTheWidthAsked) {
    ttk::Toast *one = posted("A body long enough to wrap onto a second line at the toast's own width");
    ttk::Typeface &type = canvas.root().type();

    EXPECT_DOUBLE_EQ(one->natural_height(type, 10.0), one->natural_height(type, 2000.0));
}

TEST_F(Stacked, TitleAndLongBodiesMakeTallerToasts) {
    ttk::Typeface &type = canvas.root().type();
    const double plain = posted("Body")->natural_height(type, WIDTH);
    const double titled = posted("Body", ttk::Severity::Info, "Title")->natural_height(type, WIDTH);
    const double wrapped =
            posted("A body long enough to wrap onto a second line, and then onto a third line, at the width "
                   "of a toast")
                    ->natural_height(type, WIDTH);

    EXPECT_GT(titled, plain);
    EXPECT_GT(wrapped, plain);
}

TEST_F(Stacked, EachSeverityHasItsOwnColour) {
    std::set<std::uint32_t> washes;

    for (const ttk::Severity severity :
         {ttk::Severity::Info, ttk::Severity::Success, ttk::Severity::Warning, ttk::Severity::Error}) {
        const BLRect box = posted("Same", severity)->box();

        washes.insert(pixel(box.x + 8.0, box.y + (box.h / 2.0)));
    }

    EXPECT_EQ(washes.size(), 4U);
}

TEST_F(Stacked, CountdownBarRunsAlongTheBottomEdge) {
    const BLRect box = posted("Saved")->box();
    const double x = box.x + (box.w / 2.0);

    EXPECT_NE(pixel(x, box.y + box.h - 2.0), pixel(x, box.y + box.h - 8.0));
}

TEST_F(Stacked, ErrorHasNoCountdownBar) {
    const BLRect box = posted("Broken", ttk::Severity::Error)->box();
    const double x = box.x + (box.w / 2.0);

    EXPECT_EQ(pixel(x, box.y + box.h - 2.0), pixel(x, box.y + box.h - 8.0));
}

TEST_F(Stacked, BarShrinksAsTimeRunsOut) {
    const std::string text = "Saved";
    const BLRect box = posted(text)->box();
    const double row = box.y + box.h - 2.0;
    const std::uint32_t ink = pixel(box.x + (box.w * 0.75), row);

    at(START + (lasting(text) / 2.0));

    EXPECT_NE(pixel(box.x + (box.w * 0.75), row), ink) << "the bar did not shrink";
    EXPECT_EQ(pixel(box.x + (box.w * 0.25), row), ink);
}

TEST_F(Stacked, LaterToastsKeepTheirOwnDeadlines) {
    const std::string text = "Saved";

    posted(text);
    at(START + 2.0);
    posted(text);
    at(START + lasting(text) + 0.05);
    at(START + lasting(text) + 0.05 + FADE + 0.02);

    EXPECT_EQ(gone, (std::vector<int>{0})) << "a new toast restarted or shortened another's countdown";

    at(START + 2.0 + lasting(text) + 0.3);
    at(START + 2.0 + lasting(text) + 0.3 + FADE + 0.02);

    EXPECT_EQ(gone, (std::vector<int>{0, 1}));
}

TEST_F(Stacked, PaintMatchesAFullRepaintThroughALife) {
    const std::string text = "Saved";

    notifier.post(ttk::Severity::Success, text, "Done");

    for (const double now : {
                 START,
                 START + 0.05,
                 START + 0.1,
                 START + 1.0,
                 START + 2.5,
                 START + lasting(text),
                 START + lasting(text) + 0.08,
                 START + lasting(text) + 0.3,
         }) {
        at(now);

        ASSERT_TRUE(canvas.matches_full()) << "partial/full mismatch at " << now;
    }

    settle_gone();

    EXPECT_TRUE(canvas.matches_full()) << "partial/full mismatch once the toast was dropped";
}
}
