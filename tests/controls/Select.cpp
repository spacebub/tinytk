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
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Mount.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Event.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/controls/Select.h"

namespace {
    using controls::key;
    namespace Code = ttk::Code;

    constexpr BLRect BOX{20, 10, 240, 80};
    constexpr double ROW = 32.0;

    std::vector<std::string> options(const int count) {
        std::vector<std::string> out;

        out.reserve(static_cast<std::size_t>(count));

        for (int at = 0; at < count; ++at) {
            out.push_back("Option " + std::to_string(at));
        }

        return out;
    }

    class SelectTest : public controls::Painted {
    protected:
        tests::Canvas canvas;
        std::vector<int> picked;
        std::vector<bool> openWhenPicked;
        ttk::Select *select = nullptr;

        ttk::Select *make(const int count = 3, std::string label = "Port", const BLRect &spot = BOX) {
            auto made = std::make_unique<ttk::Select>(std::move(label), [this](const int index) {
                picked.push_back(index);
                openWhenPicked.push_back(select->open());
            });

            made->set_options(options(count));
            select = controls::mount(canvas, std::move(made), spot);

            return select;
        }

        [[nodiscard]] BLRect frame() const {
            BLRect box = select->children().back()->box();

            box.h = ttk::Theme::control;

            return box;
        }

        void click_frame() {
            const BLRect box = frame();

            controls::click(canvas.root(), box.x + (box.w / 2.0), box.y + (box.h / 2.0));
        }

        [[nodiscard]] ttk::Widget *list() {
            const auto &held = canvas.root().layer(ttk::Root::POPUPS)->children();

            return held.empty() ? nullptr : held.back().get();
        }

        void click_row(const int row) {
            const BLRect box = list()->box();

            controls::click(canvas.root(), box.x + 40.0, box.y + 5.0 + (row * ROW) + (ROW / 2.0));
        }

        std::vector<std::uint32_t> field_pixels() {
            const BLRect box = frame();

            canvas.full();

            return controls::pixels_in(canvas, BLRectI{static_cast<int>(box.x), static_cast<int>(box.y),
                                                       static_cast<int>(box.w), static_cast<int>(box.h)});
        }

        void settle_animations() {
            for (int step = 0; step < 20; ++step) {
                canvas.frame_at(canvas.root().now() + 0.05);
            }
        }
    };

    TEST_F(SelectTest, StartsWithNothingCurrentAndClosed) {
        const ttk::Select *made = make();

        EXPECT_EQ(made->current(), -1);
        EXPECT_FALSE(made->open());
    }

    TEST_F(SelectTest, ClickingTheFieldOpensTheList) {
        make();
        click_frame();

        EXPECT_TRUE(select->open());
        ASSERT_NE(list(), nullptr);
        EXPECT_TRUE(canvas.root().has_dismiss());
    }

    TEST_F(SelectTest, ClickingTheFieldAgainClosesTheList) {
        make();
        click_frame();
        click_frame();

        EXPECT_FALSE(select->open());
        EXPECT_EQ(list(), nullptr);
        EXPECT_FALSE(canvas.root().has_dismiss());
    }

    TEST_F(SelectTest, TheListOpensBelowTheFieldWhenItFits) {
        make();
        click_frame();

        const BLRect box = list()->box();

        EXPECT_DOUBLE_EQ(box.x, frame().x);
        EXPECT_DOUBLE_EQ(box.w, frame().w);
        EXPECT_DOUBLE_EQ(box.y, frame().y + frame().h + 4.0);
        EXPECT_DOUBLE_EQ(box.h, (3 * ROW) + 10.0);
    }

    TEST_F(SelectTest, TheListOpensAboveTheFieldWithoutRoomBelow) {
        make(4, "", BLRect{20, 180, 240, 50});
        click_frame();

        const BLRect box = list()->box();

        EXPECT_DOUBLE_EQ(box.y + box.h, frame().y - 4.0);
    }

    TEST_F(SelectTest, TheListShowsAtMostNineRows) {
        make(20);
        click_frame();

        EXPECT_DOUBLE_EQ(list()->box().h, (9 * ROW) + 10.0);
    }

    TEST_F(SelectTest, AOneOptionListIsAtLeast44Tall) {
        make(1);
        click_frame();

        EXPECT_DOUBLE_EQ(list()->box().h, 44.0);
    }

    TEST_F(SelectTest, AClearableListHasARowForThePlaceholder) {
        make(2);
        select->clearable();
        click_frame();

        EXPECT_DOUBLE_EQ(list()->box().h, (3 * ROW) + 10.0);
    }

    TEST_F(SelectTest, NoOptionsNoList) {
        make(0);
        click_frame();

        EXPECT_FALSE(select->open());
        EXPECT_EQ(list(), nullptr);
    }

    TEST_F(SelectTest, PickingARowClosesTheListThenReports) {
        make();
        click_frame();
        click_row(2);

        EXPECT_EQ(picked, (std::vector<int>{2}));
        EXPECT_EQ(openWhenPicked, (std::vector<bool>{false})) << "the list was still open when selected ran";
        EXPECT_FALSE(select->open());
        EXPECT_EQ(list(), nullptr);
    }

    TEST_F(SelectTest, PickingDoesNotChangeTheCurrentOption) {
        make();
        select->set_current(0);
        click_frame();
        click_row(1);

        EXPECT_EQ(select->current(), 0);
    }

    TEST_F(SelectTest, PickingTheCurrentOptionStillReports) {
        make();
        select->set_current(1);
        click_frame();
        click_row(1);

        EXPECT_EQ(picked, (std::vector<int>{1}));
    }

    TEST_F(SelectTest, PickingThePlaceholderRowReportsMinusOne) {
        make();
        EXPECT_EQ(select->clearable(), select);

        click_frame();
        click_row(0);

        EXPECT_EQ(picked, (std::vector<int>{-1}));
    }

    TEST_F(SelectTest, AClearableRowShiftsTheOptions) {
        make();
        select->clearable();
        click_frame();
        click_row(1);

        EXPECT_EQ(picked, (std::vector<int>{0}));
    }

    TEST_F(SelectTest, ClearableTakesEffectAtTheNextOpen) {
        make();
        click_frame();
        select->clearable();
        click_row(0);

        EXPECT_EQ(picked, (std::vector<int>{0})) << "an open list grew a placeholder row";
    }

    TEST_F(SelectTest, TheListOpensScrolledToTheCurrentOption) {
        make(20);
        select->set_current(15);
        click_frame();

        const BLRect box = list()->box();

        controls::click(canvas.root(), box.x + 40.0, box.y + box.h - 5.0 - (ROW / 2.0));

        EXPECT_EQ(picked, (std::vector<int>{15}));
    }

    TEST_F(SelectTest, AClickOutsideClosesTheList) {
        make();
        click_frame();
        controls::click(canvas.root(), 390.0, 230.0);

        EXPECT_FALSE(select->open());
        EXPECT_EQ(list(), nullptr);
        EXPECT_TRUE(picked.empty());
    }

    TEST_F(SelectTest, TheRootsDismissClosesTheList) {
        make();
        click_frame();
        canvas.root().dismiss();

        EXPECT_FALSE(select->open());
        EXPECT_EQ(list(), nullptr);
    }

    TEST_F(SelectTest, ResizingTheWindowClosesTheList) {
        make();
        click_frame();
        canvas.root().resize(500, 300);

        EXPECT_FALSE(select->open());
    }

    TEST_F(SelectTest, CloseReleasesTheDismissHandler) {
        make();
        click_frame();
        select->close();

        EXPECT_FALSE(select->open());
        EXPECT_FALSE(canvas.root().has_dismiss());
        EXPECT_EQ(list(), nullptr);
    }

    TEST_F(SelectTest, CloseWhenClosedDoesNothing) {
        make();
        canvas.root().set_dismiss([] {});
        select->close();

        EXPECT_TRUE(canvas.root().has_dismiss()) << "closing a closed select released another popup";
    }

    TEST_F(SelectTest, ReturnAndSpaceOpenAndClose) {
        make();
        canvas.root().focus(select);

        EXPECT_TRUE(select->key(key(Code::Return)));
        EXPECT_TRUE(select->open());
        EXPECT_TRUE(select->key(key(Code::Space)));
        EXPECT_FALSE(select->open());
        EXPECT_TRUE(canvas.root().key(key(Code::Space)));
        EXPECT_TRUE(select->open());
    }

    TEST_F(SelectTest, DownReportsTheNextOption) {
        make();
        select->set_current(0);

        EXPECT_TRUE(select->key(key(Code::Down)));
        EXPECT_EQ(picked, (std::vector<int>{1}));
        EXPECT_EQ(select->current(), 0);
    }

    TEST_F(SelectTest, DownFromNothingReportsTheFirst) {
        make();

        EXPECT_TRUE(select->key(key(Code::Down)));
        EXPECT_EQ(picked, (std::vector<int>{0}));
    }

    TEST_F(SelectTest, DownAtTheLastOptionIsNotConsumed) {
        make();
        select->set_current(2);

        EXPECT_FALSE(select->key(key(Code::Down)));
        EXPECT_TRUE(picked.empty());
    }

    TEST_F(SelectTest, UpReportsThePreviousOption) {
        make();
        select->set_current(2);

        EXPECT_TRUE(select->key(key(Code::Up)));
        EXPECT_EQ(picked, (std::vector<int>{1}));
    }

    TEST_F(SelectTest, UpAtTheFirstOptionIsNotConsumed) {
        make();
        select->set_current(0);

        EXPECT_FALSE(select->key(key(Code::Up)));
        EXPECT_TRUE(picked.empty());
    }

    TEST_F(SelectTest, UpAtTheFirstOptionOfAClearableSelectReportsMinusOne) {
        make();
        select->clearable();
        select->set_current(0);

        EXPECT_TRUE(select->key(key(Code::Up)));
        EXPECT_EQ(picked, (std::vector<int>{-1}));

        select->set_current(-1);

        EXPECT_FALSE(select->key(key(Code::Up)));
        EXPECT_EQ(picked.size(), 1U);
    }

    TEST_F(SelectTest, OtherKeysAreNotConsumed) {
        make();

        EXPECT_FALSE(select->key(key(Code::Left)));
        EXPECT_FALSE(select->key(key(Code::Right)));
        EXPECT_FALSE(select->key(key(Code::Tab)));
        EXPECT_FALSE(select->key(key('a')));
        EXPECT_TRUE(picked.empty());
    }

    TEST_F(SelectTest, SetCurrentDoesNotReport) {
        make();
        select->set_current(1);

        EXPECT_EQ(select->current(), 1);
        EXPECT_TRUE(picked.empty());
    }

    TEST_F(SelectTest, AnOutOfRangeCurrentShowsThePlaceholder) {
        make();

        const std::vector<std::uint32_t> none = field_pixels();

        select->set_current(7);

        EXPECT_EQ(select->current(), 7);
        EXPECT_EQ(field_pixels(), none);

        select->set_current(1);

        EXPECT_NE(field_pixels(), none) << "a current option looked like the placeholder";
    }

    TEST_F(SelectTest, SetOptionsKeepsTheCurrentIndex) {
        make();
        select->set_current(2);
        select->set_options(options(5));

        EXPECT_EQ(select->current(), 2);

        select->set_options(std::vector<std::string>{"only"});

        EXPECT_EQ(select->current(), 2);
    }

    TEST_F(SelectTest, SetOptionsWithTheSameOptionsRepaintsNothing) {
        make();
        canvas.frame();

        const std::vector<std::string> same = options(3);

        select->set_options(same);

        EXPECT_FALSE(canvas.root().dirty());

        select->set_options(options(3));

        EXPECT_FALSE(canvas.root().dirty());
    }

    TEST_F(SelectTest, AnOpenListKeepsTheOptionsItOpenedWith) {
        make(3);
        click_frame();

        const double tall = list()->box().h;

        select->set_options(options(8));

        EXPECT_DOUBLE_EQ(list()->box().h, tall);

        click_row(2);

        EXPECT_EQ(picked, (std::vector<int>{2}));
    }

    TEST_F(SelectTest, PlaceholderReplacesTheDefaultText) {
        make();

        const std::vector<std::uint32_t> standard = field_pixels();

        EXPECT_EQ(select->placeholder("Pick one"), select);
        EXPECT_NE(field_pixels(), standard);
    }

    TEST_F(SelectTest, TheBadgeOfTheCurrentOptionIsDrawnInTheField) {
        make();
        select->set_current(0);

        const std::vector<std::uint32_t> plain = field_pixels();

        select->set_badges(std::vector<std::string>{"", "fast"});

        EXPECT_EQ(field_pixels(), plain) << "another option's badge reached the field";

        select->set_badges(std::vector<std::string>{"new"});

        EXPECT_NE(field_pixels(), plain);
    }

    TEST_F(SelectTest, SetBadgesWithTheSameBadgesRepaintsNothing) {
        make();

        const std::vector<std::string> badges{"a", "b"};

        select->set_badges(badges);
        canvas.frame();
        select->set_badges(badges);

        EXPECT_FALSE(canvas.root().dirty());

        select->set_badges(std::vector<std::string>{"a", "b"});

        EXPECT_FALSE(canvas.root().dirty());
    }

    TEST_F(SelectTest, NaturalWidthIsAtLeast200) {
        ttk::Select made("", nullptr);

        EXPECT_GE(made.natural_width(tests::fonts()), 200.0);

        made.fixedWidth = 120.0;

        EXPECT_DOUBLE_EQ(made.natural_width(tests::fonts()), 120.0);
    }

    TEST_F(SelectTest, OnlyTheFieldTakesThePointer) {
        make();

        const BLRect box = frame();
        const BLRect caption = select->children().front()->box();

        EXPECT_EQ(select->at(box.x + 5.0, box.y + 5.0), select);
        EXPECT_EQ(select->at(caption.x + 2.0, caption.y + 2.0), nullptr);
        EXPECT_EQ(select->at(box.x + 5.0, box.y + box.h + 2.0), nullptr);
    }

    TEST_F(SelectTest, ADisabledSelectIgnoresThePointer) {
        make();
        select->set_enabled(false);

        const BLRect box = frame();

        EXPECT_EQ(select->at(box.x + 5.0, box.y + 5.0), nullptr);
        EXPECT_FALSE(select->press(ttk::Pointer{.x = box.x + 5.0, .y = box.y + 5.0}));

        click_frame();

        EXPECT_FALSE(select->open());
    }

    TEST_F(SelectTest, AHiddenSelectIgnoresThePointer) {
        make();
        select->set_visible(false);

        const BLRect box = frame();

        EXPECT_EQ(select->at(box.x + 5.0, box.y + 5.0), nullptr);
    }

    TEST_F(SelectTest, ReleasingOffTheSelectDoesNotOpen) {
        make();

        const BLRect box = frame();

        canvas.root().press(ttk::Pointer{.x = box.x + 5.0, .y = box.y + 5.0});
        canvas.root().release(ttk::Pointer{.x = 390.0, .y = 230.0});

        EXPECT_FALSE(select->open());
    }

    TEST_F(SelectTest, TooltipSetsTheHint) {
        make();

        EXPECT_EQ(select->tooltip("Which port"), select);
        EXPECT_EQ(select->hint, "Which port");
    }

    TEST_F(SelectTest, AnEmptyCaptionIsHidden) {
        make(3, "");

        EXPECT_DOUBLE_EQ(frame().y, BOX.y);
    }

    TEST_F(SelectTest, HoverStartsAnAnimationThatEnds) {
        make();
        canvas.root().advance(0.0);

        const BLRect box = frame();

        canvas.root().motion(box.x + 5.0, box.y + 5.0);

        EXPECT_TRUE(canvas.root().busy());

        settle_animations();

        EXPECT_FALSE(canvas.root().busy());
    }

    TEST_F(SelectTest, OpenFieldIsOutlinedInTheAccent) {
        make();

        const BLRect box = frame();
        const int x = static_cast<int>(box.x + (box.w / 2.0));
        const int y = static_cast<int>(std::round(box.y));
        const ttk::Theme::Palette &palette = ttk::Theme::palette();

        canvas.full();

        EXPECT_FALSE(controls::leans(canvas.at(x, y), palette.accent, palette.border));

        click_frame();
        canvas.root().motion(390.0, 230.0);
        settle_animations();

        EXPECT_TRUE(controls::leans(canvas.at(x, y), palette.accent, palette.border));
    }

    TEST_F(SelectTest, TheChevronTurnsWhileOpen) {
        make();

        const BLRect box = frame();
        const BLRectI chevron{static_cast<int>(box.x + box.w - 32.0), static_cast<int>(box.y + 8.0), 28,
                              static_cast<int>(box.h - 16.0)};

        canvas.root().motion(390.0, 230.0);
        settle_animations();
        canvas.full();

        const std::vector<std::uint32_t> closed = controls::pixels_in(canvas, chevron);

        click_frame();
        canvas.root().motion(390.0, 230.0);
        settle_animations();
        canvas.full();

        EXPECT_NE(controls::pixels_in(canvas, chevron), closed);
    }

    TEST_F(SelectTest, PartialRepaintsMatchAFullOne) {
        make(12);
        select->set_current(4);
        select->set_badges(std::vector<std::string>{"a", "b", "c", "d", "e"});
        click_frame();
        settle_animations();
        click_row(2);
        settle_animations();

        EXPECT_TRUE(canvas.matches_full());
    }
}
