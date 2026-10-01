// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_STEPPER_H
#define TTK_CONTROLS_STEPPER_H


#include <functional>
#include <string>

#include "ttk/toolkit/controls/GlyphButton.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/layout/Box.h"

namespace ttk {
    //! Integer field with a minus button on the left and a plus button on the right.
    //!
    //! The control is a column holding an optional caption above the field. The buttons do not change
    //! \ref value() by themselves. They report the stepped value through the `stepped` callback, and the owner
    //! calls \ref set_value() to accept it. The range is `[1, 9]` and the value 1 until set.
    class Stepper : public Box {
    public:
        //! Creates a stepper with the caption `label`, hidden when empty, that reports steps to `stepped`.
        //!
        //! `stepped` is called with \ref value() plus or minus one when that stays inside the range. A step past the
        //! upper end is ignored. For a step below the lower end, see \ref clearable().
        Stepper(std::string label, std::function<void(int)> stepped);

        //! Sets the range of values the buttons step through to `[from, to]` and returns this.
        //!
        //! \ref value() is not clamped to the new range.
        Stepper *range(int from, int to);

        //! Lets the stepper be cleared by stepping below the lower end of the range, and returns this.
        //!
        //! Minus at the lower end then calls the `stepped` callback with `offValue`. While \ref value() is below
        //! the range, the field shows `placeholder`, minus is disabled and plus calls `stepped` with the lower end.
        //! `offValue` should be below the range for the field to show `placeholder` once it is set.
        Stepper *clearable(int offValue, std::string placeholder);

        //! Sets \ref Widget::hint to `text` and returns this.
        Stepper *tooltip(std::string text);

        //! Sets the value shown in the field without calling the `stepped` callback, and repaints.
        //!
        //! `value` is not clamped to the range. The buttons are enabled or disabled to match at the next layout.
        void set_value(int value);

        //! Returns the value shown in the field.
        [[nodiscard]] int value() const { return _value; }

        //! Lays out the caption and the field, and enables each button only when a step its way is possible.
        void arrange(Typeface &type) override;

        //! Returns \ref Widget::fixedWidth when it is set, otherwise the column's width raised to 170 pixels.
        double natural_width(Typeface &type) override;

        //! Paints the field with the value, or the placeholder while the value is cleared, then the caption and
        //! the buttons.
        void paint(const Painter &painter) override;

    private:
        void step(int by) const;

        [[nodiscard]] bool unset() const { return _clearable && _value < _from; }

        Label *_caption = nullptr;
        GlyphButton *_less = nullptr;
        GlyphButton *_more = nullptr;

        BLRect _frame{};

        int _from = 1;
        int _to = 9;
        int _value = 1;

        bool _clearable = false;
        int _off = 0;
        std::string _placeholder = "Off";

        std::function<void(int)> _stepped;
    };
}


#endif //TTK_CONTROLS_STEPPER_H
