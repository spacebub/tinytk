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
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Mount.h"
#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Event.h"
#include "ttk/toolkit/controls/Stepper.h"

namespace {
using controls::key;
namespace Code = ttk::Code;

constexpr BLRect BOX{20, 20, 200, 80};

class StepperTest : public controls::Painted {
protected:
    tests::Canvas canvas;
    std::vector<int> stepped;
    ttk::Stepper *stepper = nullptr;

    std::unique_ptr<ttk::Stepper> build(std::string label = "Players") {
        return std::make_unique<ttk::Stepper>(std::move(label), [this](const int value) { stepped.push_back(value); });
    }

    ttk::Stepper *place(std::unique_ptr<ttk::Stepper> made) {
        stepper = controls::mount(canvas, std::move(made), BOX);

        return stepper;
    }

    ttk::Stepper *make(std::string label = "Players") { return place(build(std::move(label))); }

    [[nodiscard]] ttk::Widget *frame() const { return stepper->children().back().get(); }

    [[nodiscard]] ttk::Widget *minus() const { return frame()->children().front().get(); }

    [[nodiscard]] ttk::Widget *plus() const { return frame()->children().back().get(); }

    void relayout() {
        canvas.root().relayout();
        canvas.frame();
    }

    void click(const ttk::Widget *button) {
        const BLRect &box = button->box();

        controls::click(canvas.root(), box.x + (box.w / 2.0), box.y + (box.h / 2.0));
    }

    std::vector<std::uint32_t> field_pixels() {
        const BLRect &box = frame()->box();

        canvas.full();

        return controls::pixels_in(canvas,
                                   BLRectI{static_cast<int>(box.x + 40.0),
                                           static_cast<int>(box.y),
                                           static_cast<int>(box.w - 80.0),
                                           static_cast<int>(box.h)});
    }
};

TEST_F(StepperTest, StartsAtOne) {
    make();

    EXPECT_EQ(stepper->value(), 1);
}

TEST_F(StepperTest, PlusReportsTheNextValueWithoutTakingIt) {
    make();
    stepper->set_value(4);
    relayout();
    click(plus());

    EXPECT_EQ(stepped, (std::vector<int>{5}));
    EXPECT_EQ(stepper->value(), 4);
}

TEST_F(StepperTest, MinusReportsThePreviousValue) {
    make();
    stepper->set_value(4);
    relayout();
    click(minus());

    EXPECT_EQ(stepped, (std::vector<int>{3}));
}

TEST_F(StepperTest, SetValueDoesNotReport) {
    make();
    stepper->set_value(7);

    EXPECT_EQ(stepper->value(), 7);
    EXPECT_TRUE(stepped.empty());
}

TEST_F(StepperTest, SetValueIsNotClamped) {
    make();
    stepper->set_value(40);

    EXPECT_EQ(stepper->value(), 40);
}

TEST_F(StepperTest, SetValueRepaintsOnlyOnAChange) {
    make();
    canvas.frame();
    stepper->set_value(1);

    EXPECT_FALSE(canvas.root().dirty());

    stepper->set_value(2);

    EXPECT_TRUE(canvas.root().dirty());
}

TEST_F(StepperTest, MinusIsDisabledAtTheLowerEnd) {
    make();

    EXPECT_FALSE(minus()->enabled());
    EXPECT_TRUE(plus()->enabled());

    click(minus());

    EXPECT_TRUE(stepped.empty());
}

TEST_F(StepperTest, PlusIsDisabledAtTheUpperEnd) {
    make();
    stepper->set_value(9);
    relayout();

    EXPECT_FALSE(plus()->enabled());
    EXPECT_TRUE(minus()->enabled());

    click(plus());

    EXPECT_TRUE(stepped.empty());
}

TEST_F(StepperTest, ButtonsFollowTheValueAtTheNextLayout) {
    make();
    stepper->set_value(9);

    EXPECT_TRUE(plus()->enabled()) << "the buttons changed before a layout";

    relayout();

    EXPECT_FALSE(plus()->enabled());
}

TEST_F(StepperTest, RangeMovesTheEnds) {
    auto made = build();

    EXPECT_EQ(made->range(0, 3), made.get());

    place(std::move(made));
    stepper->set_value(3);
    relayout();

    EXPECT_FALSE(plus()->enabled());

    stepper->set_value(0);
    relayout();

    EXPECT_FALSE(minus()->enabled());
    EXPECT_TRUE(plus()->enabled());
}

TEST_F(StepperTest, RangeDoesNotClampTheValue) {
    make();
    stepper->range(5, 6);

    EXPECT_EQ(stepper->value(), 1);
}

TEST_F(StepperTest, AStepThatStaysBelowTheRangeIsIgnored) {
    auto made = build();

    made->range(5, 8);
    place(std::move(made));
    click(plus());

    EXPECT_TRUE(stepped.empty()) << "a step to a value outside the range was reported";
}

TEST_F(StepperTest, ClearableMinusAtTheLowerEndReportsTheOffValue) {
    auto made = build();

    EXPECT_EQ(made->clearable(0, "Off"), made.get());

    place(std::move(made));

    EXPECT_TRUE(minus()->enabled());

    click(minus());

    EXPECT_EQ(stepped, (std::vector<int>{0}));
}

TEST_F(StepperTest, AClearedStepperDisablesMinusAndPlusGoesToTheLowerEnd) {
    auto made = build();

    made->range(2, 9)->clearable(0, "Off");
    place(std::move(made));
    stepper->set_value(0);
    relayout();

    EXPECT_FALSE(minus()->enabled());
    EXPECT_TRUE(plus()->enabled());

    click(plus());

    EXPECT_EQ(stepped, (std::vector<int>{2}));
}

TEST_F(StepperTest, AClearedStepperShowsThePlaceholder) {
    auto made = build();

    made->clearable(0, "Off");
    place(std::move(made));
    stepper->set_value(0);

    const std::vector<std::uint32_t> off = field_pixels();

    stepper->clearable(0, "Never");

    EXPECT_NE(field_pixels(), off) << "the placeholder text is not drawn";

    stepper->set_value(1);

    EXPECT_NE(field_pixels(), off);
}

TEST_F(StepperTest, TheFieldShowsTheValue) {
    make();

    const std::vector<std::uint32_t> one = field_pixels();

    stepper->set_value(8);

    EXPECT_NE(field_pixels(), one);

    stepper->set_value(1);

    EXPECT_EQ(field_pixels(), one);
}

TEST_F(StepperTest, ADisabledStepperDisablesBothButtons) {
    make();
    stepper->set_value(5);
    stepper->set_enabled(false);
    relayout();

    EXPECT_FALSE(minus()->enabled());
    EXPECT_FALSE(plus()->enabled());
}

TEST_F(StepperTest, ReturnOnAFocusedButtonSteps) {
    make();
    stepper->set_value(3);
    relayout();
    canvas.root().focus(plus());

    EXPECT_TRUE(canvas.root().key(key(Code::Return)));
    EXPECT_EQ(stepped, (std::vector<int>{4}));

    canvas.root().focus(minus());

    EXPECT_TRUE(canvas.root().key(key(Code::Space)));
    EXPECT_EQ(stepped, (std::vector<int>{4, 2}));
}

TEST_F(StepperTest, TabWalksTheButtons) {
    make();
    stepper->set_value(3);
    relayout();
    canvas.root().focus(minus());
    canvas.root().key(key(Code::Tab));

    EXPECT_EQ(canvas.root().focused(), plus());
}

TEST_F(StepperTest, NaturalWidthIsAtLeast170) {
    auto made = build("");

    EXPECT_GE(made->natural_width(tests::fonts()), 170.0);

    made->fixedWidth = 90.0;

    EXPECT_DOUBLE_EQ(made->natural_width(tests::fonts()), 90.0);
}

TEST_F(StepperTest, TooltipSetsTheHint) {
    make();

    EXPECT_EQ(stepper->tooltip("How many"), stepper);
    EXPECT_EQ(stepper->hint, "How many");
}

TEST_F(StepperTest, AnEmptyCaptionIsHidden) {
    make("");

    EXPECT_DOUBLE_EQ(frame()->box().y, BOX.y);
}

TEST_F(StepperTest, ButtonsSitAtEitherEndOfTheField) {
    make();

    const BLRect &field = frame()->box();

    EXPECT_DOUBLE_EQ(minus()->box().x, field.x + 4.0);
    EXPECT_DOUBLE_EQ(plus()->box().x + plus()->box().w, field.x + field.w - 4.0);
    EXPECT_DOUBLE_EQ(minus()->box().w, 30.0);
    EXPECT_DOUBLE_EQ(field.h, ttk::Theme::control);
}

TEST_F(StepperTest, PartialRepaintsMatchAFullOne) {
    make();
    stepper->set_value(5);
    relayout();
    click(plus());
    canvas.frame_at(1.0);
    stepper->set_value(6);
    canvas.frame_at(2.0);

    EXPECT_TRUE(canvas.matches_full());
}
}
