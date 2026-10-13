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
#include "ttk/toolkit/layout/Panel.h"

namespace {
    using tests::Block;
    using tests::boxed;

    ttk::Typeface &type() {
        return tests::fonts();
    }

    const ttk::Theme::Palette &palette() {
        return ttk::Theme::palette();
    }

    // A panel on the popup layer, which keeps the box it is placed at.
    ttk::Panel *mount(tests::Canvas &canvas, const BLRect &box) {
        auto *panel = canvas.root().layer(ttk::Root::POPUPS)->append(std::make_unique<ttk::Panel>());

        panel->place(box, type());

        return panel;
    }

    TEST(Panel, Defaults) {
        const ttk::Panel panel;

        EXPECT_FALSE(panel.inset);
        EXPECT_FALSE(panel.hoverable);
        EXPECT_FALSE(panel.lit);
        EXPECT_EQ(panel.rounding, 12.0);
        EXPECT_TRUE(panel.bordered);
        EXPECT_TRUE(panel.fills);
    }

    TEST(Panel, FillingPanelGivesEveryChildItsBox) {
        ttk::Panel panel;
        Block *one = panel.append(std::make_unique<Block>(10.0, 10.0));
        Block *two = panel.append(std::make_unique<Block>(20.0, 20.0));

        panel.place({5, 6, 70, 80}, type());

        EXPECT_TRUE(boxed(one->box(), {5, 6, 70, 80}));
        EXPECT_TRUE(boxed(two->box(), {5, 6, 70, 80}));
    }

    TEST(Panel, NonFillingPanelLeavesItsChildrenWhereTheyAre) {
        ttk::Panel panel;
        Block *child = panel.append(std::make_unique<Block>(10.0, 10.0));

        panel.fills = false;
        child->place({1, 2, 3, 4}, type());
        panel.place({5, 6, 70, 80}, type());

        EXPECT_TRUE(boxed(child->box(), {1, 2, 3, 4}));
    }

    TEST(Panel, SizesFromItsChildren) {
        ttk::Panel panel;

        panel.append(std::make_unique<Block>(30.0, 10.0));
        panel.append(std::make_unique<Block>(20.0, 40.0));

        EXPECT_EQ(panel.natural_width(type()), 30.0);
        EXPECT_EQ(panel.natural_height(type(), 100.0), 40.0);
    }

    TEST(Panel, FillsWithTheSurfaceColour) {
        tests::Canvas canvas(200, 120);

        mount(canvas, {20, 20, 100, 60});
        canvas.frame();

        EXPECT_EQ(canvas.at(70, 50), palette().surface.value);
        EXPECT_EQ(canvas.at(20, 50), palette().border.value) << "no border on the left edge";
        EXPECT_EQ(canvas.at(119, 50), palette().border.value) << "no border on the right edge";
        EXPECT_EQ(canvas.at(70, 20), palette().border.value) << "no border on the top edge";
        EXPECT_EQ(canvas.at(70, 79), palette().border.value) << "no border on the bottom edge";
        EXPECT_EQ(canvas.at(21, 50), palette().surface.value) << "the border is wider than a pixel";
    }

    TEST(Panel, PaintsNothingOutsideItsBox) {
        tests::Canvas canvas(200, 120);

        mount(canvas, {20, 20, 100, 60});
        canvas.frame();

        const std::uint32_t background = palette().background.value;

        for (int x = 0; x < 200; ++x) {
            ASSERT_EQ(canvas.at(x, 19), background) << "at " << x;
            ASSERT_EQ(canvas.at(x, 80), background) << "at " << x;
        }

        for (int y = 0; y < 120; ++y) {
            ASSERT_EQ(canvas.at(19, y), background) << "at " << y;
            ASSERT_EQ(canvas.at(120, y), background) << "at " << y;
        }
    }

    TEST(Panel, RoundsItsCorners) {
        tests::Canvas canvas(200, 120);
        ttk::Panel *panel = mount(canvas, {20, 20, 100, 60});

        canvas.frame();

        EXPECT_EQ(canvas.at(20, 20), palette().background.value);
        EXPECT_EQ(canvas.at(119, 79), palette().background.value);

        panel->rounding = 0.0;
        panel->bordered = false;
        panel->invalidate();
        canvas.frame();

        EXPECT_EQ(canvas.at(20, 20), palette().surface.value);
        EXPECT_EQ(canvas.at(119, 79), palette().surface.value);
    }

    TEST(Panel, UnborderedPanelIsSurfaceToTheEdge) {
        tests::Canvas canvas(200, 120);
        ttk::Panel *panel = mount(canvas, {20, 20, 100, 60});

        panel->bordered = false;
        canvas.frame();

        EXPECT_EQ(canvas.at(20, 50), palette().surface.value);
        EXPECT_EQ(canvas.at(70, 20), palette().surface.value);
    }

    TEST(Panel, InsetPanelIsSunken) {
        tests::Canvas canvas(200, 120);
        ttk::Panel *panel = mount(canvas, {20, 20, 100, 60});

        panel->inset = true;
        canvas.frame();

        EXPECT_EQ(canvas.at(70, 50), palette().sunken.value);
    }

    TEST(Panel, LitPanelDrawsTheStrongBorder) {
        tests::Canvas canvas(200, 120);
        ttk::Panel *panel = mount(canvas, {20, 20, 100, 60});

        panel->lit = true;
        canvas.frame();

        EXPECT_EQ(canvas.at(20, 50), palette().borderStrong.value);
        EXPECT_EQ(canvas.at(70, 50), palette().surface.value) << "a lit panel that is not hoverable was washed";
    }

    TEST(Panel, HoverableLitPanelIsWashed) {
        tests::Canvas canvas(200, 120);
        ttk::Panel *panel = mount(canvas, {20, 20, 100, 60});

        panel->hoverable = true;
        canvas.frame();

        EXPECT_EQ(canvas.at(70, 50), palette().surface.value) << "washed while not lit";

        panel->lit = true;
        panel->invalidate();
        canvas.frame();

        EXPECT_NE(canvas.at(70, 50), palette().surface.value);
        EXPECT_EQ(canvas.at(20, 50), palette().borderStrong.value);
        EXPECT_TRUE(canvas.matches_full());
    }

    TEST(Panel, PaintsChildrenOverItself) {
        tests::Canvas canvas(200, 120);
        auto *panel = canvas.root().layer(ttk::Root::POPUPS)->append(std::make_unique<ttk::Panel>());
        Block *child = panel->append(std::make_unique<Block>(10.0, 10.0, 0xffff0000U));

        panel->fills = false;
        panel->place({20, 20, 100, 60}, type());
        child->place({40, 40, 20, 20}, type());
        canvas.frame();

        EXPECT_EQ(canvas.at(50, 50), 0xffff0000U);
        EXPECT_EQ(canvas.at(90, 50), palette().surface.value);
    }

    TEST(Panel, ChangeThenInvalidateMatchesAFullRepaint) {
        tests::Canvas canvas(200, 120);
        ttk::Panel *panel = mount(canvas, {20.5, 20.25, 100, 60});

        canvas.frame();

        for (const bool state : {true, false, true}) {
            panel->lit = state;
            panel->hoverable = state;
            panel->inset = !state;
            panel->invalidate();
            canvas.frame();

            EXPECT_TRUE(canvas.matches_full()) << "lit " << state;
        }
    }
}
