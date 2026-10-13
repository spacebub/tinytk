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
#include <cstdlib>
#include <memory>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "support/Canvas.h"
#include "ttk/draw/Damage.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/controls/TextBox.h"
#include "ttk/toolkit/controls/TextEdit.h"
#include "ttk/toolkit/layout/Scroll.h"
#include "ttk/util/Cache.h"

namespace {
std::size_t area(const ttk::Damage &damage) {
    std::size_t total = 0;

    for (const BLRectI &r : damage.regions()) {
        total += static_cast<std::size_t>(r.w) * static_cast<std::size_t>(r.h);
    }

    return total;
}

bool covered(const ttk::Damage &damage, const int x, const int y) {
    return std::ranges::any_of(
            damage.regions(), [&](const BLRectI &r) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; });
}

// Antialiased corners cut by a strip's edge can round one level apart.
bool near(const std::vector<std::uint32_t> &one, const std::vector<std::uint32_t> &two) {
    return std::ranges::equal(one, two, [](const std::uint32_t a, const std::uint32_t b) {
        for (unsigned shift = 0; shift < 32; shift += 8) {
            if (std::abs(static_cast<int>((a >> shift) & 0xffU) - static_cast<int>((b >> shift) & 0xffU)) > 1) {
                return false;
            }
        }

        return true;
    });
}

TEST(Damage, KeepsDistantRectanglesApartAndDropsRepeats) {
    ttk::Damage damage;

    damage.resize(800, 600);
    damage.add({10, 10, 2, 20});
    damage.add({700, 500, 2, 20});

    for (int at = 0; at < 100; ++at) {
        damage.add({10, 10, 2, 20});
    }

    EXPECT_EQ(damage.regions().size(), 2U);
    EXPECT_EQ(area(damage), 80U);

    damage.add({0, 0, 800, 600});

    EXPECT_EQ(damage.regions().size(), 1U);
    EXPECT_TRUE(damage.whole());
}

TEST(Damage, ScrollStripsStayStrips) {
    ttk::Damage damage;

    damage.resize(800, 600);
    damage.add({0, 0, 800, 3});
    damage.add({0, 599, 800, 1});
    damage.add({790, 0, 8, 600});

    EXPECT_LT(area(damage), 10000U) << "scroll strips expanded into the viewport";
}

TEST(Damage, RoundsOutwardAndDropsWhatHasNoPixels) {
    ttk::Damage damage;

    damage.resize(800, 600);
    damage.add({-0.5, -0.5, 2, 2});

    EXPECT_EQ(area(damage), 4U) << "fractional damage was not rounded outward";

    damage.add({1e100, 0, 1, 1});
    damage.add({1, 1, -0.1, 2});

    EXPECT_EQ(area(damage), 4U) << "empty or offscreen damage survived";
}

TEST(Damage, MergingNeverLosesAPixel) {
    ttk::Damage damage;
    std::vector<bool> expected(static_cast<std::size_t>(64) * 64);
    // NOLINTNEXTLINE(bugprone-random-generator-seed,cert-msc32-c,cert-msc51-cpp): a fixed seed keeps the case repeatable.
    std::mt19937 random(7);

    damage.resize(64, 64);

    for (int at = 0; at < 160; ++at) {
        const int x = static_cast<int>(random() % 64);
        const int y = static_cast<int>(random() % 64);

        damage.add({static_cast<double>(x), static_cast<double>(y), 1, 1});
        expected[(static_cast<std::size_t>(y) * 64) + static_cast<std::size_t>(x)] = true;

        ASSERT_LE(damage.regions().size(), ttk::Damage::CROWDED);

        for (int py = 0; py < 64; ++py) {
            for (int px = 0; px < 64; ++px) {
                if (expected[(static_cast<std::size_t>(py) * 64) + static_cast<std::size_t>(px)]) {
                    ASSERT_TRUE(covered(damage, px, py)) << "lost pixel " << px << "," << py;
                }
            }
        }
    }
}

TEST(Damage, ShiftMovesPendingDamageWithThePixels) {
    ttk::Damage damage;

    damage.resize(100, 100);
    damage.add({10, 50, 10, 10});
    damage.shift({0, 20, 100, 60}, -20);

    EXPECT_TRUE(covered(damage, 10, 30));
    EXPECT_TRUE(covered(damage, 19, 39));
    EXPECT_TRUE(covered(damage, 10, 50));
    EXPECT_LE(area(damage), 300U);

    const std::size_t moved = area(damage);

    damage.shift({0, 0, 100, 40}, 30);

    EXPECT_EQ(area(damage), moved) << "shift damaged pixels moved out of its region";
}

TEST(Damage, NothingIsKeptForAFrameWithoutPixels) {
    ttk::Damage damage;

    damage.resize(0, 0);
    damage.all();

    EXPECT_TRUE(damage.empty());
}

TEST(Cache, HitKeepsEveryEntryAndMissBoundsThem) {
    struct Entry {
        std::size_t used = 0;
    };

    ttk::Cache::RunMap<Entry> entries;

    for (int at = 0; at < 8; ++at) {
        ttk::Cache::run_entry(entries, {.at = at, .run = "same"}, 8).first.used = static_cast<std::size_t>(at) + 1;
    }

    auto [hit, fresh] = ttk::Cache::run_entry(entries, {.at = 0, .run = "same"}, 8);

    EXPECT_FALSE(fresh);
    EXPECT_EQ(entries.size(), 8U);
    EXPECT_EQ(hit.used, 1U);

    ttk::Cache::run_entry(entries, {.at = 8, .run = "new"}, 8);

    EXPECT_LT(entries.size(), 8U);
}

class Painted : public ::testing::Test {
protected:
    void SetUp() override {
        if (!tests::fonts_loaded()) {
            GTEST_SKIP() << "no font installed";
        }
    }
};

TEST_F(Painted, RootKeepsDamageGivenBeforeItsFirstSize) {
    ttk::Root root(tests::fonts());

    root.damage({0, 0, 10, 10});

    EXPECT_TRUE(root.dirty());

    root.resize(400, 240);
    root.settle();

    EXPECT_TRUE(root.whole()) << "first layout did not damage the whole window";
}

class CaretBlink : public Painted, public ::testing::WithParamInterface<bool> {};

TEST_P(CaretBlink, RepaintsOnlyTheCaret) {
    const bool editor = GetParam();
    tests::Canvas canvas;
    ttk::Widget *widget = nullptr;

    if (editor) {
        auto edit = std::make_unique<ttk::TextEdit>();

        edit->set_text("first line\nsecond line with text\nthird line\nfourth line\nfifth line\nsixth line");
        edit->numbered();
        widget = canvas.root().layer(ttk::Root::POPUPS)->add(std::move(edit));
    } else {
        auto edit = std::make_unique<ttk::TextBox>(nullptr);

        edit->set_text("A narrow caret repaint: й é");
        widget = canvas.root().layer(ttk::Root::POPUPS)->add(std::move(edit));
    }

    widget->place({13.25, 17.5, 350, editor ? 190.0 : 28.0}, tests::fonts());
    canvas.frame();
    canvas.root().focus(widget);
    canvas.frame();
    widget->key(ttk::Key{.code = ttk::Code::Right});
    canvas.frame();

    for (int blink = 1; blink <= 4; ++blink) {
        const std::size_t painted = canvas.frame_at(static_cast<double>(blink));

        EXPECT_GT(painted, 0U);
        EXPECT_LT(painted, 150U) << "caret blink repainted more than its narrow strip";
        EXPECT_TRUE(canvas.matches_full()) << "caret partial/full mismatch at blink " << blink;
    }

    canvas.root().set_active(false);
    canvas.root().advance(5);
    canvas.frame();

    EXPECT_TRUE(canvas.matches_full()) << "inactive caret partial/full mismatch";
}

INSTANTIATE_TEST_SUITE_P(Controls, CaretBlink, ::testing::Bool());

TEST_F(Painted, EmptiedFieldInvalidatesWhereItsCaretIsDrawn) {
    tests::Canvas canvas;
    auto edit = std::make_unique<ttk::TextBox>(nullptr);

    edit->set_text("A field holding far more text than its width shows");

    ttk::Widget *widget = canvas.root().layer(ttk::Root::POPUPS)->add(std::move(edit));

    widget->place({13.25, 17.5, 120, 28}, tests::fonts());
    canvas.frame();
    canvas.root().focus(widget);
    canvas.frame();
    widget->key(ttk::Key{.code = ttk::Code::End});
    canvas.frame();
    widget->key(ttk::Key{.code = 'a', .ctrl = true});
    widget->key(ttk::Key{.code = 'x', .ctrl = true});
    canvas.frame();

    for (int blink = 1; blink <= 2; ++blink) {
        canvas.frame_at(static_cast<double>(blink));

        EXPECT_TRUE(canvas.matches_full()) << "emptied textbox caret partial/full mismatch at blink " << blink;
    }
}

class Rows final : public ttk::Widget {
public:
    Rows() { fixedHeight = 1200; }

    bool marked = false;

    void paint(const ttk::Painter &painter) override {
        for (int row = 0; row < 60; ++row) {
            const BLRect box{_box.x, _box.y + (row * 20), _box.w, 20};

            if (!painter.needed(box)) {
                continue;
            }

            painter.fill(box, BLRgba32(marked && row == 10 ? 0xffa03040 : row % 2 != 0 ? 0xff405060 : 0xff203040));
            painter.text(painter.font(400, 12), {box.x + 8, box.y + 2}, "Scrolling row", BLRgba32(0xffffffff));
        }
    }
};

TEST_F(Painted, ScrollRepaintsOnlyTheStrips) {
    tests::Canvas canvas;
    auto scroller = std::make_unique<ttk::Scroll>();
    auto *scroll = scroller.get();
    auto rows = std::make_unique<Rows>();
    auto *content = rows.get();

    scroll->hold(std::move(rows));
    canvas.root().content()->add(std::move(scroller));
    canvas.frame();

    for (const double offset : {3, 10, 7, 55, 44}) {
        scroll->scroll_to(offset);

        EXPECT_LT(canvas.frame(), static_cast<std::size_t>(400 * 240 / 3))
                << "scroll repainted the viewport instead of strips";
        EXPECT_TRUE(canvas.matches_full()) << "scroll partial/full mismatch at " << offset;
    }

    content->marked = true;
    content->invalidate({0, content->box().y + 200, 400, 20});
    scroll->scroll_to(60);
    scroll->scroll_to(65);
    scroll->scroll_to(62);
    canvas.frame();

    EXPECT_TRUE(canvas.matches_full()) << "batched scroll partial/full mismatch";

    canvas.root().damage_all();
    scroll->scroll_to(75);
    canvas.frame();

    EXPECT_TRUE(canvas.matches_full()) << "full damage plus scroll mismatch";
}

TEST_F(Painted, ClippedParagraphKeepsItsInk) {
    tests::Canvas canvas;
    auto label = std::make_unique<ttk::Label>("First line\nSecond line\nThird line with accents é й\nFourth "
                                              "line\nFifth line\nSixth line\nSeventh line");

    label->wrap();
    canvas.root().content()->add(std::move(label));
    canvas.frame();

    const std::vector<std::uint32_t> full = canvas.pixels();

    for (int y = 0; y < 160; y += 7) {
        canvas.root().damage({0, static_cast<double>(y), 400, 3});
        canvas.frame();

        ASSERT_EQ(full, canvas.pixels()) << "clipped paragraph lost ink outside a line box at " << y;
    }

    canvas.root().relayout();

    EXPECT_TRUE(canvas.root().dirty()) << "pending layout did not wake rendering";
}

TEST_F(Painted, ClippedEditorRowsKeepTheirInk) {
    tests::Canvas canvas;
    auto edit = std::make_unique<ttk::TextEdit>();

    edit->set_text("first line\nÉjy accents й\nthird\nfourth\nfifth\nsixth");
    edit->numbered();

    ttk::Widget *widget = canvas.root().layer(ttk::Root::POPUPS)->add(std::move(edit));

    widget->place({13.25, 17.5, 350, 190}, tests::fonts());
    canvas.frame();

    const std::vector<std::uint32_t> full = canvas.pixels();

    for (int y = 0; y <= 237; y += 3) {
        canvas.root().damage({0, static_cast<double>(y), 400, 3});
        canvas.frame();

        ASSERT_TRUE(near(full, canvas.pixels())) << "clipped editor rows lost ink outside a row at " << y;
    }
}

TEST_F(Painted, TransformedShapesAreNotCulledByTheClip) {
    tests::Canvas canvas;
    BLContext &context = canvas.surface().context();

    context.clear_all();
    context.save();
    context.clip_to_rect(BLRectI{20, 20, 30, 30});
    context.translate(30, 30);
    context.fill_round_rect(BLRect{-10, -10, 20, 20}, 3, 3, BLRgba32(0xffabcdef));
    context.restore();
    canvas.surface().present();

    const std::vector<std::uint32_t> expected = canvas.pixels();

    context.clear_all();
    context.save();
    context.clip_to_rect(BLRectI{20, 20, 30, 30});
    context.translate(30, 30);

    const ttk::Painter painter(context, tests::fonts(), {20, 20, 30, 30});

    painter.round({-10, -10, 20, 20}, 3, BLRgba32(0xffabcdef));
    context.restore();
    canvas.surface().present();

    EXPECT_EQ(expected, canvas.pixels()) << "clip rejection discarded transformed shapes";
}
}
