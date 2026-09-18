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
    // A number between two buttons. Stepping below the range clears it.
    class Stepper : public Box {
    public:
        Stepper(std::string label, std::function<void(int)> stepped);

        Stepper *range(int from, int to);
        Stepper *clearable(int offValue, std::string placeholder);
        Stepper *tooltip(std::string text);

        void set_value(int value);

        [[nodiscard]] int value() const { return _value; }

        void arrange(Typeface &type) override;

        double natural_width(Typeface &type) override;

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
