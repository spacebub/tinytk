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
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "Screen.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/system/Env.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/layout/Box.h"

namespace {
using widgets::Painted;

constexpr BLRect BOX{20, 20, 300, 64};
constexpr BLRectI PIXELS{20, 20, 300, 64};
constexpr int BIG_WEIGHT = 700;
constexpr float BIG = 44.0F;

constexpr std::string_view PARAGRAPH = "A paragraph long enough to fold over several lines when it is given "
                                       "only a narrow column, with accents é й and a few more words to be sure.";

std::string paragraph(const int times = 1) {
    std::string out(PARAGRAPH);

    for (int more = 1; more < times; ++more) {
        out += ' ';
        out += PARAGRAPH;
    }

    return out;
}

ttk::Typeface &type() {
    return tests::fonts();
}

double width(const int weight, const float size, const std::string &text) {
    return type().width(type().at(weight, size), text);
}

double line(const int weight, const float size) {
    return type().line_height(type().at(weight, size));
}

std::unique_ptr<ttk::Label> big(const std::string &text) {
    auto made = std::make_unique<ttk::Label>(text);

    made->font(BIG_WEIGHT, BIG);

    return made;
}

struct Mounted {
    tests::Canvas canvas;
    ttk::Label *label = nullptr;
    int clicks = 0;

    explicit Mounted(std::unique_ptr<ttk::Label> made, const BLRect &box = BOX) {
        label = widgets::mount(canvas, std::move(made), box);
    }

    explicit Mounted(const std::string &text, const BLRect &box = BOX)
        : Mounted(std::make_unique<ttk::Label>(text), box) {}

    [[nodiscard]] widgets::Ink drawn(const BLRectI &region = PIXELS) const {
        return widgets::ink(canvas, region, widgets::background());
    }
};

struct Column {
    tests::Canvas canvas;
    ttk::Label *label = nullptr;

    explicit Column(std::unique_ptr<ttk::Label> made) {
        auto box = ttk::Box::column();

        label = box->append(std::move(made));
        canvas.root().content()->add(std::move(box));
        canvas.frame();
    }
};

TEST(Label, TextIsReturnedAsGiven) {
    const ttk::Label label("Some text");

    EXPECT_EQ(label.text(), "Some text");
    EXPECT_EQ(ttk::Label().text(), "");
}

TEST(Label, SetTextReplacesTheText) {
    ttk::Label label("Before");

    label.set_text("After é");

    EXPECT_EQ(label.text(), "After é");

    label.set_text("");

    EXPECT_EQ(label.text(), "");
}

TEST(Label, ChainedSettersReturnTheLabel) {
    ttk::Label label("Text");

    EXPECT_EQ(label.font(600, 15), &label);
    EXPECT_EQ(label.tone(&ttk::Theme::Palette::muted), &label);
    EXPECT_EQ(label.align(ttk::Align::Centre), &label);
    EXPECT_EQ(label.wrap(), &label);
    EXPECT_EQ(label.elide(3), &label);
    EXPECT_EQ(label.section(), &label);
    EXPECT_EQ(label.mono(), &label);
    EXPECT_EQ(label.on_click([] {}), &label);
    EXPECT_EQ(label.path(), &label);
}

TEST(Label, SectionKeepsTheTextAsGiven) {
    ttk::Label label("Section é");

    label.section();

    EXPECT_EQ(label.text(), "Section é");
}

TEST(Label, PlainLabelDoesNotTakeThePointer) {
    ttk::Label label("Text");

    label.place(BOX, type());

    EXPECT_EQ(label.at(BOX.x + 5, BOX.y + 5), nullptr);
    EXPECT_FALSE(label.press(ttk::Pointer{.x = BOX.x + 5, .y = BOX.y + 5}));
    EXPECT_EQ(label.cursor, ttk::Cursor::Default);
}

TEST(Label, PlainLabelWithAHintIsHit) {
    ttk::Label label("Text");

    label.hint = "More about it";
    label.place(BOX, type());

    EXPECT_EQ(label.at(BOX.x + 250, BOX.y + 5), &label);
}

TEST(Label, FixedSizesOverrideTheNaturalOnes) {
    ttk::Label label("Text");

    label.fixedWidth = 33;
    label.fixedHeight = 44;

    EXPECT_DOUBLE_EQ(label.natural_width(type()), 33);
    EXPECT_DOUBLE_EQ(label.natural_height(type(), 10), 44);

    label.wrap();

    EXPECT_DOUBLE_EQ(label.natural_height(type(), 10), 44);
}

TEST_F(Painted, LabelNaturalWidthIsTheTextOnOneLine) {
    ttk::Label label("Ultra-Violence");

    EXPECT_DOUBLE_EQ(label.natural_width(type()), width(400, ttk::Theme::fontBody, "Ultra-Violence"));
}

TEST_F(Painted, LabelEmptyTextHasNoWidth) {
    ttk::Label label;

    EXPECT_DOUBLE_EQ(label.natural_width(type()), 0.0);
}

TEST_F(Painted, LabelNaturalHeightIsOneLineAtAnyWidth) {
    ttk::Label label(paragraph());
    const double one = line(400, ttk::Theme::fontBody);

    EXPECT_DOUBLE_EQ(label.natural_height(type(), 1000), one);
    EXPECT_DOUBLE_EQ(label.natural_height(type(), 40), one);
    EXPECT_DOUBLE_EQ(label.natural_height(type(), 0), one);
    EXPECT_DOUBLE_EQ(ttk::Label().natural_height(type(), 100), one) << "an empty label is still one line tall";
}

TEST_F(Painted, LabelFontSetsTheMeasure) {
    ttk::Label label("Heading");

    label.font(BIG_WEIGHT, BIG);

    EXPECT_DOUBLE_EQ(label.natural_width(type()), width(BIG_WEIGHT, BIG, "Heading"));
    EXPECT_DOUBLE_EQ(label.natural_height(type(), 500), line(BIG_WEIGHT, BIG));
    EXPECT_GT(label.natural_width(type()), width(400, ttk::Theme::fontBody, "Heading"));
}

TEST_F(Painted, LabelWrappedHeightFollowsTheWidth) {
    ttk::Label label(paragraph());
    const BLFont &face = type().at(400, ttk::Theme::fontBody);

    label.wrap();

    EXPECT_DOUBLE_EQ(label.natural_height(type(), 120), ttk::wrap_height(type(), face, paragraph(), 120));
    EXPECT_GT(label.natural_height(type(), 120), 3 * line(400, ttk::Theme::fontBody));
    EXPECT_GT(label.natural_height(type(), 120), label.natural_height(type(), 400));
    EXPECT_DOUBLE_EQ(label.natural_height(type(), 100000), line(400, ttk::Theme::fontBody));
}

TEST_F(Painted, LabelWrappedEmptyTextIsOneLine) {
    ttk::Label label;

    label.wrap();

    EXPECT_DOUBLE_EQ(label.natural_height(type(), 100), line(400, ttk::Theme::fontBody));
}

TEST_F(Painted, LabelWrapOffMeasuresOneLineAgain) {
    ttk::Label label(paragraph());

    label.wrap()->wrap(false);

    EXPECT_DOUBLE_EQ(label.natural_height(type(), 120), line(400, ttk::Theme::fontBody));
}

TEST_F(Painted, LabelElidedHeightStopsAtTheRows) {
    ttk::Label label(paragraph(4));
    const double one = line(400, ttk::Theme::fontBody);

    label.elide(2);

    EXPECT_DOUBLE_EQ(label.natural_height(type(), 120), 2 * one);
    EXPECT_DOUBLE_EQ(label.natural_height(type(), 100000), one) << "fewer lines than the cap were padded";

    label.elide(0);

    EXPECT_GT(label.natural_height(type(), 120), 3 * one) << "zero did not lift the cap";
}

TEST_F(Painted, LabelElideWrapsOnItsOwn) {
    ttk::Label label(paragraph());

    label.elide(3);

    EXPECT_DOUBLE_EQ(label.natural_height(type(), 120), 3 * line(400, ttk::Theme::fontBody));
}

TEST_F(Painted, LabelElidedEmptyTextIsOneLine) {
    ttk::Label label;

    label.elide(2);

    EXPECT_DOUBLE_EQ(label.natural_height(type(), 100), line(400, ttk::Theme::fontBody));
}

TEST_F(Painted, LabelSectionIsSmallTrackedUpperCase) {
    ttk::Label label("Section é tail");

    label.section();

    EXPECT_DOUBLE_EQ(label.natural_width(type()),
                     type().width_tracked(type().at(600, ttk::Theme::fontTiny), "SECTION é TAIL", 0.9F))
            << "only ASCII letters are upper cased";
    EXPECT_DOUBLE_EQ(label.natural_height(type(), 100), line(600, ttk::Theme::fontTiny));
}

TEST_F(Painted, LabelSectionSetTextUpperCasesTheNewText) {
    ttk::Label label("Old");

    label.section();
    label.set_text("new words");

    EXPECT_DOUBLE_EQ(label.natural_width(type()),
                     type().width_tracked(type().at(600, ttk::Theme::fontTiny), "NEW WORDS", 0.9F));
}

TEST_F(Painted, LabelMonoUsesTheFixedWidthFace) {
    ttk::Label narrow("iiii");
    ttk::Label wide("MMMM");

    narrow.mono();
    wide.mono();

    EXPECT_DOUBLE_EQ(narrow.natural_width(type()), wide.natural_width(type()));
    EXPECT_DOUBLE_EQ(narrow.natural_width(type()), width(ttk::Typeface::mono, ttk::Theme::fontBody, "iiii"));
}

TEST_F(Painted, LabelSemiboldMonoUsesTheBoldFixedFace) {
    ttk::Label label("code");

    label.font(600, ttk::Theme::fontBody)->mono();

    EXPECT_DOUBLE_EQ(label.natural_width(type()), width(ttk::Typeface::monoBold, ttk::Theme::fontBody, "code"));
}

TEST_F(Painted, LabelUtf8TextIsMeasured) {
    const std::string text = "Größe é й 日本 ✓";
    ttk::Label label(text);

    EXPECT_DOUBLE_EQ(label.natural_width(type()), width(400, ttk::Theme::fontBody, text));
    EXPECT_GT(label.natural_width(type()), 0.0);
}

TEST_F(Painted, LabelVeryLongTextIsMeasured) {
    const std::string text(20000, 'x');
    ttk::Label label(text);

    EXPECT_GT(label.natural_width(type()), 20000.0);
}

TEST_F(Painted, LabelPaintsInTheTextColour) {
    const Mounted mounted(big("HH"));

    EXPECT_GT(widgets::count(mounted.canvas, PIXELS, widgets::palette().text), 0U);
}

TEST_F(Painted, LabelInkStaysInsideItsBox) {
    const Mounted mounted(big("Hello, world"));

    EXPECT_GT(mounted.drawn().pixels, 0U);
    EXPECT_EQ(widgets::outside(mounted.canvas, PIXELS), 0U);
}

TEST_F(Painted, LabelInkIsCentredVertically) {
    const Mounted mounted(big("HH"), BLRect{20, 20, 300, 160});
    const widgets::Ink drawn = mounted.drawn(BLRectI{20, 20, 300, 160});

    EXPECT_NEAR((drawn.top + drawn.bottom) / 2.0, 100.0, 6.0) << drawn.top << ".." << drawn.bottom;
}

TEST_F(Painted, LabelSlotToneSetsTheColour) {
    auto made = big("HH");

    made->tone(&ttk::Theme::Palette::danger);

    const Mounted mounted(std::move(made));

    EXPECT_GT(widgets::count(mounted.canvas, PIXELS, widgets::palette().danger), 0U);
    EXPECT_EQ(widgets::count(mounted.canvas, PIXELS, widgets::palette().text), 0U);
}

TEST_F(Painted, LabelFixedToneSetsTheColour) {
    auto made = big("HH");

    made->tone(ttk::Theme::Tone(BLRgba32(0xff3366cc)));

    const Mounted mounted(std::move(made));

    EXPECT_GT(widgets::count(mounted.canvas, PIXELS, BLRgba32(0xff3366cc)), 0U);
}

TEST_F(Painted, LabelDerivedToneSetsTheColour) {
    auto made = big("HH");

    made->tone(ttk::Theme::Tone([](const ttk::Theme::Palette &palette) { return palette.success; }));

    const Mounted mounted(std::move(made));

    EXPECT_GT(widgets::count(mounted.canvas, PIXELS, widgets::palette().success), 0U);
}

TEST_F(Painted, LabelSlotToneFollowsAPaletteChange) {
    Mounted mounted(big("HH"));

    ttk::Theme::set_mode(ttk::Theme::Mode::Light);
    mounted.canvas.frame();

    EXPECT_GT(widgets::count(mounted.canvas, PIXELS, widgets::palette().text), 0U);
    EXPECT_EQ(widgets::count(mounted.canvas, PIXELS, ttk::Theme::palette(ttk::Theme::Mode::Dark).text), 0U);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, LabelFixedToneIgnoresAPaletteChange) {
    auto made = big("HH");

    made->tone(ttk::Theme::Tone(BLRgba32(0xff3366cc)));

    Mounted mounted(std::move(made));

    ttk::Theme::set_mode(ttk::Theme::Mode::Light);
    mounted.canvas.frame();

    EXPECT_GT(widgets::count(mounted.canvas, PIXELS, BLRgba32(0xff3366cc)), 0U);
}

TEST_F(Painted, LabelSectionPaintsFaint) {
    auto made = std::make_unique<ttk::Label>("SECTION");

    made->section();
    made->font(BIG_WEIGHT, BIG);

    const Mounted mounted(std::move(made));

    EXPECT_GT(widgets::count(mounted.canvas, PIXELS, widgets::palette().faint), 0U);
}

TEST_F(Painted, LabelSectionInkIsTheTrackedWidth) {
    auto made = std::make_unique<ttk::Label>("Section");

    made->section();

    const double wide = made->natural_width(type());
    const Mounted mounted(std::move(made));
    const widgets::Ink drawn = mounted.drawn();

    EXPECT_NEAR(drawn.right - drawn.left, wide, 4.0);
    EXPECT_LE(drawn.left, PIXELS.x + 2);
}

struct Placement {
    ttk::Align align;
    bool section;
    const char *name;
};

class Aligned : public Painted, public ::testing::WithParamInterface<Placement> {};

TEST_P(Aligned, PlacesTheLineAcrossTheBox) {
    auto made = std::make_unique<ttk::Label>("Middle");

    if (GetParam().section) {
        made->section();
    }

    made->align(GetParam().align);

    const Mounted mounted(std::move(made));
    const widgets::Ink drawn = mounted.drawn();

    ASSERT_GT(drawn.pixels, 0U);

    switch (GetParam().align) {
        case ttk::Align::Start:
            EXPECT_LE(drawn.left, PIXELS.x + 3);
            break;
        case ttk::Align::Centre:
            EXPECT_NEAR((drawn.left + drawn.right) / 2.0, PIXELS.x + (PIXELS.w / 2.0), 3.0);
            break;
        case ttk::Align::End:
            EXPECT_GE(drawn.right, PIXELS.x + PIXELS.w - 4);
            break;
    }

    EXPECT_EQ(widgets::outside(mounted.canvas, PIXELS), 0U);
}

INSTANTIATE_TEST_SUITE_P(Label,
                         Aligned,
                         ::testing::Values(Placement{ttk::Align::Start, false, "Start"},
                                           Placement{ttk::Align::Centre, false, "Centre"},
                                           Placement{ttk::Align::End, false, "End"},
                                           Placement{ttk::Align::Start, true, "SectionStart"},
                                           Placement{ttk::Align::Centre, true, "SectionCentre"},
                                           Placement{ttk::Align::End, true, "SectionEnd"}),
                         [](const ::testing::TestParamInfo<Placement> &each) { return std::string(each.param.name); });

TEST_F(Painted, LabelWrappedTextIgnoresAlignment) {
    auto made = std::make_unique<ttk::Label>(paragraph());

    made->wrap()->align(ttk::Align::End);

    const Mounted mounted(std::move(made), BLRect{20, 20, 300, 200});
    const widgets::Ink drawn = mounted.drawn(BLRectI{20, 20, 300, 200});

    EXPECT_LE(drawn.left, 23) << "wrapped text was not placed at the start";
}

TEST_F(Painted, LabelWrappedTextPaintsSeveralLines) {
    auto made = std::make_unique<ttk::Label>(paragraph());

    made->wrap();

    const Mounted mounted(std::move(made), BLRect{20, 20, 150, 200});
    const widgets::Ink drawn = mounted.drawn(BLRectI{20, 20, 150, 200});

    EXPECT_GT(drawn.bottom - drawn.top, 3 * line(400, ttk::Theme::fontBody));
    EXPECT_LE(drawn.right, 170) << "a wrapped line ran past the width";
    EXPECT_LT(drawn.top, 20 + (line(400, ttk::Theme::fontBody) / 2)) << "wrapped text does not start at the top";
}

TEST_F(Painted, LabelElidedTextPaintsNoMoreThanTheRows) {
    auto made = std::make_unique<ttk::Label>(paragraph());
    made->elide(2);
    const Mounted mounted(std::move(made), BLRect{20, 20, 150, 200});
    const widgets::Ink drawn = mounted.drawn(BLRectI{20, 20, 150, 200});
    const double one = line(400, ttk::Theme::fontBody);

    EXPECT_GT(drawn.bottom - drawn.top, one);
    EXPECT_LE(drawn.bottom - drawn.top, (2 * one) + 2);
    EXPECT_LE(drawn.right, 170) << "the cut line ran past the width";
}

// The lines before the cap fold as a paragraph does. The last one carries the rest, so it differs.
TEST_F(Painted, LabelElidedLastLineEndsInAnEllipsis) {
    auto capped = std::make_unique<ttk::Label>(paragraph());
    auto whole = std::make_unique<ttk::Label>(paragraph());
    capped->elide(2);
    whole->wrap();
    const Mounted cut(std::move(capped), BLRect{20, 20, 150, 200});
    const Mounted plain(std::move(whole), BLRect{20, 20, 150, 200});
    const int one = static_cast<int>(line(400, ttk::Theme::fontBody));
    std::size_t first = 0;
    std::size_t last = 0;

    for (int y = 20; y < 20 + (2 * one); ++y) {
        for (int x = 20; x < 170; ++x) {
            const bool differs = cut.canvas.at(x, y) != plain.canvas.at(x, y);

            (y < 20 + one ? first : last) += differs ? 1U : 0U;
        }
    }

    EXPECT_EQ(first, 0U) << "the first line was not drawn as it folds";
    EXPECT_GT(last, 0U) << "the last line was drawn as it folds, with no ellipsis";
}

TEST_F(Painted, LabelUnderTheCapPaintsAsAWrappedOne) {
    auto capped = std::make_unique<ttk::Label>("Two lines of words that fold once in this width");
    auto whole = std::make_unique<ttk::Label>("Two lines of words that fold once in this width");
    capped->elide(4);
    whole->wrap();
    const Mounted cut(std::move(capped), BLRect{20, 20, 150, 200});
    const Mounted plain(std::move(whole), BLRect{20, 20, 150, 200});

    EXPECT_GT(cut.drawn(BLRectI{20, 20, 150, 200}).pixels, 0U);
    EXPECT_EQ(cut.canvas.pixels(), plain.canvas.pixels());
}

TEST_F(Painted, LabelElidesTextWiderThanItsBox) {
    constexpr BLRect narrow{20, 20, 80, 30};
    const Mounted mounted(paragraph(), narrow);
    const widgets::Ink drawn = mounted.drawn(widgets::pixels_of(narrow));

    EXPECT_GT(drawn.pixels, 0U);
    EXPECT_GE(drawn.right, 90) << "the elided text did not use its room";
    EXPECT_EQ(widgets::outside(mounted.canvas, widgets::pixels_of(narrow)), 0U);
}

TEST_F(Painted, LabelElidedTextDiffersFromACrop) {
    constexpr BLRect narrow{20, 20, 80, 30};
    const Mounted elided(paragraph(), narrow);
    const Mounted whole(paragraph(), BLRect{20, 20, 380, 30});
    std::size_t differ = 0;

    for (int y = 20; y < 50; ++y) {
        for (int x = 20; x < 100; ++x) {
            differ += elided.canvas.at(x, y) != whole.canvas.at(x, y) ? 1U : 0U;
        }
    }

    EXPECT_GT(differ, 0U) << "no ellipsis was drawn";
}

TEST_F(Painted, LabelInATinyBoxPaintsOnlyThere) {
    constexpr BLRect tiny{30, 30, 3, 30};
    Mounted mounted(paragraph(), tiny);

    EXPECT_EQ(widgets::outside(mounted.canvas, widgets::pixels_of(tiny)), 0U);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, LabelInAnEmptyBoxPaintsNothing) {
    const Mounted mounted(paragraph(), BLRect{30, 30, 0, 0});

    EXPECT_EQ(widgets::outside(mounted.canvas, BLRectI{0, 0, 0, 0}), 0U);
}

TEST_F(Painted, LabelEmptyTextPaintsNothing) {
    const Mounted mounted("");

    EXPECT_EQ(mounted.drawn().pixels, 0U);
}

TEST_F(Painted, LabelUtf8TextPaintsInsideItsBox) {
    const Mounted mounted("Größe é й ✓ and ünïcödé");

    EXPECT_GT(mounted.drawn().pixels, 0U);
    EXPECT_EQ(widgets::outside(mounted.canvas, PIXELS), 0U);
}

TEST_F(Painted, LabelSetTextRepaintsOnlyTheLabel) {
    Mounted mounted("First text");
    const std::vector<std::uint32_t> before = mounted.canvas.pixels();

    mounted.label->set_text("Second, other text");

    EXPECT_TRUE(mounted.canvas.root().dirty());

    const std::size_t painted = mounted.canvas.frame();

    EXPECT_GT(painted, 0U);
    EXPECT_LE(painted, widgets::area(BOX));
    EXPECT_NE(mounted.canvas.pixels(), before);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, LabelSetTextToEmptyClearsTheInk) {
    Mounted mounted("Some text");

    mounted.label->set_text("");
    mounted.canvas.frame();

    EXPECT_EQ(mounted.drawn().pixels, 0U);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, LabelSetTextToTheSameTextDamagesNothing) {
    Mounted mounted("Same");

    mounted.label->set_text("Same");

    EXPECT_FALSE(mounted.canvas.root().dirty());
}

TEST_F(Painted, LabelSetTextKeepsTheBoxAndElides) {
    Column column(std::make_unique<ttk::Label>("Short"));
    const BLRect was = column.label->box();

    column.label->set_text(paragraph(2));

    EXPECT_FALSE(column.canvas.root().settle()) << "a one line label asked for a new layout";
    EXPECT_DOUBLE_EQ(column.label->box().h, was.h);

    column.canvas.frame();

    EXPECT_EQ(widgets::outside(column.canvas, widgets::pixels_of(column.label->box())), 0U);
    EXPECT_TRUE(column.canvas.matches_full());
}

TEST_F(Painted, LabelWrappedSetTextRelayoutsWhenItsHeightChanges) {
    auto made = std::make_unique<ttk::Label>("One line");

    made->wrap();

    Column column(std::move(made));
    const double was = column.label->box().h;

    column.label->set_text(paragraph(4));

    EXPECT_TRUE(column.canvas.root().settle()) << "a wrapped label that grew asked for no layout";
    EXPECT_GT(column.label->box().h, was);

    column.canvas.frame();

    EXPECT_TRUE(column.canvas.matches_full());
}

TEST_F(Painted, LabelWrappedSetTextOfTheSameHeightKeepsTheLayout) {
    auto made = std::make_unique<ttk::Label>("One line");

    made->wrap();

    Column column(std::move(made));

    column.label->set_text("Another line");

    EXPECT_FALSE(column.canvas.root().settle()) << "a wrapped label of unchanged height asked for a layout";
    EXPECT_TRUE(column.canvas.root().dirty());

    const std::size_t painted = column.canvas.frame();

    EXPECT_LE(painted, widgets::area(column.label->box()));
    EXPECT_TRUE(column.canvas.matches_full());
}

TEST_F(Painted, LabelElidedSetTextRelayoutsOnlyUpToTheCap) {
    auto made = std::make_unique<ttk::Label>("One line");
    made->elide(2);
    Column column(std::move(made));
    const double was = column.label->box().h;

    column.label->set_text(paragraph(4));

    EXPECT_TRUE(column.canvas.root().settle()) << "a capped label that grew to its cap asked for no layout";
    EXPECT_DOUBLE_EQ(column.label->box().h, 2 * line(400, ttk::Theme::fontBody));
    EXPECT_GT(column.label->box().h, was);

    column.label->set_text(paragraph(8));

    EXPECT_FALSE(column.canvas.root().settle()) << "a capped label past its cap asked for a layout";
    EXPECT_TRUE(column.canvas.root().dirty());

    column.canvas.frame();

    EXPECT_TRUE(column.canvas.matches_full());
}

TEST_F(Painted, LabelLinkIsHitOnlyOverItsText) {
    auto made = std::make_unique<ttk::Label>("Open");
    int clicks = 0;

    made->on_click([&clicks] { ++clicks; });

    const double reach = made->natural_width(type());
    const Mounted mounted(std::move(made));

    EXPECT_EQ(mounted.label->cursor, ttk::Cursor::Pointer);
    EXPECT_EQ(mounted.label->at(BOX.x + 1, BOX.y + 10), mounted.label);
    EXPECT_EQ(mounted.label->at(BOX.x + reach - 1, BOX.y + 10), mounted.label);
    EXPECT_EQ(mounted.label->at(BOX.x + reach + 1, BOX.y + 10), nullptr);
    EXPECT_EQ(mounted.label->at(BOX.x + 1, BOX.y + BOX.h + 1), nullptr);
}

TEST_F(Painted, LabelLinkClickOverTheTextCalls) {
    auto made = std::make_unique<ttk::Label>("Open");
    int clicks = 0;

    made->on_click([&clicks] { ++clicks; });

    Mounted mounted(std::move(made));

    widgets::click(mounted.canvas, BOX.x + 5, BOX.y + 30);

    EXPECT_EQ(clicks, 1);

    widgets::click(mounted.canvas, BOX.x + 250, BOX.y + 30);

    EXPECT_EQ(clicks, 1) << "a click in the box but past the text went through";
}

TEST_F(Painted, LabelLinkReleasePastTheTextDoesNotCall) {
    auto made = std::make_unique<ttk::Label>("Open");
    int clicks = 0;

    made->on_click([&clicks] { ++clicks; });

    Mounted mounted(std::move(made));

    mounted.canvas.root().press(ttk::Pointer{.x = BOX.x + 5, .y = BOX.y + 30});

    EXPECT_EQ(mounted.canvas.root().grabbed(), mounted.label);

    mounted.canvas.root().release(ttk::Pointer{.x = BOX.x + 250, .y = BOX.y + 30});

    EXPECT_EQ(clicks, 0);
}

TEST_F(Painted, LabelLinkHitFollowsTheAlignment) {
    auto made = std::make_unique<ttk::Label>("Open");

    made->on_click([] {})->align(ttk::Align::End);

    const Mounted mounted(std::move(made));

    EXPECT_EQ(mounted.label->at(BOX.x + 5, BOX.y + 10), nullptr);
    EXPECT_EQ(mounted.label->at(BOX.x + BOX.w - 2, BOX.y + 10), mounted.label);
}

TEST_F(Painted, LabelCentredLinkIsHitAroundTheMiddle) {
    auto made = std::make_unique<ttk::Label>("Open");

    made->on_click([] {})->align(ttk::Align::Centre);

    const Mounted mounted(std::move(made));

    EXPECT_EQ(mounted.label->at(BOX.x + 5, BOX.y + 10), nullptr);
    EXPECT_EQ(mounted.label->at(BOX.x + (BOX.w / 2), BOX.y + 10), mounted.label);
    EXPECT_EQ(mounted.label->at(BOX.x + BOX.w - 2, BOX.y + 10), nullptr);
}

TEST_F(Painted, LabelLinkSetTextMovesTheHitArea) {
    auto made = std::make_unique<ttk::Label>("Go");

    made->on_click([] {});

    const Mounted mounted(std::move(made));

    EXPECT_EQ(mounted.label->at(BOX.x + 150, BOX.y + 10), nullptr);

    mounted.label->set_text("A much longer link text that reaches across");

    EXPECT_EQ(mounted.label->at(BOX.x + 150, BOX.y + 10), mounted.label);
}

TEST_F(Painted, LabelDisabledLinkIsNotHit) {
    auto made = std::make_unique<ttk::Label>("Open");
    int clicks = 0;

    made->on_click([&clicks] { ++clicks; });

    Mounted mounted(std::move(made));

    mounted.label->set_enabled(false);

    EXPECT_EQ(mounted.label->at(BOX.x + 5, BOX.y + 10), nullptr);

    widgets::click(mounted.canvas, BOX.x + 5, BOX.y + 30);

    EXPECT_EQ(clicks, 0);
}

TEST_F(Painted, LabelHiddenLinkIsNotHit) {
    auto made = std::make_unique<ttk::Label>("Open");

    made->on_click([] {});

    const Mounted mounted(std::move(made));

    mounted.label->set_visible(false);

    EXPECT_EQ(mounted.label->at(BOX.x + 5, BOX.y + 10), nullptr);
}

TEST_F(Painted, LabelLinkTurnsTheAccentColourOnHover) {
    auto made = big("HH");

    made->on_click([] {});

    Mounted mounted(std::move(made));

    EXPECT_GT(widgets::count(mounted.canvas, PIXELS, widgets::palette().text), 0U);

    widgets::hover(mounted.canvas, BOX.x + 10, BOX.y + 30);

    const std::size_t painted = mounted.canvas.frame();

    EXPECT_TRUE(mounted.label->hovered());
    EXPECT_LE(painted, widgets::area(BOX));
    EXPECT_GT(widgets::count(mounted.canvas, PIXELS, widgets::palette().accent), 0U);
    EXPECT_EQ(widgets::count(mounted.canvas, PIXELS, widgets::palette().text), 0U);
    EXPECT_TRUE(mounted.canvas.matches_full());

    widgets::hover(mounted.canvas, 390, 230);
    mounted.canvas.frame();

    EXPECT_FALSE(mounted.label->hovered());
    EXPECT_EQ(widgets::count(mounted.canvas, PIXELS, widgets::palette().accent), 0U);
    EXPECT_TRUE(mounted.canvas.matches_full());
}

TEST_F(Painted, LabelPlainLabelDoesNotLightUnderAHover) {
    Mounted mounted(big("HH"));
    const std::vector<std::uint32_t> before = mounted.canvas.pixels();

    mounted.label->enter();
    mounted.canvas.frame();

    EXPECT_TRUE(mounted.label->hovered());
    EXPECT_EQ(mounted.canvas.pixels(), before);

    mounted.label->leave();

    EXPECT_FALSE(mounted.label->hovered());
}

TEST_F(Painted, LabelPathMeasuresTheWholePathInTheFixedFace) {
    const std::string path = ttk::Env::get("HOME") + "/games/doom/wads/eviternity.wad";
    ttk::Label label(path);

    label.path();

    EXPECT_DOUBLE_EQ(label.natural_width(type()), width(ttk::Typeface::mono, ttk::Theme::fontBody, path));
}

TEST_F(Painted, LabelPathWritesTheHomeAsTilde) {
    const std::string home = ttk::Env::get("HOME");

    ASSERT_FALSE(home.empty());

    auto made = std::make_unique<ttk::Label>(home + "/games/file.wad");

    made->path();

    const Mounted path(std::move(made));

    auto shown = std::make_unique<ttk::Label>("~/games/file.wad");

    shown->mono();

    const Mounted tilde(std::move(shown));

    EXPECT_EQ(path.canvas.pixels(), tilde.canvas.pixels());
}

TEST_F(Painted, LabelPathIsShortenedToFitItsBox) {
    constexpr BLRect narrow{20, 20, 120, 30};
    auto made = std::make_unique<ttk::Label>("/srv/a/very/deep/tree/of/folders/holding/the/file.wad");

    made->path();

    const Mounted mounted(std::move(made), narrow);
    const widgets::Ink drawn = mounted.drawn(widgets::pixels_of(narrow));

    EXPECT_GT(drawn.pixels, 0U);
    EXPECT_EQ(widgets::outside(mounted.canvas, widgets::pixels_of(narrow)), 0U);
}

TEST_F(Painted, LabelPathRefitsWhenTheTextChanges) {
    constexpr BLRect narrow{20, 20, 200, 30};
    auto made = std::make_unique<ttk::Label>("/srv/first/file.wad");

    made->path();

    Mounted mounted(std::move(made), narrow);
    const std::vector<std::uint32_t> before = mounted.canvas.pixels();

    mounted.label->set_text("/srv/second/other.wad");

    EXPECT_LE(mounted.canvas.frame(), widgets::area(narrow));
    EXPECT_NE(mounted.canvas.pixels(), before);
    EXPECT_TRUE(mounted.canvas.matches_full());
}
}
