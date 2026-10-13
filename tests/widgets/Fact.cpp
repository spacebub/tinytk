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
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Screen.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/system/Env.h"
#include "ttk/toolkit/controls/Fact.h"
#include "ttk/toolkit/controls/Label.h"

namespace {
    using widgets::Painted;

    constexpr BLRect BOX{20, 20, 300, 60};
    constexpr BLRectI PIXELS{20, 20, 300, 60};

    ttk::Typeface &type() {
        return tests::fonts();
    }

    ttk::Label *caption(const ttk::Fact &fact) {
        return dynamic_cast<ttk::Label *>(fact.children().at(0).get());
    }

    ttk::Label *value(const ttk::Fact &fact) {
        return dynamic_cast<ttk::Label *>(fact.children().at(1).get());
    }

    double tracked(const std::string &text) {
        return type().width_tracked(type().at(600, ttk::Theme::fontTiny), text, 0.9F);
    }

    double semibold(const std::string &text) {
        return type().width(type().at(600, ttk::Theme::fontBody), text);
    }

    struct Mounted {
        tests::Canvas canvas;
        ttk::Fact *fact = nullptr;

        explicit Mounted(const std::string &label = "Config", const std::string &shown = "Ultra-Violence") {
            fact = widgets::mount(canvas, std::make_unique<ttk::Fact>(label, shown), BOX);
        }

        [[nodiscard]] BLRectI caption_pixels() const { return widgets::pixels_of(caption(*fact)->box()); }
        [[nodiscard]] BLRectI value_pixels() const { return widgets::pixels_of(value(*fact)->box()); }
    };

    TEST(Fact, HoldsACaptionOverAValue) {
        const ttk::Fact fact("Config", "Ultra-Violence");

        ASSERT_EQ(fact.children().size(), 2U);
        ASSERT_NE(caption(fact), nullptr);
        ASSERT_NE(value(fact), nullptr);
        EXPECT_EQ(caption(fact)->text(), "Config");
        EXPECT_EQ(value(fact)->text(), "Ultra-Violence");
    }

    TEST(Fact, SetValueReplacesTheValueText) {
        const ttk::Fact fact("Config", "Before");

        fact.set_value("After é");

        EXPECT_EQ(value(fact)->text(), "After é");
        EXPECT_EQ(caption(fact)->text(), "Config");
    }

    TEST(Fact, ChainedSettersReturnTheFact) {
        ttk::Fact fact("Config", "/srv/file");

        EXPECT_EQ(fact.path(), &fact);
        EXPECT_EQ(fact.on_click("Open it", [] {}), &fact);
    }

    TEST(Fact, OnClickMakesTheValueALinkWithATooltip) {
        ttk::Fact fact("Config", "/srv/file");

        fact.on_click("Opens the folder", [] {});

        EXPECT_EQ(value(fact)->hint, "Opens the folder");
        EXPECT_EQ(value(fact)->cursor, ttk::Cursor::Pointer);
        EXPECT_TRUE(caption(fact)->hint.empty());
    }

    TEST_F(Painted, FactCaptionIsASectionCaption) {
        const ttk::Fact fact("Config é", "Value");

        EXPECT_DOUBLE_EQ(caption(fact)->natural_width(type()), tracked("CONFIG é"));
        EXPECT_DOUBLE_EQ(caption(fact)->natural_height(type(), 100),
                         type().line_height(type().at(600, ttk::Theme::fontTiny)));
    }

    TEST_F(Painted, FactValueIsSemiboldBodyType) {
        const ttk::Fact fact("Config", "Ultra-Violence");

        EXPECT_DOUBLE_EQ(value(fact)->natural_width(type()), semibold("Ultra-Violence"));
        EXPECT_DOUBLE_EQ(value(fact)->natural_height(type(), 100),
                         type().line_height(type().at(600, ttk::Theme::fontBody)));
    }

    TEST_F(Painted, FactNaturalWidthIsTheWiderOfTheTwo) {
        ttk::Fact longValue("Id", "A value much wider than its caption");
        ttk::Fact longCaption("A caption much wider than its value", "1");

        EXPECT_DOUBLE_EQ(longValue.natural_width(type()), semibold("A value much wider than its caption"));
        EXPECT_DOUBLE_EQ(longCaption.natural_width(type()), tracked("A CAPTION MUCH WIDER THAN ITS VALUE"));
    }

    TEST_F(Painted, FactNaturalHeightStacksBothLinesWithAGap) {
        ttk::Fact fact("Config", "Value");

        EXPECT_DOUBLE_EQ(fact.natural_height(type(), 300),
                         type().line_height(type().at(600, ttk::Theme::fontTiny)) + 3.0
                                 + type().line_height(type().at(600, ttk::Theme::fontBody)));
    }

    TEST_F(Painted, FactEmptyTextsStillStackTwoLines) {
        ttk::Fact fact("", "");

        EXPECT_DOUBLE_EQ(fact.natural_width(type()), 0.0);
        EXPECT_GT(fact.natural_height(type(), 300), type().line_height(type().at(600, ttk::Theme::fontBody)));
    }

    TEST_F(Painted, FactPathMeasuresTheValueInTheBoldFixedFace) {
        const std::string path = "/srv/games/doom/eviternity.wad";
        ttk::Fact fact("File", path);

        fact.path();

        EXPECT_DOUBLE_EQ(value(fact)->natural_width(type()),
                         type().width(type().at(ttk::Typeface::monoBold, ttk::Theme::fontBody), path));
    }

    TEST_F(Painted, FactPlacesTheValueUnderTheCaption) {
        const Mounted mounted;

        EXPECT_DOUBLE_EQ(caption(*mounted.fact)->box().y, BOX.y);
        EXPECT_DOUBLE_EQ(value(*mounted.fact)->box().y, BOX.y + caption(*mounted.fact)->box().h + 3.0);
    }

    TEST_F(Painted, FactPaintsTheCaptionFaintAndTheValueInText) {
        const Mounted mounted;
        const std::uint32_t captionInk = widgets::peak(mounted.canvas, mounted.caption_pixels(), widgets::background());
        const std::uint32_t valueInk = widgets::peak(mounted.canvas, mounted.value_pixels(), widgets::background());

        EXPECT_LT(widgets::distance(captionInk, widgets::palette().faint.value),
                  widgets::distance(captionInk, widgets::palette().text.value))
                << std::hex << captionInk;
        EXPECT_LT(widgets::distance(valueInk, widgets::palette().text.value),
                  widgets::distance(valueInk, widgets::palette().faint.value))
                << std::hex << valueInk;
        EXPECT_EQ(widgets::outside(mounted.canvas, PIXELS), 0U);
    }

    TEST_F(Painted, FactSetValueRepaintsOnlyTheValue) {
        Mounted mounted;
        const std::vector<std::uint32_t> before = mounted.canvas.pixels();

        mounted.fact->set_value("Nightmare!");

        const std::size_t painted = mounted.canvas.frame();

        EXPECT_GT(painted, 0U);
        EXPECT_LE(painted, widgets::area(value(*mounted.fact)->box()));
        EXPECT_NE(mounted.canvas.pixels(), before);
        EXPECT_TRUE(mounted.canvas.matches_full());
    }

    TEST_F(Painted, FactSetValueToTheSameTextDamagesNothing) {
        Mounted mounted;

        mounted.fact->set_value("Ultra-Violence");

        EXPECT_FALSE(mounted.canvas.root().dirty());
    }

    TEST_F(Painted, FactLinkValueCallsOnClick) {
        Mounted mounted;
        int clicks = 0;

        mounted.fact->on_click("Opens it", [&clicks] { ++clicks; });
        mounted.canvas.root().relayout();
        mounted.canvas.frame();

        const BLRect box = value(*mounted.fact)->box();

        widgets::click(mounted.canvas, box.x + 4, box.y + (box.h / 2));

        EXPECT_EQ(clicks, 1);

        widgets::click(mounted.canvas, box.x + 290, box.y + (box.h / 2));

        EXPECT_EQ(clicks, 1) << "a click past the value text went through";

        const BLRect head = caption(*mounted.fact)->box();

        widgets::click(mounted.canvas, head.x + 4, head.y + (head.h / 2));

        EXPECT_EQ(clicks, 1) << "the caption is not part of the link";
    }

    TEST_F(Painted, FactLinkValueLightsOnHover) {
        Mounted mounted;

        mounted.fact->on_click("Opens it", [] {});
        mounted.canvas.root().relayout();
        mounted.canvas.frame();

        const BLRect box = value(*mounted.fact)->box();

        widgets::hover(mounted.canvas, box.x + 4, box.y + (box.h / 2));

        const std::size_t painted = mounted.canvas.frame();
        const std::uint32_t ink = widgets::peak(mounted.canvas, mounted.value_pixels(), widgets::background());

        EXPECT_LE(painted, widgets::area(box));
        EXPECT_LT(widgets::distance(ink, widgets::palette().accent.value),
                  widgets::distance(ink, widgets::palette().text.value))
                << std::hex << ink;
        EXPECT_TRUE(mounted.canvas.matches_full());
    }

    TEST_F(Painted, FactPathValueWritesTheHomeAsTilde) {
        const std::string home = ttk::Env::get("HOME");

        ASSERT_FALSE(home.empty());

        tests::Canvas pathCanvas;
        auto made = std::make_unique<ttk::Fact>("File", home + "/games/file.wad");

        made->path();
        widgets::mount(pathCanvas, std::move(made), BOX);

        tests::Canvas tildeCanvas;
        auto shown = std::make_unique<ttk::Fact>("File", "~/games/file.wad");

        value(*shown)->mono();
        widgets::mount(tildeCanvas, std::move(shown), BOX);

        EXPECT_EQ(pathCanvas.pixels(), tildeCanvas.pixels());
    }

    TEST_F(Painted, FactFollowsAPaletteChange) {
        Mounted mounted;

        ttk::Theme::set_mode(ttk::Theme::Mode::Light);
        mounted.canvas.frame();

        const std::uint32_t valueInk = widgets::peak(mounted.canvas, mounted.value_pixels(), widgets::background());

        EXPECT_EQ(mounted.canvas.at(PIXELS.x + PIXELS.w - 1, PIXELS.y), widgets::background());
        EXPECT_LT(widgets::distance(valueInk, widgets::palette().text.value), 60) << std::hex << valueInk;
        EXPECT_TRUE(mounted.canvas.matches_full());
    }
}
