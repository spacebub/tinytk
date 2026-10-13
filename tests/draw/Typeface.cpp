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
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#ifdef __GLIBC__
#include <malloc.h>
#endif

#include "draw/Pixels.h"
#include "support/Canvas.h"
#include "ttk/draw/Typeface.h"

namespace {
constexpr std::uint32_t GROUND = 0xff000000U;
constexpr std::string_view ELLIPSIS = "\xe2\x80\xa6";

bool continuation(const std::string_view run, const std::size_t at) {
    return at < run.size() && (static_cast<unsigned char>(run[at]) & 0xc0U) == 0x80U;
}

std::vector<std::size_t> boundaries(const std::string_view run) {
    std::vector<std::size_t> cuts;

    for (std::size_t at = 0; at <= run.size(); ++at) {
        if (!continuation(run, at)) {
            cuts.push_back(at);
        }
    }

    return cuts;
}

TEST(Typeface, PickPassesProportionalWeightsThrough) {
    for (const int weight : {ttk::Typeface::regular, ttk::Typeface::semibold, ttk::Typeface::bold, 100, 900}) {
        EXPECT_EQ(ttk::Typeface::pick(weight, false), weight);
    }
}

TEST(Typeface, PickMapsFixedWeightsToMonospace) {
    EXPECT_EQ(ttk::Typeface::pick(ttk::Typeface::regular, true), ttk::Typeface::mono);
    EXPECT_EQ(ttk::Typeface::pick(ttk::Typeface::semibold - 1, true), ttk::Typeface::mono);
    EXPECT_EQ(ttk::Typeface::pick(ttk::Typeface::semibold, true), ttk::Typeface::monoBold);
    EXPECT_EQ(ttk::Typeface::pick(ttk::Typeface::bold, true), ttk::Typeface::monoBold);
    EXPECT_EQ(ttk::Typeface::pick(0, true), ttk::Typeface::mono);
}

TEST(Typeface, FontBeforeLoadHasNoFaceAndStaysCached) {
    ttk::Typeface type;
    const BLFont &early = type.at(ttk::Typeface::regular, 14);

    EXPECT_FALSE(early.is_valid());

    if (!type.load()) {
        GTEST_SKIP() << "no font installed";
    }

    EXPECT_EQ(&type.at(ttk::Typeface::regular, 14), &early) << "the font asked for before load moved";
    EXPECT_FALSE(type.at(ttk::Typeface::regular, 14).is_valid());
    EXPECT_TRUE(type.at(ttk::Typeface::bold, 14).is_valid());
}

TEST(Typeface, WeightAskedForBeforeLoadStillLoads) {
    ttk::Typeface type;

    type.at(ttk::Typeface::regular, 14);

    if (!type.load()) {
        GTEST_SKIP() << "no font installed";
    }

    EXPECT_TRUE(type.at(ttk::Typeface::regular, 15).is_valid());
}

TEST(Typeface, AskingBeforeLoadLeavesWhatLoadReports) {
    ttk::Typeface asked;
    ttk::Typeface fresh;

    asked.at(500, 14);
    asked.at(ttk::Typeface::mono, 14);

    EXPECT_EQ(asked.load(), fresh.load());
    EXPECT_FALSE(asked.at(500, 15).is_valid());
}

TEST(Typeface, EmptyRunsMeasureZeroWithoutAFace) {
    ttk::Typeface type;
    const BLFont &font = type.at(ttk::Typeface::regular, 14);

    EXPECT_EQ(type.width(font, ""), 0.0F);
    EXPECT_EQ(type.width_once(font, ""), 0.0F);
    EXPECT_EQ(type.width_word(font, ""), 0.0F);
    EXPECT_EQ(type.width_tracked(font, "", 2.0F), 0.0F);
    EXPECT_EQ(type.nearest(font, "", 10.0F), 0U);
}

class Typed : public ::testing::Test {
protected:
    void SetUp() override {
        if (!tests::fonts_loaded()) {
            GTEST_SKIP() << "no font installed";
        }
    }

    static ttk::Typeface &type() { return tests::fonts(); }

    static const BLFont &body() { return tests::fonts().at(ttk::Typeface::regular, 14); }
};

TEST_F(Typed, LoadGivesEveryWeightAFace) {
    ttk::Typeface fresh;

    ASSERT_TRUE(fresh.load());

    for (const int weight : {
                 ttk::Typeface::regular,
                 ttk::Typeface::semibold,
                 ttk::Typeface::bold,
                 ttk::Typeface::mono,
                 ttk::Typeface::monoBold,
         }) {
        EXPECT_TRUE(fresh.at(weight, 12).is_valid()) << "weight " << weight;
    }
}

TEST_F(Typed, LoadTwiceStillLoads) {
    ttk::Typeface fresh;

    ASSERT_TRUE(fresh.load());
    EXPECT_TRUE(fresh.load());
    EXPECT_TRUE(fresh.at(ttk::Typeface::regular, 12).is_valid());
}

TEST_F(Typed, UnknownWeightGivesAFontWithNoFace) {
    ttk::Typeface fresh;

    ASSERT_TRUE(fresh.load());

    EXPECT_FALSE(fresh.at(500, 12).is_valid());
    EXPECT_EQ(&fresh.at(500, 12), &fresh.at(500, 12));
}

TEST_F(Typed, AtKeepsAddressesAndSizes) {
    ttk::Typeface fresh;

    ASSERT_TRUE(fresh.load());

    const BLFont &first = fresh.at(ttk::Typeface::regular, 13);

    EXPECT_FLOAT_EQ(first.size(), 13.0F);

    for (int size = 6; size < 80; ++size) {
        fresh.at(ttk::Typeface::bold, static_cast<float>(size));
        fresh.at(ttk::Typeface::mono, static_cast<float>(size) + 0.5F);
    }

    EXPECT_EQ(&fresh.at(ttk::Typeface::regular, 13), &first) << "a font moved when others were made";
    EXPECT_NE(&fresh.at(ttk::Typeface::bold, 13), &first);
    EXPECT_FLOAT_EQ(fresh.at(ttk::Typeface::mono, 20.5F).size(), 20.5F);
}

TEST_F(Typed, AtTellsSizesApartToAQuarterPixel) {
    const BLFont &base = type().at(ttk::Typeface::regular, 12.0F);

    EXPECT_EQ(&type().at(ttk::Typeface::regular, 12.1F), &base);
    EXPECT_NE(&type().at(ttk::Typeface::regular, 12.25F), &base);
    EXPECT_NE(&type().at(ttk::Typeface::regular, 11.75F), &base);
}

TEST_F(Typed, WidthIsZeroForEmptyAndPositiveForText) {
    EXPECT_EQ(type().width(body(), ""), 0.0F);
    EXPECT_GT(type().width(body(), "Hello"), 0.0F);
    EXPECT_GT(type().width(body(), " "), 0.0F);
}

TEST_F(Typed, WidthIsTheSameTwice) {
    const std::string run = "Repeatable measurement, й é ü";
    const float first = type().width(body(), run);
    const std::string copy = std::string("Repeatable ") + "measurement, й é ü";

    EXPECT_EQ(type().width(body(), run), first);
    EXPECT_EQ(type().width(body(), copy), first) << "a different buffer with the same text";
}

TEST_F(Typed, WidthGrowsWithThePrefix) {
    const std::string run = "The quick brown fox jumps over the lazy dog, éй ü 123";
    float last = 0.0F;

    for (const std::size_t cut : boundaries(run)) {
        const float wide = type().width(body(), std::string_view(run).substr(0, cut));

        EXPECT_GE(wide, last) << "prefix of " << cut << " bytes";
        last = wide;
    }
}

TEST_F(Typed, WidthGrowsWithSize) {
    const float small = type().width(type().at(ttk::Typeface::regular, 10), "Scale");
    const float large = type().width(type().at(ttk::Typeface::regular, 20), "Scale");

    EXPECT_NEAR(large, small * 2.0F, 1.0F);
}

TEST_F(Typed, WidthsAgreeAcrossTheirCaches) {
    for (const std::string_view run : {"word", "Hello, world", "наслов", "e\xcc\x81t\xc3\xa9"}) {
        const float shaped = type().width(body(), run);

        EXPECT_FLOAT_EQ(type().width_once(body(), run), shaped) << run;
        EXPECT_FLOAT_EQ(type().width_word(body(), run), shaped) << run;
        EXPECT_FLOAT_EQ(type().width_word(body(), run), shaped) << "second ask of " << run;
    }
}

TEST_F(Typed, CombiningMarkAddsNoAdvance) {
    const float plain = type().width(body(), "e");
    const float marked = type().width(body(), "e\xcc\x81");

    EXPECT_NEAR(marked, plain, 0.5F);
    EXPECT_LT(marked, type().width(body(), "ee"));
}

TEST_F(Typed, CombiningMarkMatchesThePrecomposedWidth) {
    EXPECT_NEAR(type().width(body(), "e\xcc\x81"), type().width(body(), "\xc3\xa9"), 0.5F);
}

TEST_F(Typed, TrackingKeepsAMarkOnItsBase) {
    EXPECT_FLOAT_EQ(type().width_tracked(body(), "e\xcc\x81", 2.0F), type().width_tracked(body(), "e", 2.0F))
            << "a mark is not a letter of its own to space out";
}

TEST_F(Typed, TrackedWidthAddsTrackingBetweenCharacters) {
    const std::string_view run = "abcй";
    float parts = 0.0F;

    for (const std::string_view one : {"a", "b", "c", "й"}) {
        parts += type().width(body(), one);
    }

    EXPECT_FLOAT_EQ(type().width_tracked(body(), run, 0.0F), parts);
    EXPECT_NEAR(type().width_tracked(body(), run, 2.5F), parts + 7.5F, 1e-3F) << "tracking after the last counted";
    EXPECT_FLOAT_EQ(type().width_tracked(body(), "a", 10.0F), type().width(body(), "a"));
    EXPECT_EQ(type().width_tracked(body(), "", 3.0F), 0.0F);
}

TEST_F(Typed, ElideKeepsWhatFits) {
    const std::string_view run = "Short";
    const float wide = type().width(body(), run);

    EXPECT_EQ(type().elide(body(), run, wide + 1.0F), run);
    EXPECT_EQ(type().elide(body(), run, wide), run) << "a run exactly as wide as the room was cut";
    EXPECT_EQ(type().elide(body(), "", 0.0F), "");
}

TEST_F(Typed, ElideCutsToAPrefixAndAnEllipsis) {
    const std::string run = "A long label that cannot possibly fit in the room it is given";

    for (const float room : {30.0F, 55.5F, 80.0F, 120.25F, 200.0F}) {
        const std::string cut = type().elide(body(), run, room);

        ASSERT_GE(cut.size(), ELLIPSIS.size()) << "room " << room;
        EXPECT_TRUE(cut.ends_with(ELLIPSIS)) << cut;
        EXPECT_TRUE(run.starts_with(std::string_view(cut).substr(0, cut.size() - ELLIPSIS.size()))) << cut;
        EXPECT_LE(type().width(body(), cut), room) << cut;

        const std::size_t kept = cut.size() - ELLIPSIS.size();
        const std::string longer = run.substr(0, kept + 1) + std::string(ELLIPSIS);

        EXPECT_GT(type().width(body(), longer), room) << "a longer prefix would have fitted: " << cut;
    }
}

TEST_F(Typed, ElideCutsOnCharacterBoundaries) {
    const std::string run = "ййййййййййййййййййййééééééé漢字漢字漢字";

    for (int step = 0; 8.0F + (static_cast<float>(step) * 3.0F) < type().width(body(), run); ++step) {
        const float room = 8.0F + (static_cast<float>(step) * 3.0F);
        const std::string cut = type().elide(body(), run, room);

        if (cut.empty()) {
            continue;
        }

        const std::size_t kept = cut.size() - ELLIPSIS.size();

        EXPECT_FALSE(continuation(run, kept)) << "cut inside a character at room " << room;
    }
}

TEST_F(Typed, ElideGivesNothingWhenTheEllipsisDoesNotFit) {
    const float dots = type().width(body(), ELLIPSIS);

    EXPECT_EQ(type().elide(body(), "Anything at all", dots * 0.5F), "");
    EXPECT_EQ(type().elide(body(), "Anything at all", 0.0F), "");
    EXPECT_EQ(type().elide(body(), "Anything at all", -5.0F), "");
    EXPECT_EQ(type().elide(body(), "Anything at all", dots), ELLIPSIS) << "only the ellipsis fitted";
}

TEST_F(Typed, ElideAnswersFromTheCache) {
    const std::string &first = type().elide(body(), "Cached elision of a long run", 60.0F);
    const std::string &second = type().elide(body(), "Cached elision of a long run", 60.0F);

    EXPECT_EQ(&first, &second);
    EXPECT_NE(&type().elide(body(), "Cached elision of a long run", 61.0F), &first) << "room not in the key";
}

TEST_F(Typed, ElideWithTrackingMeasuresTracked) {
    const std::string run = "Tracked capitals";
    const float room = type().width(body(), run);
    const std::string cut = type().elide(body(), run, room, 2.0F);

    EXPECT_TRUE(cut.ends_with(ELLIPSIS)) << "tracking did not widen the run past its room";
    EXPECT_LE(type().width_tracked(body(), cut, 2.0F), room);
    EXPECT_EQ(type().elide(body(), run, room, 0.0F), run);
}

TEST_F(Typed, NearestIsZeroForEmptyAndAtTheStart) {
    EXPECT_EQ(type().nearest(body(), "", 0.0F), 0U);
    EXPECT_EQ(type().nearest(body(), "", 100.0F), 0U);
    EXPECT_EQ(type().nearest(body(), "Hello", 0.0F), 0U);
    EXPECT_EQ(type().nearest(body(), "Hello", -50.0F), 0U);
}

TEST_F(Typed, NearestIsTheSizePastTheEnd) {
    const std::string_view run = "Hello й";

    EXPECT_EQ(type().nearest(body(), run, type().width(body(), run)), run.size());
    EXPECT_EQ(type().nearest(body(), run, 1e6F), run.size());
}

TEST_F(Typed, NearestRoundTripsEveryBoundary) {
    for (const std::string_view run : {"Hello, world", "mono spaced 0123", "Ünïcödé й漢字"}) {
        for (const std::size_t cut : boundaries(run)) {
            const float x = type().width(body(), run.substr(0, cut));

            EXPECT_EQ(type().nearest(body(), run, x), cut) << run << " at " << cut;
        }
    }
}

TEST_F(Typed, NearestPicksTheCloserSide) {
    const std::string_view run = "WW";
    const float one = type().width(body(), "W");

    EXPECT_EQ(type().nearest(body(), run, one * 0.4F), 0U);
    EXPECT_EQ(type().nearest(body(), run, one * 0.6F), 1U);
    EXPECT_EQ(type().nearest(body(), run, one * 1.6F), 2U);
}

TEST_F(Typed, NearestNeverSplitsACharacter) {
    const std::string run = "e\xcc\x81 йж 漢字 a\xcc\x88o";
    const float wide = type().width(body(), run);

    for (int step = 0; static_cast<float>(step) * 0.37F < wide + 6.0F; ++step) {
        const float x = -2.0F + (static_cast<float>(step) * 0.37F);
        const std::size_t at = type().nearest(body(), run, x);

        ASSERT_LE(at, run.size());
        EXPECT_FALSE(continuation(run, at)) << "offset " << at << " at x " << x;
    }
}

TEST_F(Typed, LineHeightIsAscentPlusDescent) {
    const BLFontMetrics metrics = body().metrics();

    EXPECT_FLOAT_EQ(type().line_height(body()), metrics.ascent + metrics.descent);
    EXPECT_GT(type().line_height(body()), 14.0F * 0.8F);
}

TEST_F(Typed, LineHeightScalesWithSize) {
    const float small = type().line_height(type().at(ttk::Typeface::regular, 12));
    const float large = type().line_height(type().at(ttk::Typeface::regular, 24));

    EXPECT_NEAR(large, small * 2.0F, 0.5F);
}

TEST_F(Typed, BleedLeavesAtLeastThePixelOfRoom) {
    for (int size = 6; size <= 64; size += 2) {
        for (const int weight : {ttk::Typeface::regular, ttk::Typeface::bold, ttk::Typeface::mono}) {
            EXPECT_GE(ttk::Typeface::bleed(type().at(weight, static_cast<float>(size))), 1.0)
                    << "weight " << weight << " size " << size;
        }
    }
}

TEST_F(Typed, BleedStaysInProportionAcrossSizes) {
    const double large = ttk::Typeface::bleed(type().at(ttk::Typeface::regular, 64));
    const double rate = (large - 2.5) / 64.0;

    for (int size = 6; size <= 64; ++size) {
        const double bleed = ttk::Typeface::bleed(type().at(ttk::Typeface::regular, static_cast<float>(size)));

        EXPECT_LE(bleed, 2.5 + (std::max(rate, 0.0) * size) + 1.0) << "size " << size;
    }
}

TEST_F(Typed, BleedDoesNotShrinkAsTheFontGrows) {
    double last = 0.0;

    for (int size = 6; size <= 64; ++size) {
        const double bleed = ttk::Typeface::bleed(type().at(ttk::Typeface::regular, static_cast<float>(size)));

        EXPECT_GE(bleed, last - 1.0) << "size " << size;
        last = bleed;
    }
}

class Drawn : public Typed {
protected:
    static constexpr int WIDE = 320;
    static constexpr int TALL = 80;

    void SetUp() override {
        Typed::SetUp();

        if (IsSkipped()) {
            return;
        }

        ASSERT_EQ(image.create(WIDE, TALL, BL_FORMAT_PRGB32), BL_SUCCESS);
        context.begin(image);
        context.fill_all(BLRgba32(GROUND));
    }

    void TearDown() override { context.end(); }

    tests::Ink settle() {
        context.flush(BL_CONTEXT_FLUSH_SYNC);

        return tests::ink(image, GROUND);
    }

    // Whether `found` lies inside the line box at `top`, `wide` across, padded by the font's bleed.
    static ::testing::AssertionResult
    boxed(const tests::Ink &found, const BLFont &font, const BLPoint top, const double wide) {
        const double pad = ttk::Typeface::bleed(font);
        const double tall = type().line_height(font);

        if (found.x0 < top.x - pad || found.y0 < top.y - pad || found.x1 > top.x + wide + pad
            || found.y1 > top.y + tall + pad) {
            return ::testing::AssertionFailure() << "ink [" << found.x0 << "," << found.y0 << " .. " << found.x1 << ","
                                                 << found.y1 << ") outside [" << top.x - pad << "," << top.y - pad
                                                 << " .. " << top.x + wide + pad << "," << top.y + tall + pad << ")";
        }

        return ::testing::AssertionSuccess();
    }

    BLImage image;
    BLContext context;
};

TEST_F(Drawn, DrawInksInsideTheLineBox) {
    const BLPoint top{20.0, 18.0};
    const std::string_view run = "Hojgy ÉÅ й|";

    type().draw(context, body(), top, run, BLRgba32(0xffffffff));

    const tests::Ink found = settle();

    ASSERT_TRUE(found.any()) << "nothing was drawn";
    EXPECT_TRUE(boxed(found, body(), top, type().width(body(), run)));
}

TEST_F(Drawn, DrawInksInsideTheLineBoxAtEverySize) {
    for (const float size : {8.0F, 11.5F, 14.0F, 19.0F, 30.0F, 44.0F}) {
        context.fill_all(BLRgba32(GROUND));

        const BLFont &font = type().at(ttk::Typeface::bold, size);
        const BLPoint top{10.25, 12.5};
        const std::string_view run = "ÅÉjgpqy|";

        type().draw(context, font, top, run, BLRgba32(0xffffffff));

        const tests::Ink found = settle();

        ASSERT_TRUE(found.any()) << "size " << size;
        EXPECT_TRUE(boxed(found, font, top, type().width(font, run))) << "size " << size;
    }
}

TEST_F(Drawn, DrawUnderAScaleStaysInsideTheLineBox) {
    const BLPoint top{10.0, 10.0};
    const std::string_view run = "Scaled Éjy";

    context.save();
    context.scale(1.5);
    type().draw(context, body(), top, run, BLRgba32(0xffffffff));
    context.restore();

    const tests::Ink found = settle();
    const double pad = ttk::Typeface::bleed(body()) * 1.5;

    ASSERT_TRUE(found.any());
    EXPECT_GE(found.y0, (top.y * 1.5) - pad);
    EXPECT_LE(found.y1, ((top.y + type().line_height(body())) * 1.5) + pad);
}

TEST_F(Drawn, DrawOfNothingDrawsNothing) {
    type().draw(context, body(), {10, 10}, "", BLRgba32(0xffffffff));
    type().draw_centred(context, body(), {10, 10}, 30.0F, "", BLRgba32(0xffffffff));
    type().draw_once(context, body(), {10, 10}, "", BLRgba32(0xffffffff), 100.0);
    type().draw_once(context, body(), {10, 10}, "Text", BLRgba32(0xffffffff), 0.0);
    type().draw_once(context, body(), {10, 10}, "Text", BLRgba32(0xffffffff), -3.0);

    EXPECT_FALSE(settle().any());
}

TEST_F(Drawn, DrawTwiceGivesTheSamePixels) {
    type().draw(context, body(), {12.0, 9.0}, "Twice drawn", BLRgba32(0xffffffff));
    settle();

    const std::vector<std::uint32_t> first = tests::pixels(image);

    context.fill_all(BLRgba32(GROUND));
    type().draw(context, body(), {12.0, 9.0}, "Twice drawn", BLRgba32(0xffffffff));
    settle();

    EXPECT_EQ(tests::pixels(image), first);
}

TEST_F(Drawn, DrawRoundsTheOriginToAWholePixel) {
    type().draw(context, body(), {30.0, 10.0}, "Round", BLRgba32(0xffffffff));
    settle();

    const std::vector<std::uint32_t> whole = tests::pixels(image);

    context.fill_all(BLRgba32(GROUND));
    type().draw(context, body(), {30.2, 10.3}, "Round", BLRgba32(0xffffffff));
    settle();

    EXPECT_EQ(tests::pixels(image), whole) << "a fraction of a pixel moved the masked run";
}

TEST_F(Drawn, DrawUsesTheTone) {
    type().draw(context, type().at(ttk::Typeface::bold, 30), {10, 10}, "I", BLRgba32(0xff00ff00));

    const tests::Ink found = settle();

    ASSERT_TRUE(found.any());

    bool solid = false;

    for (int y = found.y0; y < found.y1; ++y) {
        for (int x = found.x0; x < found.x1; ++x) {
            const std::uint32_t at = tests::pixel(image, x, y);

            EXPECT_EQ(at & 0xff00ffU, 0U) << "ink not in the tone at " << x << "," << y;
            solid = solid || at == 0xff00ff00U;
        }
    }

    EXPECT_TRUE(solid) << "a thick stem had no fully covered pixel";
}

TEST_F(Drawn, DrawCentredCentresTheLineBox) {
    const BLFont &font = type().at(ttk::Typeface::regular, 20);
    const BLPoint top{10.0, 5.0};
    constexpr float box = 60.0F;

    type().draw_centred(context, font, top, box, "Hxj", BLRgba32(0xffffffff));

    const tests::Ink found = settle();
    const double line = type().line_height(font);
    const double pad = ttk::Typeface::bleed(font);
    const double centred = top.y + ((box - line) * 0.5);

    ASSERT_TRUE(found.any());
    EXPECT_GE(found.y0, centred - pad);
    EXPECT_LE(found.y1, centred + line + pad);
    EXPECT_GT(found.y0, top.y + 5.0) << "the run sat at the top of its box";
}

TEST_F(Drawn, DrawTrackedSpreadsTheRun) {
    type().draw_tracked(context, body(), {10, 10}, "spread", BLRgba32(0xffffffff), 0.0F);

    const tests::Ink tight = settle();

    context.fill_all(BLRgba32(GROUND));
    type().draw_tracked(context, body(), {10, 10}, "spread", BLRgba32(0xffffffff), 6.0F);

    const tests::Ink loose = settle();

    ASSERT_TRUE(tight.any());
    EXPECT_EQ(loose.x0, tight.x0);
    EXPECT_NEAR(loose.x1 - tight.x1, 30, 2) << "five gaps of six pixels";
    EXPECT_TRUE(boxed(loose, body(), {10, 10}, type().width_tracked(body(), "spread", 6.0F)));
}

TEST_F(Drawn, DrawOnceInksInsideTheLineBox) {
    const BLPoint top{15.0, 20.0};
    const std::string_view run = "Logged line Éjy й";

    type().draw_once(context, body(), top, run, BLRgba32(0xffffffff), 1000.0);

    const tests::Ink found = settle();

    ASSERT_TRUE(found.any());
    EXPECT_TRUE(boxed(found, body(), top, type().width(body(), run)));
}

TEST_F(Drawn, DrawOnceMatchesDrawWhereItFits) {
    const std::string_view run = "Same ink";

    type().draw(context, body(), {10, 10}, run, BLRgba32(0xffffffff));

    const tests::Ink drawn = settle();

    context.fill_all(BLRgba32(GROUND));
    type().draw_once(context, body(), {10, 10}, run, BLRgba32(0xffffffff), 500.0);

    const tests::Ink once = settle();

    ASSERT_TRUE(once.any());
    EXPECT_NEAR(once.x0, drawn.x0, 1);
    EXPECT_NEAR(once.x1, drawn.x1, 1);
    EXPECT_NEAR(once.y0, drawn.y0, 1);
    EXPECT_NEAR(once.y1, drawn.y1, 1);
}

TEST_F(Drawn, DrawOnceStaysWithinItsRoom) {
    const BLPoint top{10.0, 20.0};
    const std::string_view run = "This log line is far too long for the room it is drawn into";

    for (const double room : {20.0, 64.5, 100.0, 150.0}) {
        context.fill_all(BLRgba32(GROUND));
        type().draw_once(context, body(), top, run, BLRgba32(0xffffffff), room);

        const tests::Ink found = settle();

        ASSERT_TRUE(found.any()) << "room " << room;
        EXPECT_TRUE(boxed(found, body(), top, room)) << "room " << room;
    }
}

TEST_F(Drawn, DrawOnceUnderAScaleStaysWithinItsRoom) {
    const std::string_view run = "Scaled log line far too long for its room";

    context.save();
    context.scale(2.0);
    type().draw_once(context, body(), {5, 5}, run, BLRgba32(0xffffffff), 60.0);
    context.restore();

    const tests::Ink found = settle();

    ASSERT_TRUE(found.any());
    EXPECT_LE(found.x1, static_cast<int>((5.0 + 60.0 + ttk::Typeface::bleed(body())) * 2.0));
}

TEST_F(Drawn, DrawOnceTwiceGivesTheSamePixels) {
    type().draw_once(context, body(), {12, 9}, "Twice composed", BLRgba32(0xffffffff), 90.0);
    settle();

    const std::vector<std::uint32_t> first = tests::pixels(image);

    context.fill_all(BLRgba32(GROUND));
    type().draw_once(context, body(), {12, 9}, "Twice composed", BLRgba32(0xffffffff), 90.0);
    settle();

    EXPECT_EQ(tests::pixels(image), first);
}

TEST_F(Drawn, CombiningMarksDrawInsideTheLineBox) {
    const BLPoint top{20.0, 20.0};
    const std::string_view run = "A\xcc\x8a e\xcc\x81 o\xcc\x88\xcc\x81 g\xcc\xa7";

    type().draw(context, body(), top, run, BLRgba32(0xffffffff));

    const tests::Ink found = settle();

    ASSERT_TRUE(found.any());
    EXPECT_TRUE(boxed(found, body(), top, type().width(body(), run)));
}

TEST_F(Drawn, CombiningMarkSitsOverItsBase) {
    const BLPoint top{20.0, 20.0};

    type().draw(context, body(), top, "e", BLRgba32(0xffffffff));

    const tests::Ink plain = settle();

    context.fill_all(BLRgba32(GROUND));
    type().draw(context, body(), top, "e\xcc\x81", BLRgba32(0xffffffff));

    const tests::Ink marked = settle();

    ASSERT_TRUE(plain.any());
    ASSERT_TRUE(marked.any());
    EXPECT_LT(marked.y0, plain.y0) << "the accent stands above the e";
    EXPECT_GE(marked.x0, plain.x0 - 1) << "not beside it";
    EXPECT_LE(marked.x1, plain.x1 + 1) << "not beside it";
}

#ifdef __GLIBC__
std::size_t held() {
    return mallinfo2().uordblks;
}

std::string line(const int at) {
    static constexpr std::string_view words[] = {"alpha", "beta", "gamma", "delta", "й", "é", "log", "0x1f"};
    std::string made = std::to_string(at);

    for (int word = 0; word < 6; ++word) {
        made += ' ';
        made += words[((static_cast<std::size_t>(at) * 31U) + (static_cast<std::size_t>(word) * 7U))
                      % std::size(words)];
    }

    return made;
}

TEST_F(Drawn, ShapedRunsDoNotGrowWithoutBound) {
    ttk::Typeface fresh;

    ASSERT_TRUE(fresh.load());

    const BLFont &font = fresh.at(ttk::Typeface::regular, 12);
    const auto pass = [&](const int from) {
        for (int at = from; at < from + 6000; ++at) {
            fresh.draw(context, font, {4, 4}, line(at), BLRgba32(0xffffffff));
        }
    };

    pass(0);

    const std::size_t warm = held();

    pass(6000);
    pass(12000);

    EXPECT_LT(held(), warm + (warm / 2) + (std::size_t{4} << 20U)) << "shaped runs kept growing";
}

TEST_F(Drawn, ComposedLinesDoNotGrowWithoutBound) {
    ttk::Typeface fresh;

    ASSERT_TRUE(fresh.load());

    const BLFont &font = fresh.at(ttk::Typeface::mono, 12);
    const auto pass = [&](const int from) {
        for (int at = from; at < from + 2000; ++at) {
            fresh.draw_once(context, font, {4, 4}, line(at), BLRgba32(0xffffffff), 300.0);
        }
    };

    pass(0);

    const std::size_t warm = held();

    pass(2000);
    pass(4000);

    EXPECT_LT(held(), warm + (std::size_t{2} << 20U)) << "composed lines kept growing";
}

TEST_F(Drawn, ElisionsAndWordsDoNotGrowWithoutBound) {
    ttk::Typeface fresh;

    ASSERT_TRUE(fresh.load());

    const BLFont &font = fresh.at(ttk::Typeface::regular, 12);
    const auto pass = [&](const int from) {
        for (int at = from; at < from + 6000; ++at) {
            const std::string text = line(at);

            fresh.elide(font, text, 40.0F);
            fresh.width_word(font, text);
        }
    };

    pass(0);

    const std::size_t warm = held();

    pass(6000);
    pass(12000);

    EXPECT_LT(held(), warm + (warm / 2) + (std::size_t{4} << 20U)) << "elisions or words kept growing";
}
#endif
}
