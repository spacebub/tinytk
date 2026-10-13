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

#include <gtest/gtest.h>

#include "Blocks.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/layout/Box.h"
#include "ttk/toolkit/layout/Rule.h"

namespace {
    using tests::Block;
    using tests::boxed;

    ttk::Typeface &type() {
        return tests::fonts();
    }

    TEST(Rule, IsOnePixelTall) {
        ttk::Rule rule;

        EXPECT_EQ(rule.fixedHeight, 1.0);
        EXPECT_EQ(rule.wanted_height(type(), 10.0), 1.0);
        EXPECT_EQ(rule.wanted_height(type(), 1000.0), 1.0);
        EXPECT_EQ(rule.wanted_width(type()), 0.0);
    }

    TEST(Rule, DividesAColumn) {
        auto column = ttk::Box::column();

        column->spacing(4.0);
        column->append(std::make_unique<Block>(10.0, 10.0));

        const auto *rule = column->append(std::make_unique<ttk::Rule>());
        Block *after = column->append(std::make_unique<Block>(10.0, 10.0));

        column->place({0, 0, 120, 200}, type());

        EXPECT_TRUE(boxed(rule->box(), {0, 14, 120, 1}));
        EXPECT_EQ(after->box().y, 19.0);
        EXPECT_EQ(column->natural_height(type(), 120.0), 29.0);
    }

    TEST(Rule, PaintsTheTopRowOfItsBoxInTheBorderColour) {
        tests::Canvas canvas(160, 60);
        auto *rule = canvas.root().layer(ttk::Root::POPUPS)->append(std::make_unique<ttk::Rule>());

        rule->place({10, 20, 100, 5}, type());
        canvas.frame();

        const std::uint32_t border = ttk::Theme::palette().border.value;
        const std::uint32_t background = ttk::Theme::palette().background.value;

        for (int x = 10; x < 110; ++x) {
            EXPECT_EQ(canvas.at(x, 20), border) << "at " << x;
        }

        for (int y = 21; y < 25; ++y) {
            EXPECT_EQ(canvas.at(50, y), background) << "the rule painted below its top row at " << y;
        }

        EXPECT_EQ(canvas.at(9, 20), background);
        EXPECT_EQ(canvas.at(110, 20), background);
        EXPECT_EQ(canvas.at(50, 19), background);
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST(Rule, FollowsTheTheme) {
        tests::Canvas canvas(60, 20);

        canvas.root().content()->add(std::make_unique<ttk::Rule>());
        canvas.frame();

        const ttk::Theme::Mode was = ttk::Theme::mode();
        const ttk::Theme::Mode other = ttk::Theme::palette().dark ? ttk::Theme::Mode::Light : ttk::Theme::Mode::Dark;

        ttk::Theme::set_mode(other);
        canvas.frame();

        EXPECT_EQ(canvas.at(30, 0), ttk::Theme::palette().border.value) << "a theme change did not repaint the rule";

        ttk::Theme::set_mode(was);
    }
}
