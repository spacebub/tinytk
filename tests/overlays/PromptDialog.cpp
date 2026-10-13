// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Overlays.h"
#include "support/Canvas.h"
#include "ttk/dialogs/DialogLayer.h"
#include "ttk/dialogs/PromptDialog.h"
#include "ttk/toolkit/controls/Button.h"
#include "ttk/toolkit/controls/Field.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/controls/TextBox.h"

namespace {
    class Prompt : public overlays::Painted {
    protected:
        tests::Canvas canvas{640, 400};
        ttk::DialogLayer *layer = nullptr;
        ttk::Dialog *dialog = nullptr;
        ttk::Field *field = nullptr;
        ttk::Button *cancel = nullptr;
        ttk::Button *accept = nullptr;
        std::vector<std::string> log;

        void SetUp() override {
            overlays::Painted::SetUp();

            if (IsSkipped()) {
                return;
            }

            layer = overlays::dialog_layer(canvas);
        }

        void open(const std::string &value) {
            dialog = layer->show(std::make_unique<ttk::PromptDialog>(
                    "Rename", "New name", value, "Rename",
                    [this](const std::string &typed) { log.push_back((layer->covered() ? "up:" : "") + typed); }));
            canvas.frame();
            canvas.frame_at(canvas.root().now() + 1.0);

            const std::vector<ttk::Field *> fields = overlays::all<ttk::Field>(dialog);
            const std::vector<ttk::Button *> buttons = overlays::all<ttk::Button>(dialog);

            ASSERT_EQ(fields.size(), 1U);
            ASSERT_EQ(buttons.size(), 2U) << "a prompt has Cancel and an accept button";

            field = fields[0];
            cancel = buttons[0];
            accept = buttons[1];
        }

        void record_dismissal() {
            dialog->dismissed = [this] { log.emplace_back("dismissed"); };
        }

        bool key(const int code) { return canvas.root().key(ttk::Key{.code = code}); }
    };

    TEST_F(Prompt, ShowsTheTitleAndTheCaption) {
        ASSERT_NO_FATAL_FAILURE(open("draft"));

        std::vector<std::string> texts;

        for (const ttk::Label *label : overlays::all<ttk::Label>(dialog)) {
            texts.push_back(label->text());
        }

        EXPECT_NE(std::ranges::find(texts, "Rename"), texts.end());
        EXPECT_NE(std::ranges::find(texts, "New name"), texts.end());
    }

    TEST_F(Prompt, FieldHoldsTheValueGiven) {
        ASSERT_NO_FATAL_FAILURE(open("draft"));

        EXPECT_EQ(field->text(), "draft");
    }

    TEST_F(Prompt, OpenedGivesTheFieldFocus) {
        ASSERT_NO_FATAL_FAILURE(open("draft"));

        EXPECT_EQ(canvas.root().focused(), field->input());
    }

    TEST_F(Prompt, TypingReplacesTheSelectedValue) {
        ASSERT_NO_FATAL_FAILURE(open("draft"));

        canvas.root().wrote("final");

        EXPECT_EQ(field->text(), "final") << "the value was not selected when the dialog opened";
    }

    TEST_F(Prompt, AcceptIsEnabledForAValue) {
        ASSERT_NO_FATAL_FAILURE(open("draft"));

        EXPECT_TRUE(accept->enabled());
        EXPECT_TRUE(cancel->enabled());
    }

    TEST_F(Prompt, AcceptIsDisabledForAnEmptyValue) {
        ASSERT_NO_FATAL_FAILURE(open(""));

        EXPECT_FALSE(accept->enabled());
    }

    TEST_F(Prompt, AcceptIsDisabledForWhitespace) {
        ASSERT_NO_FATAL_FAILURE(open(" \t "));

        EXPECT_FALSE(accept->enabled());
    }

    TEST_F(Prompt, TypingTurnsAcceptOnAndErasingTurnsItOff) {
        ASSERT_NO_FATAL_FAILURE(open(""));

        canvas.root().wrote("a");

        EXPECT_TRUE(accept->enabled());

        key(ttk::Code::Backspace);

        EXPECT_FALSE(accept->enabled());

        canvas.root().wrote("  ");

        EXPECT_FALSE(accept->enabled()) << "whitespace alone enabled the accept button";
    }

    TEST_F(Prompt, ReturnInTheFieldDismissesThenAcceptsTheTrimmedText) {
        ASSERT_NO_FATAL_FAILURE(open("  report  "));
        record_dismissal();

        EXPECT_TRUE(key(ttk::Code::Return));
        EXPECT_EQ(log, (std::vector<std::string>{"dismissed", "up:report"}));
    }

    TEST_F(Prompt, ReturnWithOnlyWhitespaceDoesNothing) {
        ASSERT_NO_FATAL_FAILURE(open("   "));
        record_dismissal();
        key(ttk::Code::Return);

        EXPECT_TRUE(log.empty());
    }

    TEST_F(Prompt, TypedTextIsWhatIsAccepted) {
        ASSERT_NO_FATAL_FAILURE(open("old"));

        canvas.root().wrote(" new name ");
        key(ttk::Code::Return);

        EXPECT_EQ(log, (std::vector<std::string>{"new name"}));
    }

    TEST_F(Prompt, AcceptButtonAcceptsAfterTheStackTookTheDialogDown) {
        ASSERT_NO_FATAL_FAILURE(open("draft"));

        overlays::click(canvas.root(), accept);

        EXPECT_EQ(log, (std::vector<std::string>{"draft"}));
        EXPECT_FALSE(layer->covered());
    }

    TEST_F(Prompt, DisabledAcceptButtonIgnoresAClick) {
        ASSERT_NO_FATAL_FAILURE(open(""));

        overlays::click(canvas.root(), accept);

        EXPECT_TRUE(log.empty());
        EXPECT_TRUE(layer->covered());
    }

    TEST_F(Prompt, CancelDismissesWithoutAnAnswer) {
        ASSERT_NO_FATAL_FAILURE(open("draft"));

        overlays::click(canvas.root(), cancel);

        EXPECT_TRUE(log.empty());
        EXPECT_FALSE(layer->covered());
    }

    TEST_F(Prompt, CancelCallsDismissed) {
        ASSERT_NO_FATAL_FAILURE(open("draft"));
        record_dismissal();
        overlays::click(canvas.root(), cancel);

        EXPECT_EQ(log, (std::vector<std::string>{"dismissed"}));
    }

    TEST_F(Prompt, EscapeInTheFieldIsLeftToTheApplication) {
        ASSERT_NO_FATAL_FAILURE(open("draft"));

        EXPECT_FALSE(key(ttk::Code::Escape));
        EXPECT_TRUE(layer->covered());
        EXPECT_TRUE(log.empty());
    }

    TEST_F(Prompt, TabMovesFromTheFieldToTheButtonsAndBack) {
        ASSERT_NO_FATAL_FAILURE(open("draft"));

        std::vector<const ttk::Widget *> seen;

        for (int step = 0; step < 3; ++step) {
            key(ttk::Code::Tab);
            seen.push_back(canvas.root().focused());
        }

        EXPECT_EQ(seen, (std::vector<const ttk::Widget *>{cancel, accept, field->input()}));
    }

    TEST_F(Prompt, UncoveredPromptTakesFocusBack) {
        ASSERT_NO_FATAL_FAILURE(open("first"));

        const ttk::Field *under = field;

        ASSERT_NO_FATAL_FAILURE(open("second"));

        EXPECT_EQ(canvas.root().focused(), field->input());

        layer->dismiss();

        EXPECT_EQ(canvas.root().focused(), under->input()) << "the prompt uncovered did not take focus again";
    }

    TEST_F(Prompt, PaintMatchesAFullRepaintWhileTyping) {
        ASSERT_NO_FATAL_FAILURE(open("draft"));

        canvas.root().wrote("typed");
        canvas.frame_at(1.2);

        EXPECT_TRUE(canvas.matches_full());
    }
}
