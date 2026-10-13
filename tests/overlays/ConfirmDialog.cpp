// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Overlays.h"
#include "support/Canvas.h"
#include "ttk/dialogs/ConfirmDialog.h"
#include "ttk/dialogs/DialogLayer.h"
#include "ttk/toolkit/controls/Button.h"
#include "ttk/toolkit/controls/Label.h"

namespace {
class Counted final : public ttk::ConfirmDialog {
public:
    Counted(const bool danger, std::function<void()> accepted, int *destroyed)
        : ConfirmDialog("Delete it?", "The file goes for good.", "Delete", danger, std::move(accepted)),
          _destroyed(destroyed) {}

    ~Counted() override { ++*_destroyed; }

    Counted(const Counted &) = delete;
    Counted &operator=(const Counted &) = delete;
    Counted(Counted &&) = delete;
    Counted &operator=(Counted &&) = delete;

private:
    int *_destroyed;
};

class Confirm : public overlays::Painted {
protected:
    tests::Canvas canvas{640, 400};
    ttk::DialogLayer *layer = nullptr;
    ttk::Dialog *dialog = nullptr;
    ttk::Button *cancel = nullptr;
    ttk::Button *accept = nullptr;
    std::vector<std::string> log;
    int destroyed = 0;

    void SetUp() override {
        overlays::Painted::SetUp();

        if (IsSkipped()) {
            return;
        }

        layer = overlays::dialog_layer(canvas);
    }

    void open(const bool danger = false, const bool answered = true) {
        std::function<void()> accepted;

        if (answered) {
            accepted = [this] { log.emplace_back(layer->covered() ? "accepted while up" : "accepted"); };
        }

        dialog = layer->show(std::make_unique<Counted>(danger, std::move(accepted), &destroyed));
        canvas.frame();
        canvas.frame_at(canvas.root().now() + 1.0);

        const std::vector<ttk::Button *> buttons = overlays::all<ttk::Button>(dialog);

        ASSERT_EQ(buttons.size(), 2U) << "a confirm dialog has Cancel and an accept button";

        cancel = buttons[0];
        accept = buttons[1];
    }

    void record_dismissal() {
        dialog->dismissed = [this] { log.emplace_back("dismissed"); };
    }
};

TEST_F(Confirm, ShowsTheTitleAndTheMessage) {
    ASSERT_NO_FATAL_FAILURE(open());

    std::vector<std::string> texts;

    for (const ttk::Label *label : overlays::all<ttk::Label>(dialog)) {
        texts.push_back(label->text());
    }

    EXPECT_EQ(texts, (std::vector<std::string>{"Delete it?", "The file goes for good."}));
}

TEST_F(Confirm, CancelSitsBeforeTheAcceptButtonOnTheRight) {
    ASSERT_NO_FATAL_FAILURE(open());

    const BLRect card = dialog->card()->box();

    EXPECT_LT(cancel->box().x, accept->box().x);
    EXPECT_NEAR(accept->box().x + accept->box().w, card.x + card.w - 22.0, 1.0)
            << "the buttons are not packed against the padded right edge";
}

TEST_F(Confirm, CancelCallsDismissedAlone) {
    ASSERT_NO_FATAL_FAILURE(open());
    record_dismissal();
    overlays::click(canvas.root(), cancel);

    EXPECT_EQ(log, (std::vector<std::string>{"dismissed"}));
}

TEST_F(Confirm, AcceptCallsDismissedThenAccepted) {
    ASSERT_NO_FATAL_FAILURE(open());
    record_dismissal();
    overlays::click(canvas.root(), accept);

    EXPECT_EQ(log, (std::vector<std::string>{"dismissed", "accepted while up"}));
}

TEST_F(Confirm, AcceptedRunsAfterTheStackTookTheDialogDown) {
    ASSERT_NO_FATAL_FAILURE(open());
    overlays::click(canvas.root(), accept);

    EXPECT_EQ(log, (std::vector<std::string>{"accepted"}));
    EXPECT_EQ(destroyed, 1);
}

TEST_F(Confirm, CancelThroughTheStackTakesTheDialogDownUnanswered) {
    ASSERT_NO_FATAL_FAILURE(open());
    overlays::click(canvas.root(), cancel);

    EXPECT_TRUE(log.empty());
    EXPECT_EQ(destroyed, 1);
    EXPECT_FALSE(layer->covered());
}

TEST_F(Confirm, ScrimClickCancels) {
    ASSERT_NO_FATAL_FAILURE(open());
    overlays::click(canvas.root(), 4.0, canvas.height() - 4.0);

    EXPECT_TRUE(log.empty());
    EXPECT_FALSE(layer->covered());
}

TEST_F(Confirm, AcceptWithoutACallbackOnlyCloses) {
    ASSERT_NO_FATAL_FAILURE(open(false, false));
    overlays::click(canvas.root(), accept);

    EXPECT_FALSE(layer->covered());
}

TEST_F(Confirm, NothingIsFocusedOnOpen) {
    ASSERT_NO_FATAL_FAILURE(open());

    EXPECT_EQ(canvas.root().focused(), nullptr);
}

TEST_F(Confirm, TabCyclesBetweenTheTwoButtons) {
    ASSERT_NO_FATAL_FAILURE(open());

    std::vector<const ttk::Widget *> seen;

    for (int step = 0; step < 3; ++step) {
        canvas.root().key(ttk::Key{.code = ttk::Code::Tab});
        seen.push_back(canvas.root().focused());
    }

    EXPECT_EQ(seen, (std::vector<const ttk::Widget *>{cancel, accept, cancel}));
}

TEST_F(Confirm, ReturnOnTheFocusedAcceptButtonAccepts) {
    ASSERT_NO_FATAL_FAILURE(open());
    canvas.root().focus(accept);

    EXPECT_TRUE(canvas.root().key(ttk::Key{.code = ttk::Code::Return}));
    EXPECT_EQ(log, (std::vector<std::string>{"accepted"}));
}

TEST_F(Confirm, SpaceOnTheFocusedCancelButtonCancels) {
    ASSERT_NO_FATAL_FAILURE(open());
    canvas.root().focus(cancel);

    EXPECT_TRUE(canvas.root().key(ttk::Key{.code = ttk::Code::Space}));
    EXPECT_TRUE(log.empty());
    EXPECT_FALSE(layer->covered());
}

TEST_F(Confirm, EscapeIsNotConsumed) {
    ASSERT_NO_FATAL_FAILURE(open());
    canvas.root().focus(accept);

    EXPECT_FALSE(canvas.root().key(ttk::Key{.code = ttk::Code::Escape}));
    EXPECT_TRUE(layer->covered());
}

TEST_F(Confirm, DangerAcceptLooksUnlikePrimary) {
    ASSERT_NO_FATAL_FAILURE(open(false));

    const BLRect box = accept->box();
    const auto sample = [&] {
        std::vector<std::uint32_t> out;

        for (int y = static_cast<int>(box.y) + 2; y < static_cast<int>(box.y + box.h) - 2; y += 3) {
            for (int x = static_cast<int>(box.x) + 2; x < static_cast<int>(box.x + box.w) - 2; x += 3) {
                out.push_back(canvas.at(x, y));
            }
        }

        return out;
    };
    const std::vector<std::uint32_t> primary = sample();

    layer->dismiss();
    ASSERT_NO_FATAL_FAILURE(open(true));

    ASSERT_DOUBLE_EQ(accept->box().x, box.x);

    EXPECT_NE(sample(), primary) << "danger made no difference to the accept button";
}

TEST_F(Confirm, PaintMatchesAFullRepaintWhileGrowingAndHovered) {
    layer->show(std::make_unique<Counted>(true, nullptr, &destroyed));

    for (const double at : {0.0, 0.05, 0.1, 0.5}) {
        canvas.frame_at(at);

        EXPECT_TRUE(canvas.matches_full()) << "partial/full mismatch at " << at;
    }

    const ttk::Button *button = overlays::all<ttk::Button>(layer->top()).back();

    canvas.root().motion(overlays::mid_x(button->box()), overlays::mid_y(button->box()));
    canvas.frame_at(0.6);
    canvas.frame_at(1.0);

    EXPECT_TRUE(canvas.matches_full()) << "hovered accept button partial/full mismatch";
}
}
