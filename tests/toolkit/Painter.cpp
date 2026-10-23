// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "support/Canvas.h"
#include "ttk/toolkit/Painter.h"

namespace {
constexpr int WIDTH = 160;
constexpr int HEIGHT = 100;

constexpr BLRectI EVERYWHERE{0, 0, WIDTH, HEIGHT};

const BLRgba32 INK(0xff2060c0);
const BLRgba32 CLEAR(0x00ffffff);

constexpr std::string_view ELLIPSIS = "\xe2\x80\xa6";

struct Ink {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;

    [[nodiscard]] bool empty() const { return right <= left || bottom <= top; }
    [[nodiscard]] int width() const { return right - left; }
    [[nodiscard]] double middle_x() const { return (left + right) / 2.0; }
    [[nodiscard]] double middle_y() const { return (top + bottom) / 2.0; }
};

std::vector<std::uint32_t> pixels(const BLImage &image) {
    BLImageData data{};
    std::vector<std::uint32_t> copy(static_cast<std::size_t>(image.width()) * static_cast<std::size_t>(image.height()));

    if (image.get_data(&data) != BL_SUCCESS) {
        return copy;
    }

    for (int y = 0; y < image.height(); ++y) {
        const auto *row = static_cast<const std::uint32_t *>(data.pixel_data) + (y * data.stride / 4);

        std::copy_n(row, image.width(), copy.begin() + (static_cast<std::ptrdiff_t>(y) * image.width()));
    }

    return copy;
}

Ink ink(const BLImage &image) {
    const std::vector<std::uint32_t> all = pixels(image);
    Ink found{.left = image.width(), .top = image.height(), .right = 0, .bottom = 0};

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (all[(static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width()))
                    + static_cast<std::size_t>(x)]
                != 0) {
                found.left = std::min(found.left, x);
                found.top = std::min(found.top, y);
                found.right = std::max(found.right, x + 1);
                found.bottom = std::max(found.bottom, y + 1);
            }
        }
    }

    return found;
}

std::uint32_t pixel(const BLImage &image, const int x, const int y) {
    return pixels(image)[(static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width()))
                         + static_cast<std::size_t>(x)];
}

// Two analytic rasterisations of the same edge can round a channel a level or two apart.
bool near(const std::vector<std::uint32_t> &one, const std::vector<std::uint32_t> &two, const int levels) {
    return std::ranges::equal(one, two, [levels](const std::uint32_t a, const std::uint32_t b) {
        for (unsigned shift = 0; shift < 32; shift += 8) {
            if (std::abs(static_cast<int>((a >> shift) & 0xffU) - static_cast<int>((b >> shift) & 0xffU)) > levels) {
                return false;
            }
        }

        return true;
    });
}

int worst(const std::vector<std::uint32_t> &one, const std::vector<std::uint32_t> &two) {
    int most = 0;

    for (std::size_t at = 0; at < std::min(one.size(), two.size()); ++at) {
        for (unsigned shift = 0; shift < 32; shift += 8) {
            most = std::max(most,
                            std::abs(static_cast<int>((one[at] >> shift) & 0xffU)
                                     - static_cast<int>((two[at] >> shift) & 0xffU)));
        }
    }

    return most;
}

class Drawn : public ::testing::Test {
protected:
    BLImage mine{WIDTH, HEIGHT, BL_FORMAT_PRGB32};
    BLImage theirs{WIDTH, HEIGHT, BL_FORMAT_PRGB32};
    BLContext ours{mine};
    BLContext direct{theirs};
    ttk::Painter painter{ours, tests::fonts(), EVERYWHERE};

    void SetUp() override {
        ours.clear_all();
        direct.clear_all();
    }

    void finish() {
        ours.end();
        direct.end();
    }

    ::testing::AssertionResult same() {
        finish();

        if (pixels(mine) == pixels(theirs)) {
            return ::testing::AssertionSuccess();
        }

        return ::testing::AssertionFailure() << "the painter drew other pixels than Blend2D";
    }

    bool blank() {
        finish();

        return ink(mine).empty();
    }
};

class Written : public Drawn {
protected:
    void SetUp() override {
        if (!tests::fonts_loaded()) {
            GTEST_SKIP() << "no font installed";
        }

        Drawn::SetUp();
    }

    const BLFont &body() const { return painter.font(400, 14.0F); }
};

TEST_F(Drawn, FillMatchesBlend2D) {
    painter.fill({10, 20, 30, 40}, INK);
    direct.fill_rect(BLRect{10, 20, 30, 40}, INK);

    EXPECT_TRUE(same());
}

TEST_F(Drawn, FillOfAFractionalBoxMatchesBlend2D) {
    painter.fill({10.25, 20.5, 30.75, 9.1}, INK);
    direct.fill_rect(BLRect{10.25, 20.5, 30.75, 9.1}, INK);

    EXPECT_TRUE(same());
}

TEST_F(Drawn, TranslucentFillMatchesBlend2D) {
    painter.fill({0, 0, 50, 50}, INK);
    painter.fill({25, 25, 50, 50}, BLRgba32(0x80ff0000));
    direct.fill_rect(BLRect{0, 0, 50, 50}, INK);
    direct.fill_rect(BLRect{25, 25, 50, 50}, BLRgba32(0x80ff0000));

    EXPECT_TRUE(same());
}

TEST_F(Drawn, FillOutsideTheClipDrawsNothing) {
    const ttk::Painter narrow(ours, tests::fonts(), {0, 0, 20, 20});

    narrow.fill({30, 30, 20, 20}, INK);

    EXPECT_TRUE(blank()) << "a box the clip rejects was drawn";
}

TEST_F(Drawn, FillOfAnEmptyBoxDrawsNothing) {
    painter.fill({10, 10, 0, 20}, INK);
    painter.fill({10, 10, 20, -1}, INK);

    EXPECT_TRUE(blank());
}

TEST_F(Drawn, RoundMatchesBlend2D) {
    painter.round({10, 10, 60, 40}, 8, INK);
    direct.fill_round_rect(BLRect{10, 10, 60, 40}, 8, 8, INK);

    EXPECT_TRUE(same());
}

TEST_F(Drawn, RoundClampsTheRadiusToHalfTheShorterSide) {
    painter.round({10, 10, 60, 30}, 100, INK);
    direct.fill_round_rect(BLRect{10, 10, 60, 30}, 15, 15, INK);

    EXPECT_TRUE(same());
}

TEST_F(Drawn, RoundWithNoRadiusFillsAPlainRectangle) {
    painter.round({10, 10, 60, 30}, 0, INK);
    painter.round({80, 10, 60, 30}, -5, INK);
    direct.fill_rect(BLRect{10, 10, 60, 30}, INK);
    direct.fill_rect(BLRect{80, 10, 60, 30}, INK);

    EXPECT_TRUE(same());
}

TEST_F(Drawn, RoundOutsideTheClipDrawsNothing) {
    const ttk::Painter narrow(ours, tests::fonts(), {0, 0, 20, 20});

    narrow.round({30, 30, 20, 20}, 4, INK);

    EXPECT_TRUE(blank());
}

TEST_F(Drawn, SquareOutlineIsTheFourEdgesOfTheBox) {
    painter.outline({10, 10, 60, 40}, 0, 2, INK);
    direct.fill_rect(BLRect{10, 10, 60, 2}, INK);
    direct.fill_rect(BLRect{10, 48, 60, 2}, INK);
    direct.fill_rect(BLRect{10, 12, 2, 36}, INK);
    direct.fill_rect(BLRect{68, 12, 2, 36}, INK);

    EXPECT_TRUE(same());
}

// Filled as a ring rather than stroked, so the curved corners round apart a little.
TEST_F(Drawn, RoundedOutlineFollowsAStrokeInsideTheBox) {
    constexpr double RADIUS = 10;
    constexpr double THICK = 2;

    painter.outline({10, 10, 80, 50}, RADIUS, THICK, INK);
    direct.set_stroke_width(THICK);
    direct.stroke_round_rect(BLRoundRect(11, 11, 78, 48, RADIUS - (THICK / 2), RADIUS - (THICK / 2)), INK);
    finish();

    EXPECT_LE(worst(pixels(mine), pixels(theirs)), 48) << "the border is not the stroke drawn inside the box";
    EXPECT_EQ(pixel(mine, 10, 10), 0U) << "the outer corner was not rounded";
    EXPECT_EQ(pixel(mine, 50, 10), INK.value) << "the top edge is not solid";
    EXPECT_EQ(pixel(mine, 11, 35), INK.value) << "the left edge is not solid";
    EXPECT_EQ(pixel(mine, 12, 35), 0U) << "the border is thicker than its width";
}

TEST_F(Drawn, OutlineStaysInsideTheBox) {
    painter.outline({10.0, 10.0, 60.0, 40.0}, 8, 3, INK);
    finish();

    const Ink drawn = ink(mine);

    EXPECT_EQ(drawn.left, 10);
    EXPECT_EQ(drawn.top, 10);
    EXPECT_EQ(drawn.right, 70);
    EXPECT_EQ(drawn.bottom, 50);
    EXPECT_EQ(pixel(mine, 40, 30), 0U) << "the outline filled its inside";
}

TEST_F(Drawn, OutlineNeedsAPositiveWidth) {
    painter.outline({10, 10, 60, 40}, 4, 0, INK);
    painter.outline({10, 10, 60, 40}, 4, -2, INK);

    EXPECT_TRUE(blank());
}

TEST_F(Drawn, OutlineNeedsABoxWiderAndTallerThanTwiceItsWidth) {
    painter.outline({10, 10, 8, 40}, 0, 4, INK);
    painter.outline({30, 10, 40, 8}, 0, 4, INK);
    painter.outline({80, 10, 6, 6}, 0, 4, INK);

    EXPECT_TRUE(blank());
}

TEST_F(Drawn, OutlineJustWiderThanTwiceItsWidthDraws) {
    painter.outline({10, 10, 9, 9}, 0, 4, INK);

    EXPECT_FALSE(blank());
}

TEST_F(Drawn, OutlineOutsideTheClipDrawsNothing) {
    const ttk::Painter narrow(ours, tests::fonts(), {0, 0, 20, 20});

    narrow.outline({30, 30, 20, 20}, 4, 1, INK);

    EXPECT_TRUE(blank());
}

// The border filled whole, as the ring between the outer and the inner rounded rectangle.
void ring(BLContext &context, const BLRect &box, const double radius, const double width, const BLRgba32 tone) {
    const double corner = std::max(0.0, std::min(radius - (width / 2.0), std::min(box.w - width, box.h - width) / 2.0));
    const double outer = corner > 0.0 ? corner + (width / 2.0) : 0.0;
    const double inner = corner > 0.0 ? std::max(0.0, corner - (width / 2.0)) : 0.0;

    BLPath path;

    path.add_round_rect(BLRoundRect(box.x, box.y, box.w, box.h, outer, outer));
    path.add_round_rect(
            BLRoundRect(box.x + width, box.y + width, box.w - (width * 2.0), box.h - (width * 2.0), inner, inner),
            BL_GEOMETRY_DIRECTION_CCW);
    context.fill_path(path, tone);
}

struct Border {
    BLRect box;
    double radius = 0.0;
    double width = 0.0;
};

std::vector<Border> borders() {
    std::vector<Border> all;

    for (const double shift : {0.0, 0.5, 0.25, 0.7}) {
        for (const BLSize size : {BLSize{60, 40}, BLSize{64, 23}, BLSize{24, 24}, BLSize{23, 64}, BLSize{9, 9}}) {
            for (const double radius : {0.0, 1.0, 3.0, 8.0, 11.5, 12.0, 40.0}) {
                for (const double width : {1.0, 1.5, 2.0, 3.0}) {
                    all.push_back(Border{
                            .box = BLRect{10.0 + shift, 12.0 + (shift / 2.0), size.w, size.h},
                            .radius = radius,
                            .width = width,
                    });
                }
            }
        }
    }

    return all;
}

// Every channel of a translucent border over a ground, within a level of the ring filled whole.
TEST(Outline, PiecesMatchTheWholeRing) {
    const BLRgba32 ground(0xff303840);
    const BLRgba32 tone(0x9ad0e0f0);

    for (const Border &border : borders()) {
        BLImage mine(WIDTH, HEIGHT, BL_FORMAT_PRGB32);
        BLImage theirs(WIDTH, HEIGHT, BL_FORMAT_PRGB32);
        BLContext ours(mine);
        BLContext direct(theirs);

        ours.fill_all(ground);
        direct.fill_all(ground);
        ttk::Painter(ours, tests::fonts(), EVERYWHERE).outline(border.box, border.radius, border.width, tone);
        ring(direct, border.box, border.radius, border.width, tone);
        ours.end();
        direct.end();

        EXPECT_LE(worst(pixels(mine), pixels(theirs)), 1)
                << "box " << border.box.x << ',' << border.box.y << ' ' << border.box.w << 'x' << border.box.h
                << " radius " << border.radius << " width " << border.width;
    }
}

// Under a scale the pieces would not meet on whole pixels, so the ring is filled whole.
TEST(Outline, UnderAScaleIsTheWholeRing) {
    BLImage mine(WIDTH, HEIGHT, BL_FORMAT_PRGB32);
    BLImage theirs(WIDTH, HEIGHT, BL_FORMAT_PRGB32);
    BLContext ours(mine);
    BLContext direct(theirs);
    const BLRgba32 tone(0x9ad0e0f0);

    ours.clear_all();
    direct.clear_all();
    ours.scale(1.3);
    direct.scale(1.3);

    const ttk::Painter painter(ours, tests::fonts(), EVERYWHERE);

    painter.transformed(true);
    painter.outline({10.5, 10.5, 60, 40}, 8, 1, tone);
    ring(direct, {10.5, 10.5, 60, 40}, 8, 1, tone);
    ours.end();
    direct.end();

    EXPECT_EQ(pixels(mine), pixels(theirs));
}

TEST_F(Drawn, CircleMatchesBlend2D) {
    painter.circle({50.5, 40.25}, 17.5, INK);
    direct.fill_circle(50.5, 40.25, 17.5, INK);

    EXPECT_TRUE(same());
}

TEST_F(Drawn, CircleNeedsAPositiveRadius) {
    painter.circle({50, 40}, 0, INK);
    painter.circle({50, 40}, -3, INK);

    EXPECT_TRUE(blank());
}

TEST_F(Drawn, CircleOutsideTheClipDrawsNothing) {
    const ttk::Painter narrow(ours, tests::fonts(), {0, 0, 20, 20});

    narrow.circle({50, 50}, 10, INK);

    EXPECT_TRUE(blank());
}

TEST_F(Drawn, CircleReachingIntoTheClipDraws) {
    const ttk::Painter narrow(ours, tests::fonts(), {0, 0, 20, 20});

    narrow.circle({25, 10}, 10, INK);

    EXPECT_FALSE(blank());
}

BLPath triangle() {
    BLPath shape;

    shape.move_to(10, 80);
    shape.line_to(60, 10);
    shape.line_to(110, 80);
    shape.close();

    return shape;
}

BLPath zigzag() {
    BLPath shape;

    shape.move_to(10, 70);
    shape.line_to(40, 20);
    shape.line_to(70, 70);
    shape.line_to(100, 20);

    return shape;
}

TEST_F(Drawn, PathMatchesBlend2D) {
    painter.path(triangle(), INK);
    direct.fill_path(triangle(), INK);

    EXPECT_TRUE(same());
}

TEST_F(Drawn, StrokeMatchesBlend2DWithRoundCapsAndJoins) {
    painter.stroke(zigzag(), 5, INK);
    direct.set_stroke_width(5);
    direct.set_stroke_caps(BL_STROKE_CAP_ROUND);
    direct.set_stroke_join(BL_STROKE_JOIN_ROUND);
    direct.stroke_path(zigzag(), INK);

    EXPECT_TRUE(same());
}

TEST_F(Drawn, StrokeLeavesItsSettingsOnTheContext) {
    painter.stroke(zigzag(), 3.5, INK);

    EXPECT_EQ(ours.stroke_width(), 3.5);
    EXPECT_EQ(ours.stroke_join(), BL_STROKE_JOIN_ROUND);
    EXPECT_EQ(ours.stroke_start_cap(), BL_STROKE_CAP_ROUND);
    EXPECT_EQ(ours.stroke_end_cap(), BL_STROKE_CAP_ROUND);
}

TEST_F(Drawn, StrokeNeedsAPositiveWidth) {
    painter.stroke(zigzag(), 0, INK);
    painter.stroke(zigzag(), -1, INK);

    EXPECT_EQ(ours.stroke_width(), 1.0) << "a refused stroke still changed the context";
    EXPECT_TRUE(blank());
}

TEST_F(Drawn, ATransparentToneDrawsNothing) {
    painter.fill({10, 10, 50, 50}, CLEAR);
    painter.round({10, 10, 50, 50}, 6, CLEAR);
    painter.outline({10, 10, 50, 50}, 6, 2, CLEAR);
    painter.circle({50, 50}, 20, CLEAR);
    painter.path(triangle(), CLEAR);
    painter.stroke(zigzag(), 4, CLEAR);

    EXPECT_TRUE(blank());
}

TEST_F(Drawn, NeededRejectsAnEmptyBox) {
    EXPECT_FALSE(painter.needed({10, 10, 0, 10}));
    EXPECT_FALSE(painter.needed({10, 10, 10, 0}));
    EXPECT_FALSE(painter.needed({10, 10, -5, 10}));
}

TEST_F(Drawn, NeededAcceptsABoxInsideOrAcrossTheClip) {
    const ttk::Painter narrow(ours, tests::fonts(), {20, 20, 40, 40});

    EXPECT_TRUE(narrow.needed({30, 30, 5, 5}));
    EXPECT_TRUE(narrow.needed({0, 0, 21, 21}));
    EXPECT_TRUE(narrow.needed({0, 0, 200, 200}));
    EXPECT_TRUE(narrow.needed({59.5, 59.5, 10, 10}));
}

TEST_F(Drawn, NeededRejectsABoxOutsideOrOnlyTouchingTheClip) {
    const ttk::Painter narrow(ours, tests::fonts(), {20, 20, 40, 40});

    EXPECT_FALSE(narrow.needed({0, 0, 10, 10}));
    EXPECT_FALSE(narrow.needed({0, 0, 20, 100}));
    EXPECT_FALSE(narrow.needed({60, 20, 10, 10}));
    EXPECT_FALSE(narrow.needed({20, 60, 10, 10}));
}

TEST_F(Drawn, NeededAcceptsAnyBoxUnderATransform) {
    const ttk::Painter narrow(ours, tests::fonts(), {20, 20, 40, 40});

    ours.save();
    ours.translate(100, 100);
    narrow.transformed(true);

    EXPECT_TRUE(narrow.needed({-1000, -1000, 1, 1}));
    EXPECT_FALSE(narrow.needed({0, 0, 0, 10})) << "an empty box is rejected under a transform too";

    ours.restore();
    narrow.transformed(false);

    EXPECT_FALSE(narrow.needed({-1000, -1000, 1, 1}));
}

TEST_F(Drawn, ShapesUnderATransformAreNotCulledByTheClip) {
    const ttk::Painter narrow(ours, tests::fonts(), {20, 20, 40, 40});

    ours.translate(30, 30);
    narrow.transformed(true);
    narrow.fill({-5, -5, 10, 10}, INK);

    EXPECT_FALSE(blank());
}

TEST_F(Drawn, ClipIsTheRectangleGiven) {
    const ttk::Painter narrow(ours, tests::fonts(), {3, 4, 5, 6});

    EXPECT_EQ(narrow.clip().x, 3);
    EXPECT_EQ(narrow.clip().y, 4);
    EXPECT_EQ(narrow.clip().w, 5);
    EXPECT_EQ(narrow.clip().h, 6);
}

TEST_F(Drawn, PushNarrowsTheClipToWholePixels) {
    painter.push({10.5, 20.25, 30.1, 9.5});

    EXPECT_EQ(painter.clip().x, 10);
    EXPECT_EQ(painter.clip().y, 20);
    EXPECT_EQ(painter.clip().w, 31);
    EXPECT_EQ(painter.clip().h, 10);

    painter.pop();
}

TEST_F(Drawn, PushIntersectsWithTheClipInEffect) {
    const ttk::Painter narrow(ours, tests::fonts(), {20, 20, 40, 40});

    narrow.push({0, 30, 100, 100});

    EXPECT_EQ(narrow.clip().x, 20);
    EXPECT_EQ(narrow.clip().y, 30);
    EXPECT_EQ(narrow.clip().w, 40);
    EXPECT_EQ(narrow.clip().h, 30);

    narrow.pop();
}

TEST_F(Drawn, PushOfADisjointBoxLeavesNothingNeeded) {
    const ttk::Painter narrow(ours, tests::fonts(), {20, 20, 40, 40});

    narrow.push({100, 0, 10, 10});

    EXPECT_EQ(narrow.clip().w, 0);
    EXPECT_EQ(narrow.clip().h, 0);
    EXPECT_FALSE(narrow.needed({0, 0, 50, 50}));

    narrow.pop();

    EXPECT_TRUE(narrow.needed({0, 0, WIDTH, HEIGHT}));
}

TEST_F(Drawn, PushOfADisjointBoxNeedsNoBoxAcrossWhereTheyMissed) {
    const ttk::Painter narrow(ours, tests::fonts(), {20, 20, 40, 40});

    narrow.push({100, 0, 10, 10});

    EXPECT_FALSE(narrow.needed({0, 0, WIDTH, HEIGHT}));
    EXPECT_FALSE(narrow.needed({90, -10, 30, 30}));

    narrow.push({0, 0, WIDTH, HEIGHT});

    EXPECT_EQ(narrow.clip().w, 0) << "a push inside an empty clip found something";
    EXPECT_FALSE(narrow.needed({0, 0, WIDTH, HEIGHT}));

    narrow.pop();
    narrow.pop();
}

TEST_F(Drawn, PopRestoresTheClip) {
    painter.push({10, 10, 20, 20});
    painter.pop();

    EXPECT_EQ(painter.clip().x, EVERYWHERE.x);
    EXPECT_EQ(painter.clip().y, EVERYWHERE.y);
    EXPECT_EQ(painter.clip().w, EVERYWHERE.w);
    EXPECT_EQ(painter.clip().h, EVERYWHERE.h);
}

TEST_F(Drawn, PushesNest) {
    painter.push({10, 10, 100, 80});
    painter.push({50, 0, 100, 30});

    EXPECT_EQ(painter.clip().x, 50);
    EXPECT_EQ(painter.clip().y, 10);
    EXPECT_EQ(painter.clip().w, 60);
    EXPECT_EQ(painter.clip().h, 20);

    painter.pop();

    EXPECT_EQ(painter.clip().x, 10);
    EXPECT_EQ(painter.clip().w, 100);

    painter.pop();

    EXPECT_EQ(painter.clip().w, WIDTH);
}

TEST_F(Drawn, PushClipsTheContext) {
    painter.push({10.5, 10.5, 20, 20});
    ours.fill_all(INK);
    painter.pop();
    direct.fill_rect(BLRectI{10, 10, 21, 21}, INK);

    EXPECT_TRUE(same()) << "the context was not clipped to the pushed box in whole pixels";
}

TEST_F(Drawn, PopRestoresTheContextClip) {
    painter.push({10, 10, 20, 20});
    painter.pop();
    painter.fill({50, 50, 20, 20}, INK);

    EXPECT_FALSE(blank()) << "drawing after pop was still clipped to the pushed box";
}

TEST_F(Drawn, PopRestoresContextStateChangedInside) {
    painter.push({10, 10, 20, 20});
    ours.set_stroke_width(9);
    painter.pop();

    EXPECT_EQ(ours.stroke_width(), 1.0) << "push did not save the context state";
}

TEST_F(Drawn, ContextAndTypeAreWhatThePainterWasGiven) {
    EXPECT_EQ(&painter.context(), &ours);
    EXPECT_EQ(&painter.type(), &tests::fonts());
}

TEST(Painter, UnaimedPainterNeedsNothing) {
    const ttk::Painter idle(tests::fonts());

    EXPECT_EQ(idle.clip().w, 0);
    EXPECT_FALSE(idle.needed({0, 0, 100, 100}));
    EXPECT_FALSE(idle.needed({-10, -10, 20, 20})) << "a box across the origin was needed with no context";
    EXPECT_EQ(&idle.type(), &tests::fonts());
}

TEST_F(Drawn, AimRetargetsThePainter) {
    ttk::Painter aimed(tests::fonts());

    aimed.aim(direct, {5, 6, 7, 8});

    EXPECT_EQ(&aimed.context(), &direct);
    EXPECT_EQ(aimed.clip().x, 5);
    EXPECT_EQ(aimed.clip().h, 8);

    aimed.fill({0, 0, 20, 20}, INK);
    aimed.aim(ours, EVERYWHERE);
    aimed.fill({0, 0, 20, 20}, INK);

    EXPECT_TRUE(same()) << "one painter aimed twice drew differently into each context";
}

TEST_F(Drawn, AimDropsUnmatchedPushes) {
    ttk::Painter aimed(tests::fonts());

    aimed.aim(ours, EVERYWHERE);
    aimed.push({10, 10, 10, 10});
    ours.restore();
    aimed.aim(ours, {0, 0, 50, 50});

    EXPECT_EQ(aimed.clip().w, 50) << "a stale push survived aim";

    aimed.pop();

    EXPECT_EQ(aimed.clip().w, 50) << "pop restored a clip saved before aim";
}

TEST_F(Written, FontIsTheTypefacesOwn) {
    const BLFont &font = painter.font(400, 14.0F);

    EXPECT_EQ(&font, &tests::fonts().at(400, 14.0F));
    EXPECT_EQ(&font, &painter.font(400, 14.0F));
    EXPECT_NE(&font, &painter.font(700, 14.0F));
    EXPECT_NE(&font, &painter.font(400, 18.0F));
}

TEST_F(Written, WidthIsTheAdvanceOfTheRun) {
    EXPECT_EQ(painter.width(body(), ""), 0.0);
    EXPECT_GT(painter.width(body(), "Hello"), 0.0);
    EXPECT_DOUBLE_EQ(painter.width(body(), "Hello"), tests::fonts().width(body(), "Hello"));
    EXPECT_LT(painter.width(body(), "Hello"), painter.width(body(), "Hello, world"));
}

TEST_F(Written, WidthMeasuresUtf8) {
    EXPECT_GT(painter.width(body(), "日本語"), 0.0);
    EXPECT_GT(painter.width(body(), "é"), 0.0);
}

TEST_F(Written, LineHeightIsAscentPlusDescent) {
    const BLFontMetrics metrics = body().metrics();

    EXPECT_DOUBLE_EQ(painter.line_height(body()), static_cast<double>(metrics.ascent + metrics.descent));
    EXPECT_GT(painter.line_height(body()), 0.0);
}

TEST_F(Written, ElideLeavesARunThatFits) {
    EXPECT_EQ(painter.elide(body(), "Short", 500), "Short");
    EXPECT_EQ(painter.elide(body(), "", 500), "");
}

TEST_F(Written, ElideCutsWithAnEllipsis) {
    const std::string cut = painter.elide(body(), "A rather long run of text to cut", 60);

    EXPECT_TRUE(cut.ends_with(ELLIPSIS)) << cut;
    EXPECT_LE(painter.width(body(), cut), 60.0);
    EXPECT_TRUE(std::string_view("A rather long run of text to cut").starts_with(cut.substr(0, cut.size() - 3)));
}

TEST_F(Written, ElideCutsOnACharacterBoundary) {
    const std::string cut = painter.elide(body(), "ééééééééééééééééééééééé", 40);

    ASSERT_TRUE(cut.ends_with(ELLIPSIS)) << cut;
    EXPECT_EQ((cut.size() - ELLIPSIS.size()) % 2, 0U) << "a two byte character was split";
}

TEST_F(Written, TextMatchesTheTypeface) {
    painter.text(body(), {10.0, 20.0}, "Hello, wörld", INK);
    tests::fonts().draw(direct, body(), {10.0, 20.0}, "Hello, wörld", INK);

    EXPECT_TRUE(same());
}

TEST_F(Written, TextMatchesBlend2DOnTheRoundedBaseline) {
    const BLFontMetrics metrics = body().metrics();

    painter.text(body(), {10.0, 20.0}, "Hello", INK);
    direct.fill_utf8_text(BLPoint(10.0, std::floor(20.0 + metrics.ascent + 0.5)), body(), "Hello", 5, INK);
    finish();

    EXPECT_TRUE(near(pixels(mine), pixels(theirs), 2)) << "text left its baseline or its pen position";
}

TEST_F(Written, TextSitsInsideItsLineBox) {
    const double bleed = ttk::Typeface::bleed(body());

    painter.text(body(), {20.0, 30.0}, "Hxg", INK);
    finish();

    const Ink drawn = ink(mine);

    ASSERT_FALSE(drawn.empty());
    EXPECT_GE(drawn.top, 30.0 - bleed);
    EXPECT_LE(drawn.bottom, 30.0 + painter.line_height(body()) + bleed);
    EXPECT_GE(drawn.left, 19);
}

TEST_F(Written, TextOfAnEmptyRunDrawsNothing) {
    painter.text(body(), {10.0, 20.0}, "", INK);

    EXPECT_TRUE(blank());
}

TEST_F(Written, LabelAtTheStartMatchesCentredText) {
    painter.label(body(), {10, 10, 140, 40}, ttk::Align::Start, "Label", INK);
    tests::fonts().draw_centred(direct, body(), {10, 10}, 40.0F, "Label", INK);

    EXPECT_TRUE(same());
}

TEST_F(Written, LabelInTheCentreMatchesCentredText) {
    const double wide = painter.width(body(), "Label");

    painter.label(body(), {10, 10, 140, 40}, ttk::Align::Centre, "Label", INK);
    tests::fonts().draw_centred(direct, body(), {10 + ((140 - wide) / 2), 10}, 40.0F, "Label", INK);

    EXPECT_TRUE(same());
}

TEST_F(Written, LabelAtTheEndMatchesCentredText) {
    const double wide = painter.width(body(), "Label");

    painter.label(body(), {10, 10, 140, 40}, ttk::Align::End, "Label", INK);
    tests::fonts().draw_centred(direct, body(), {150 - wide, 10}, 40.0F, "Label", INK);

    EXPECT_TRUE(same());
}

TEST_F(Written, LabelAlignmentsMoveTheInkAcross) {
    BLImage start(WIDTH, HEIGHT, BL_FORMAT_PRGB32);
    BLImage centre(WIDTH, HEIGHT, BL_FORMAT_PRGB32);
    BLImage end(WIDTH, HEIGHT, BL_FORMAT_PRGB32);
    const BLRect box{10, 10, 140, 40};

    for (auto [image, align] : {
                 std::pair{&start, ttk::Align::Start},
                 std::pair{&centre, ttk::Align::Centre},
                 std::pair{&end, ttk::Align::End},
         }) {
        BLContext context(*image);

        context.clear_all();
        ttk::Painter(context, tests::fonts(), EVERYWHERE).label(body(), box, align, "Mid", INK);
        context.end();
    }

    const Ink left = ink(start);
    const Ink middle = ink(centre);
    const Ink right = ink(end);

    EXPECT_NEAR(left.left, box.x, 2.0);
    EXPECT_NEAR(middle.middle_x(), box.x + (box.w / 2), 3.0);
    EXPECT_NEAR(right.right, box.x + box.w, 2.0);
    EXPECT_NEAR(middle.middle_y(), box.y + (box.h / 2), 3.0) << "the label is not centred vertically";
}

TEST_F(Written, LabelElidesToTheBox) {
    constexpr std::string_view LONG = "A label far too long to fit the narrow box it was given";
    const std::string shown = painter.elide(body(), LONG, 70);

    painter.label(body(), {10, 10, 70, 30}, ttk::Align::Start, LONG, INK);
    tests::fonts().draw_centred(direct, body(), {10, 10}, 30.0F, shown, INK);

    ASSERT_TRUE(same());
    EXPECT_LE(ink(mine).right, 10 + 70 + 1) << "an elided label ran past its box";
}

TEST_F(Written, ElidedLabelAtTheEndStaysInsideTheBox) {
    painter.label(body(), {10, 10, 70, 30}, ttk::Align::End, "A label far too long for its box", INK);
    finish();

    const Ink drawn = ink(mine);

    EXPECT_GE(drawn.left, 9);
    EXPECT_LE(drawn.right, 81);
}

TEST_F(Written, LabelOfAnEmptyRunOrNoWidthDrawsNothing) {
    painter.label(body(), {10, 10, 100, 30}, ttk::Align::Start, "", INK);
    painter.label(body(), {10, 10, 0, 30}, ttk::Align::Centre, "Text", INK);
    painter.label(body(), {10, 10, -5, 30}, ttk::Align::End, "Text", INK);

    EXPECT_TRUE(blank());
}

TEST_F(Written, LabelTooNarrowForAnEllipsisDrawsNothing) {
    painter.label(body(), {10, 10, 1, 30}, ttk::Align::Start, "Text", INK);

    EXPECT_TRUE(blank());
}

TEST_F(Written, RowMatchesTheTypefacesOnceDrawing) {
    const double top = painter.baseline(body(), {10, 10, 140, 30}) - body().metrics().ascent;

    painter.row(body(), {10, 10, 140, 30}, "Row of a list", INK);
    tests::fonts().draw_once(direct, body(), {10, top}, "Row of a list", INK, 140);

    EXPECT_TRUE(same());
}

TEST_F(Written, RowOnMatchesTheTypefacesOnceDrawingOnTheBaseline) {
    painter.row_on(body(), {10, 40}, 140, "Row of a list", INK);
    tests::fonts().draw_once(direct, body(), {10, 40 - body().metrics().ascent}, "Row of a list", INK, 140);

    EXPECT_TRUE(same());
}

TEST_F(Written, RowOnOfAnEmptyRunOrNoRoomDrawsNothing) {
    painter.row_on(body(), {10, 40}, 140, "", INK);
    painter.row_on(body(), {10, 40}, 0, "Row", INK);

    EXPECT_TRUE(blank());
}

TEST_F(Written, FacesOnOneBaselineEndTheirCapitalsOnOneRow) {
    BLImage sans(WIDTH, HEIGHT, BL_FORMAT_PRGB32);
    BLImage mono(WIDTH, HEIGHT, BL_FORMAT_PRGB32);

    for (auto [image, weight] : {std::pair{&sans, ttk::Typeface::regular}, std::pair{&mono, ttk::Typeface::mono}}) {
        BLContext context(*image);

        context.clear_all();
        ttk::Painter(context, tests::fonts(), EVERYWHERE).row_on(painter.font(weight, 13.0F), {10, 50}, 140, "H", INK);
        context.end();
    }

    EXPECT_EQ(ink(sans).bottom, 50);
    EXPECT_EQ(ink(mono).bottom, 50);
}

TEST_F(Written, LabelSetsTheRunOnTheBaselineOfItsBox) {
    const BLFont &mono = painter.font(ttk::Typeface::mono, 13.0F);
    const BLRect box{10, 10.5, 140, 31};

    painter.label(mono, box, ttk::Align::Start, "Label", INK);
    tests::fonts().draw(direct, mono, {10, painter.baseline(mono, box) - mono.metrics().ascent}, "Label", INK);

    EXPECT_TRUE(same());
}

TEST_F(Written, LabelsOfEitherFaceCentreTheirCapitalsInTheBox) {
    const BLRect box{10, 10, 140, 40};

    for (const int weight : {ttk::Typeface::regular, ttk::Typeface::mono}) {
        BLImage image(WIDTH, HEIGHT, BL_FORMAT_PRGB32);
        BLContext context(image);

        context.clear_all();
        ttk::Painter(context, tests::fonts(), EVERYWHERE)
                .label(painter.font(weight, 13.0F), box, ttk::Align::Start, "H", INK);
        context.end();

        EXPECT_NEAR(ink(image).middle_y(), box.y + (box.h / 2.0), 1.0) << "weight " << weight;
    }
}

TEST_F(Written, RowIsCutAtTheRightEdge) {
    painter.row(body(), {10, 10, 60, 30}, "A row that runs far past the box it was given", INK);
    finish();

    EXPECT_LE(ink(mine).right, 71);
    EXPECT_NEAR(ink(mine).left, 10, 2.0);
}

TEST_F(Written, RowOfAnEmptyRunOrNoWidthDrawsNothing) {
    painter.row(body(), {10, 10, 100, 30}, "", INK);
    painter.row(body(), {10, 10, 0, 30}, "Text", INK);

    EXPECT_TRUE(blank());
}

TEST_F(Written, BandedRowInTheRowsOwnToneMatchesThePlainRow) {
    const double top = painter.baseline(body(), {10, 10, 140, 30}) - body().metrics().ascent;
    const std::vector<ttk::Typeface::Band> bands{{.from = 3.3, .to = 20.0, .tone = INK},
                                                 {.from = 41.6, .to = 77.1, .tone = INK}};

    painter.row(body(), {10, 10, 140, 30}, "Row of a list", INK, bands);
    tests::fonts().draw_once(direct, body(), {10, top}, "Row of a list", INK, 140);

    EXPECT_TRUE(same());
}

TEST_F(Written, BandedRowWithNoBandsIsThePlainRow) {
    const double top = painter.baseline(body(), {10, 10, 140, 30}) - body().metrics().ascent;

    painter.row(body(), {10, 10, 140, 30}, "Row of a list", INK, {});
    tests::fonts().draw_once(direct, body(), {10, top}, "Row of a list", INK, 140);

    EXPECT_TRUE(same());
}

// Columns of `image` holding any pixel leaning red, and any leaning to INK's blue.
struct Hues {
    std::vector<bool> red;
    std::vector<bool> blue;
};

Hues hues(const BLImage &image) {
    const std::vector<std::uint32_t> all = pixels(image);
    Hues found{.red = std::vector<bool>(WIDTH), .blue = std::vector<bool>(WIDTH)};

    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            const std::uint32_t at = all[(static_cast<std::size_t>(y) * WIDTH) + static_cast<std::size_t>(x)];
            const auto red = static_cast<int>((at >> 16U) & 0xffU);
            const auto blue = static_cast<int>(at & 0xffU);

            if ((at >> 24U) < 0x60U) {
                continue;
            }

            if (red > blue + 40) {
                found.red[static_cast<std::size_t>(x)] = true;
            } else if (blue > red + 40) {
                found.blue[static_cast<std::size_t>(x)] = true;
            }
        }
    }

    return found;
}

TEST_F(Written, BandsColourOnlyTheirStretch) {
    const std::vector<ttk::Typeface::Band> bands{{.from = 40.0, .to = 80.0, .tone = BLRgba32(0xffff0000)}};

    painter.row(body(), {10, 10, 140, 30}, "MMMMMMMMMMMMMMMM", INK, bands);
    finish();

    const Hues found = hues(mine);

    for (std::size_t x = 0; x < WIDTH; ++x) {
        if (x < 49 || x > 90) {
            EXPECT_FALSE(found.red[x]) << "red outside the band at " << x;
        }

        if (x > 51 && x < 88) {
            EXPECT_FALSE(found.blue[x]) << "the row's tone inside the band at " << x;
        }
    }

    EXPECT_TRUE(std::ranges::any_of(found.red, [](const bool on) { return on; }));
    EXPECT_TRUE(std::ranges::any_of(found.blue, [](const bool on) { return on; }));
}

TEST_F(Written, BandsUnderAFractionalTranslateColourOnlyTheirStretch) {
    const std::vector<ttk::Typeface::Band> bands{{.from = 40.0, .to = 80.0, .tone = BLRgba32(0xffff0000)}};

    ours.translate(0.37, 0.21);
    painter.transformed(true);
    painter.row(body(), {10, 10, 140, 30}, "MMMMMMMMMMMMMMMM", INK, bands);
    finish();

    const Hues found = hues(mine);

    for (std::size_t x = 0; x < WIDTH; ++x) {
        if (x < 49 || x > 91) {
            EXPECT_FALSE(found.red[x]) << "red outside the band at " << x;
        }

        if (x > 52 && x < 88) {
            EXPECT_FALSE(found.blue[x]) << "the row's tone inside the band at " << x;
        }
    }

    EXPECT_TRUE(std::ranges::any_of(found.red, [](const bool on) { return on; }));
    EXPECT_TRUE(std::ranges::any_of(found.blue, [](const bool on) { return on; }));
}

TEST_F(Written, TrackedMatchesTheTypeface) {
    painter.tracked(body(), {10, 20}, "TRACKED", INK, 3.0);
    tests::fonts().draw_tracked(direct, body(), {10, 20}, "TRACKED", INK, 3.0F);

    EXPECT_TRUE(same());
}

TEST_F(Written, TrackingWidensTheRunByTheSpacing) {
    BLImage loose(WIDTH, HEIGHT, BL_FORMAT_PRGB32);
    BLContext context(loose);

    context.clear_all();
    ttk::Painter(context, tests::fonts(), EVERYWHERE).tracked(body(), {10, 20}, "ABCD", INK, 5.0);
    context.end();
    painter.tracked(body(), {10, 20}, "ABCD", INK, 0.0);
    finish();

    EXPECT_NEAR(ink(loose).width() - ink(mine).width(), 15, 2) << "three gaps of five pixels were not added";
    EXPECT_NEAR(ink(loose).left, ink(mine).left, 1);
}

TEST_F(Written, TrackedOfAnEmptyRunDrawsNothing) {
    painter.tracked(body(), {10, 20}, "", INK, 4.0);

    EXPECT_TRUE(blank());
}

TEST_F(Written, ParagraphDrawsEachFoldedLineDown) {
    constexpr std::string_view RUN = "The quick brown fox jumps over the lazy dog again";
    const double step = painter.line_height(body());
    const std::vector<ttk::Fold> lines = ttk::fold_spans(tests::fonts(), body(), RUN, 90);

    ASSERT_GT(lines.size(), 1U);

    const double used = painter.paragraph(body(), {10, 5, 90, 1}, RUN, INK);

    for (std::size_t at = 0; at < lines.size(); ++at) {
        tests::fonts().draw(direct, body(), {10, 5 + (static_cast<double>(at) * step)}, lines[at].text, INK);
    }

    EXPECT_DOUBLE_EQ(used, static_cast<double>(lines.size()) * step);
    EXPECT_TRUE(same()) << "box.h cut lines or lines were placed elsewhere";
}

TEST_F(Written, ParagraphReturnsTheWrapHeight) {
    constexpr std::string_view RUN = "Several words that wrap onto more than a single line";

    EXPECT_DOUBLE_EQ(painter.paragraph(body(), {0, 0, 80, 10}, RUN, INK), painter.wrap_height(body(), RUN, 80));
}

TEST_F(Written, ParagraphOfAnEmptyRunReturnsOneLine) {
    EXPECT_DOUBLE_EQ(painter.paragraph(body(), {10, 10, 100, 50}, "", INK), painter.line_height(body()));
    EXPECT_TRUE(blank());
}

TEST_F(Written, ParagraphInATransparentToneMeasuresWithoutDrawing) {
    constexpr std::string_view RUN = "Measured but never drawn at all";

    EXPECT_DOUBLE_EQ(painter.paragraph(body(), {10, 10, 60, 50}, RUN, CLEAR), painter.wrap_height(body(), RUN, 60));
    EXPECT_TRUE(blank());
}

TEST_F(Written, ParagraphBreaksAtNewlines) {
    const double step = painter.line_height(body());

    EXPECT_DOUBLE_EQ(painter.paragraph(body(), {0, 0, 150, 10}, "one\ntwo\n\nfour", INK), 4 * step);
}

TEST_F(Written, WrapHeightOfAnEmptyRunIsZero) {
    EXPECT_EQ(painter.wrap_height(body(), "", 100), 0.0);
    EXPECT_EQ(ttk::wrap_height(tests::fonts(), body(), "", 100), 0.0);
}

TEST_F(Written, WrapHeightCountsTheFoldedLines) {
    constexpr std::string_view RUN = "Several words that wrap onto more than a single line";
    const std::size_t lines = ttk::fold_spans(tests::fonts(), body(), RUN, 70).size();

    EXPECT_DOUBLE_EQ(painter.wrap_height(body(), RUN, 70), static_cast<double>(lines) * painter.line_height(body()));
    EXPECT_DOUBLE_EQ(painter.wrap_height(body(), RUN, 70), ttk::wrap_height(tests::fonts(), body(), RUN, 70));
}

TEST_F(Written, WrapHeightGrowsAsTheRoomShrinks) {
    constexpr std::string_view RUN = "Several words that wrap onto more than a single line";

    EXPECT_DOUBLE_EQ(painter.wrap_height(body(), RUN, 10000), painter.line_height(body()));
    EXPECT_GT(painter.wrap_height(body(), RUN, 60), painter.wrap_height(body(), RUN, 200));
}

TEST_F(Written, FoldOfAnEmptyRunIsOneEmptyLine) {
    const std::vector<ttk::Fold> lines = ttk::fold_spans(tests::fonts(), body(), "", 100);

    ASSERT_EQ(lines.size(), 1U);
    EXPECT_TRUE(lines[0].text.empty());
    EXPECT_EQ(lines[0].from, 0U);
    EXPECT_EQ(lines[0].to, 0U);
}

TEST_F(Written, FoldKeepsARunThatFitsOnOneLine) {
    const std::vector<ttk::Fold> lines = ttk::fold_spans(tests::fonts(), body(), "fits on a line", 10000);

    ASSERT_EQ(lines.size(), 1U);
    EXPECT_EQ(lines[0].text, "fits on a line");
    EXPECT_EQ(lines[0].from, 0U);
    EXPECT_EQ(lines[0].to, 14U);
}

TEST_F(Written, FoldBreaksAtSpacesAndLeavesThemOut) {
    constexpr std::string_view RUN = "alpha beta gamma";
    const double room = painter.width(body(), "alpha beta") + 1;
    const std::vector<ttk::Fold> lines = ttk::fold_spans(tests::fonts(), body(), RUN, room);

    ASSERT_EQ(lines.size(), 2U);
    EXPECT_EQ(lines[0].text, "alpha beta");
    EXPECT_EQ(lines[0].from, 0U);
    EXPECT_EQ(lines[0].to, 10U);
    EXPECT_EQ(lines[1].text, "gamma");
    EXPECT_EQ(lines[1].from, 11U) << "the space it broke at was kept";
    EXPECT_EQ(lines[1].to, 16U);
}

TEST_F(Written, FoldEndsALineAtEveryNewline) {
    const std::vector<ttk::Fold> lines = ttk::fold_spans(tests::fonts(), body(), "one\n\nthree", 10000);

    ASSERT_EQ(lines.size(), 3U);
    EXPECT_EQ(lines[0].text, "one");
    EXPECT_TRUE(lines[1].text.empty());
    EXPECT_EQ(lines[2].text, "three");
    EXPECT_EQ(lines[2].from, 5U);
    EXPECT_EQ(lines[2].to, 10U);
}

TEST_F(Written, FoldCutsAWordWiderThanTheRoom) {
    constexpr std::string_view RUN = "abcdefghijklmnopqrstuvwxyz";
    const double room = painter.width(body(), "abcdefgh");
    const std::vector<ttk::Fold> lines = ttk::fold_spans(tests::fonts(), body(), RUN, room);

    ASSERT_GT(lines.size(), 1U);

    std::string joined;

    for (const ttk::Fold &line : lines) {
        EXPECT_FALSE(line.text.empty());
        EXPECT_LE(painter.width(body(), line.text), room + 0.5) << line.text;
        EXPECT_EQ(RUN.substr(line.from, line.to - line.from), line.text);
        joined += line.text;
    }

    EXPECT_EQ(joined, RUN) << "cutting a word lost or doubled characters";
}

TEST_F(Written, FoldCutsOnUtf8CharacterBoundaries) {
    constexpr std::string_view RUN = "日本語のとても長い単語です";
    const double room = painter.width(body(), "日本語");
    const std::vector<ttk::Fold> lines = ttk::fold_spans(tests::fonts(), body(), RUN, room);

    ASSERT_GT(lines.size(), 1U);

    for (const ttk::Fold &line : lines) {
        EXPECT_EQ(line.text.size() % 3, 0U) << "a three byte character was split";
        EXPECT_EQ(line.from % 3, 0U);
    }
}

TEST_F(Written, FoldKeepsOneCharacterPerLineWithNoRoom) {
    const std::vector<ttk::Fold> lines = ttk::fold_spans(tests::fonts(), body(), "abé", 0);

    ASSERT_EQ(lines.size(), 3U);
    EXPECT_EQ(lines[0].text, "a");
    EXPECT_EQ(lines[1].text, "b");
    EXPECT_EQ(lines[2].text, "é");
}

TEST_F(Written, FoldIsTheSameForTheSameRun) {
    const std::vector<ttk::Fold> first = ttk::fold_spans(tests::fonts(), body(), "repeat the same words", 50);
    const std::vector<ttk::Fold> second = ttk::fold_spans(tests::fonts(), body(), "repeat the same words", 50);

    ASSERT_EQ(first.size(), second.size());

    for (std::size_t at = 0; at < first.size(); ++at) {
        EXPECT_EQ(first[at].text, second[at].text);
    }
}
}
