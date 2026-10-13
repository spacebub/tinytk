// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Mount.h"
#include "support/Canvas.h"
#include "ttk/draw/Glyphs.h"
#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Event.h"
#include "ttk/toolkit/controls/Field.h"
#include "ttk/toolkit/controls/Pill.h"

namespace {
using controls::Calls;
using controls::key;
namespace Code = ttk::Code;

constexpr BLRect BOX{20, 20, 360, 200};
constexpr double CAPTION = 22.0;
constexpr double SPACING = 6.0;

class FieldTest : public controls::Painted {
protected:
    tests::Canvas canvas;
    Calls calls;

    std::unique_ptr<ttk::Field> build(std::string label = "Name") {
        return std::make_unique<ttk::Field>(std::move(label), calls.recorder());
    }

    ttk::Field *place(std::unique_ptr<ttk::Field> field) { return controls::mount(canvas, std::move(field), BOX); }

    ttk::Field *make(std::string label = "Name") { return place(build(std::move(label))); }

    void relayout() {
        canvas.root().relayout();
        canvas.frame();
    }

    static ttk::Widget *input_row(const ttk::Field *field) { return field->children()[1].get(); }

    static ttk::Widget *last_in_row(const ttk::Field *field) { return input_row(field)->children().back().get(); }

    void click_centre(const ttk::Widget *widget) {
        const BLRect &box = widget->box();

        controls::click(canvas.root(), box.x + (box.w / 2.0), box.y + (box.h / 2.0));
    }
};

TEST_F(FieldTest, StartsEmpty) {
    const ttk::Field *field = make();

    EXPECT_TRUE(field->text().empty());
    ASSERT_NE(field->input(), nullptr);
    EXPECT_EQ(&field->text(), &field->input()->text());
}

TEST_F(FieldTest, ValueSetsTheTextWithoutCallingEdited) {
    ttk::Field *field = make();

    EXPECT_EQ(field->value("preset"), field);
    EXPECT_EQ(field->text(), "preset");
    EXPECT_TRUE(calls.texts.empty());
}

TEST_F(FieldTest, SetTextSetsTheTextWithoutCallingEdited) {
    const ttk::Field *field = make();

    field->set_text("other");

    EXPECT_EQ(field->text(), "other");
    EXPECT_TRUE(calls.texts.empty());
}

TEST_F(FieldTest, TypingCallsEdited) {
    const ttk::Field *field = make();

    canvas.root().focus(field->input());
    canvas.root().wrote("ab");
    canvas.root().key(key(Code::Backspace));

    EXPECT_EQ(field->text(), "a");
    EXPECT_EQ(calls.texts, (std::vector<std::string>{"ab", "a"}));
}

TEST_F(FieldTest, ReturnCallsAccepted) {
    ttk::Field *field = make();
    int accepted = 0;

    field->accepted = [&] { ++accepted; };
    canvas.root().focus(field->input());

    EXPECT_TRUE(canvas.root().key(key(Code::Return)));
    EXPECT_EQ(accepted, 1);
}

TEST_F(FieldTest, ReturnWithoutAcceptedIsHarmless) {
    const ttk::Field *field = make();

    canvas.root().focus(field->input());

    EXPECT_TRUE(canvas.root().key(key(Code::Return)));
}

TEST_F(FieldTest, ClickingTheFrameFocusesTheInput) {
    const ttk::Field *field = make();

    click_centre(field->input());

    EXPECT_EQ(canvas.root().focused(), field->input());
}

TEST_F(FieldTest, TakeFocusFocusesTheInputAndSelectsAll) {
    ttk::Field *field = make();

    field->value("replace me");
    field->take_focus();

    EXPECT_EQ(canvas.root().focused(), field->input());

    canvas.root().wrote("new");

    EXPECT_EQ(field->text(), "new");
}

TEST_F(FieldTest, TakeFocusWithoutARootDoesNothing) {
    const ttk::Field field("Name", nullptr);

    field.take_focus();

    EXPECT_FALSE(field.input()->focused());
}

TEST_F(FieldTest, PlaceholderShowsInTheEmptyInput) {
    ttk::Field *field = make();
    const BLRect &input = field->input()->box();
    const BLRectI area{
            static_cast<int>(input.x), static_cast<int>(input.y), static_cast<int>(input.w), static_cast<int>(input.h)};

    canvas.full();

    const std::vector<std::uint32_t> bare = controls::pixels_in(canvas, area);

    EXPECT_EQ(field->placeholder("Type a name"), field);

    canvas.full();

    EXPECT_NE(bare, controls::pixels_in(canvas, area));
}

TEST_F(FieldTest, ReadOnlyRefusesTyping) {
    ttk::Field *field = make();

    EXPECT_EQ(field->read_only(), field);

    canvas.root().focus(field->input());
    canvas.root().wrote("x");

    EXPECT_TRUE(field->text().empty());
    EXPECT_TRUE(calls.texts.empty());
}

TEST_F(FieldTest, SecretHidesTheText) {
    ttk::Field *field = make();
    const BLRect &input = field->input()->box();
    const BLRectI area{
            static_cast<int>(input.x), static_cast<int>(input.y), static_cast<int>(input.w), static_cast<int>(input.h)};

    EXPECT_EQ(field->secret(), field);

    field->value("iiii");
    canvas.full();

    const std::vector<std::uint32_t> narrow = controls::pixels_in(canvas, area);

    field->value("WWWW");
    canvas.full();

    EXPECT_EQ(narrow, controls::pixels_in(canvas, area));
    EXPECT_EQ(field->text(), "WWWW");
}

TEST_F(FieldTest, MonoChangesTheFace) {
    ttk::Field *field = make();
    const BLRect &input = field->input()->box();
    const BLRectI area{
            static_cast<int>(input.x), static_cast<int>(input.y), static_cast<int>(input.w), static_cast<int>(input.h)};

    field->value("iiiiWWWW");
    canvas.full();

    const std::vector<std::uint32_t> proportional = controls::pixels_in(canvas, area);

    EXPECT_EQ(field->mono(), field);

    canvas.full();

    EXPECT_NE(proportional, controls::pixels_in(canvas, area));
}

TEST_F(FieldTest, TheInputSitsBelowTheCaption) {
    const ttk::Field *field = make();

    EXPECT_DOUBLE_EQ(field->input()->box().y, BOX.y + CAPTION + SPACING + 6.0);
    EXPECT_DOUBLE_EQ(field->input()->box().x, BOX.x + 12.0);
    EXPECT_DOUBLE_EQ(field->input()->box().w, BOX.w - 24.0);
    EXPECT_DOUBLE_EQ(field->input()->box().h, ttk::Theme::control - 12.0);
}

TEST_F(FieldTest, AnEmptyLabelHidesTheCaptionRow) {
    const ttk::Field *field = make("");

    EXPECT_DOUBLE_EQ(field->input()->box().y, BOX.y + 6.0);
}

TEST_F(FieldTest, ABadgeShowsTheCaptionRowEvenWithoutALabel) {
    ttk::Field *field = make("");

    EXPECT_EQ(field->badge("New", ttk::Pill::Kind::Success), field);

    relayout();

    EXPECT_DOUBLE_EQ(field->input()->box().y, BOX.y + CAPTION + SPACING + 6.0);

    field->badge("", ttk::Pill::Kind::Success);
    relayout();

    EXPECT_DOUBLE_EQ(field->input()->box().y, BOX.y + 6.0) << "an emptied badge kept the caption row";
}

TEST_F(FieldTest, ANoteMakesTheFieldTaller) {
    ttk::Field *field = make();
    ttk::Typeface &type = tests::fonts();
    const double bare = field->natural_height(type, BOX.w);

    EXPECT_EQ(field->note("A note under the input"), field);
    EXPECT_GT(field->natural_height(type, BOX.w), bare);

    field->note("");

    EXPECT_DOUBLE_EQ(field->natural_height(type, BOX.w), bare);
}

TEST_F(FieldTest, AtLeast240PixelsWide) {
    ttk::Field field("", nullptr);

    EXPECT_GE(field.wanted_width(tests::fonts()), 240.0);
}

TEST_F(FieldTest, IconSitsInsideTheFrameAndNarrowsTheInput) {
    auto made = build();
    int pressed = 0;

    EXPECT_EQ(made->icon(ttk::Glyphs::Glyph::Cog, "Settings", [&] { ++pressed; }), made.get());

    const ttk::Field *field = place(std::move(made));
    const ttk::Widget *icon = last_in_row(field);
    const double frameTop = field->input()->box().y - 6.0;

    EXPECT_DOUBLE_EQ(field->input()->box().w, BOX.w - 12.0 - 42.0);
    EXPECT_DOUBLE_EQ(icon->box().x, BOX.x + BOX.w - 34.0);
    EXPECT_DOUBLE_EQ(icon->box().y, frameTop + ((ttk::Theme::control - 30.0) / 2.0));
    EXPECT_DOUBLE_EQ(icon->box().w, 30.0);
    EXPECT_EQ(icon->hint, "Settings");

    click_centre(icon);

    EXPECT_EQ(pressed, 1);
}

TEST_F(FieldTest, HidingTheIconWidensTheInput) {
    auto made = build();
    int pressed = 0;

    made->icon(ttk::Glyphs::Glyph::Cog, "Settings", [&] { ++pressed; });

    const ttk::Field *field = place(std::move(made));
    const BLRect spot = last_in_row(field)->box();

    field->set_icon_visible(false);
    relayout();

    EXPECT_FALSE(last_in_row(field)->visible());
    EXPECT_DOUBLE_EQ(field->input()->box().w, BOX.w - 24.0);

    controls::click(canvas.root(), spot.x + 15.0, spot.y + 15.0);

    EXPECT_EQ(pressed, 0) << "a hidden icon took the click";

    field->set_icon_visible(true);
    relayout();

    EXPECT_TRUE(last_in_row(field)->visible());
    EXPECT_DOUBLE_EQ(field->input()->box().w, BOX.w - 12.0 - 42.0);
}

TEST_F(FieldTest, SetIconHintChangesTheTooltip) {
    auto made = build();

    made->icon(ttk::Glyphs::Glyph::Cog, "Before", nullptr);

    const ttk::Field *field = place(std::move(made));

    field->set_icon_hint("After");

    EXPECT_EQ(last_in_row(field)->hint, "After");
}

TEST_F(FieldTest, IconCallsWithoutAnIconDoNothing) {
    const ttk::Field *field = make();
    const std::size_t children = input_row(field)->children().size();

    field->set_icon_visible(false);
    field->set_icon_hint("Nothing");

    EXPECT_EQ(input_row(field)->children().size(), children);
    EXPECT_TRUE(field->input()->visible());
}

TEST_F(FieldTest, ActionSitsBesideTheFrame) {
    auto made = build();
    int pressed = 0;

    EXPECT_EQ(made->action("Browse", [&] { ++pressed; }), made.get());

    const ttk::Field *field = place(std::move(made));
    const ttk::Widget *action = last_in_row(field);
    const double frameRight = action->box().x - 8.0;

    EXPECT_DOUBLE_EQ(field->input()->box().x + field->input()->box().w, frameRight - 12.0);
    EXPECT_GT(action->box().x, field->input()->box().x + field->input()->box().w);

    click_centre(action);

    EXPECT_EQ(pressed, 1);
}

TEST_F(FieldTest, PrefixPushesTheInputRight) {
    auto made = build();
    const std::string prefix = "https://";
    ttk::Typeface &type = tests::fonts();

    EXPECT_EQ(made->prefix(prefix), made.get());

    const ttk::Field *field = place(std::move(made));
    const double taken = type.width(type.at(ttk::Typeface::mono, ttk::Theme::fontBody), prefix);

    EXPECT_DOUBLE_EQ(field->input()->box().x, BOX.x + 12.0 + taken + 19.0);
}

TEST_F(FieldTest, LeadingGlyphPushesTheInputRight) {
    auto made = build();

    EXPECT_EQ(made->leading_glyph(ttk::Glyphs::Glyph::Cog), made.get());

    const ttk::Field *field = place(std::move(made));

    EXPECT_DOUBLE_EQ(field->input()->box().x, BOX.x + 12.0 + (12.0 * 1.1) + 9.0);
}

TEST_F(FieldTest, EmptyLeadingGlyphAndPrefixLeaveTheInputWhereItWas) {
    auto made = build();

    made->leading_glyph(ttk::Glyphs::Glyph::Empty)->prefix("");

    const ttk::Field *field = place(std::move(made));

    EXPECT_DOUBLE_EQ(field->input()->box().x, BOX.x + 12.0);
}

TEST_F(FieldTest, GlyphAndPrefixTogether) {
    auto made = build();
    const std::string prefix = "~/";
    ttk::Typeface &type = tests::fonts();

    made->leading_glyph(ttk::Glyphs::Glyph::Cog)->prefix(prefix);

    const ttk::Field *field = place(std::move(made));
    const double taken = type.width(type.at(ttk::Typeface::mono, ttk::Theme::fontBody), prefix);

    EXPECT_DOUBLE_EQ(field->input()->box().x, BOX.x + 12.0 + (12.0 * 1.1) + 9.0 + taken + 19.0);
}

TEST_F(FieldTest, FrameIsOutlinedInTheAccentWhileFocused) {
    const ttk::Field *field = make();
    const ttk::Theme::Palette &palette = ttk::Theme::palette();
    const int x = static_cast<int>(BOX.x + (BOX.w / 2.0));
    const int y = static_cast<int>(field->input()->box().y - 6.0);

    canvas.full();

    EXPECT_EQ(canvas.at(x, y), palette.borderStrong.value);
    EXPECT_EQ(canvas.at(x, y + 10), palette.field.value) << "the frame is not filled with the field colour";

    canvas.root().focus(field->input());
    canvas.frame();

    EXPECT_EQ(canvas.at(x, y), palette.accent.value);

    canvas.root().focus(nullptr);
    canvas.frame();

    EXPECT_EQ(canvas.at(x, y), palette.borderStrong.value) << "the outline kept the accent after focus left";
}

TEST_F(FieldTest, LeadingGlyphTakesTheAccentWhileFocused) {
    auto made = build();

    made->leading_glyph(ttk::Glyphs::Glyph::Cog);

    const ttk::Field *field = place(std::move(made));
    const double side = ttk::Glyphs::span(1.1F);
    const double frameTop = field->input()->box().y - 6.0;
    const BLRectI area{static_cast<int>(BOX.x + 12.0),
                       static_cast<int>(frameTop + ((ttk::Theme::control - side) / 2.0)),
                       static_cast<int>(side),
                       static_cast<int>(side)};
    const ttk::Theme::Palette &palette = ttk::Theme::palette();

    canvas.full();

    EXPECT_EQ(controls::count_near(canvas, area, palette.accent, 40), 0);
    EXPECT_GT(controls::count_near(canvas, area, palette.faint, 40), 0) << "the resting glyph is not faint";

    canvas.root().focus(field->input());
    canvas.frame();

    EXPECT_GT(controls::count_near(canvas, area, palette.accent, 40), 0);
}

TEST_F(FieldTest, PrefixIsDrawnWithADivider) {
    auto made = build();
    const std::string prefix = "https://";
    ttk::Typeface &type = tests::fonts();

    made->prefix(prefix);

    const ttk::Field *field = place(std::move(made));
    const double taken = type.width(type.at(ttk::Typeface::mono, ttk::Theme::fontBody), prefix);
    const double frameTop = field->input()->box().y - 6.0;
    const auto divider = static_cast<int>(std::round(BOX.x + 12.0 + taken + 9.0));

    canvas.full();

    EXPECT_TRUE(controls::leans(canvas.at(divider, static_cast<int>(frameTop + (ttk::Theme::control / 2.0))),
                                ttk::Theme::palette().borderStrong,
                                ttk::Theme::palette().field));
}

TEST_F(FieldTest, PartialRepaintsMatchAFullOne) {
    auto made = build();

    made->leading_glyph(ttk::Glyphs::Glyph::Cog)->prefix("~/")->note("note")->badge("New", ttk::Pill::Kind::None);

    const ttk::Field *field = place(std::move(made));

    field->take_focus();
    canvas.frame();
    canvas.root().wrote("typed");
    canvas.frame();
    canvas.root().focus(nullptr);
    canvas.frame();

    EXPECT_TRUE(canvas.matches_full());
}
}
