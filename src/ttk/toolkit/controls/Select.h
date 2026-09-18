// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_SELECT_H
#define TTK_CONTROLS_SELECT_H


#include <functional>
#include <string>
#include <vector>

#include "ttk/draw/Anim.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/layout/Box.h"

namespace ttk {
    // The list lives on the root's popup layer while it is open rather than under the
    // box, so nothing it covers has to know about it.
    class Select : public Box {
    public:
        Select(std::string label, std::function<void(int)> selected);

        void set_options(std::vector<std::string> options);

        // Per option. Short of the list or empty is none.
        void set_badges(std::vector<std::string> badges);

        void set_current(int index);

        [[nodiscard]] int current() const { return _current; }

        Select *placeholder(std::string text);

        // Offers the placeholder as a row of its own, which clears the value.
        Select *clearable(bool value = true);

        Select *tooltip(std::string text);

        [[nodiscard]] bool open() const { return _list != nullptr; }

        void close();

        void arrange(Typeface &type) override;

        double natural_width(Typeface &type) override;

        void paint(const Painter &painter) override;

        bool press(const Pointer &at) override;
        void release(const Pointer &at) override;
        void enter() override;
        void leave() override;

        [[nodiscard]] bool takes_focus() const override { return enabled(); }
        bool key(const Key &pressed) override;

        bool advance(double now) override;

        Widget *at(double x, double y) override;

    private:
        void show();

        Label *_caption = nullptr;

        // The box, worked out at arrange time.
        BLRect _frame{};

        std::vector<std::string> _options;
        std::vector<std::string> _badges;

        std::string _placeholder = "(Default)";

        int _current = -1;
        bool _clearable = false;

        std::function<void(int)> _selected;

        // Owned by the popup layer while it is up.
        Widget *_list = nullptr;

        Anim::Tween _lit;
        Anim::Tween _turn;
    };
}


#endif //TTK_CONTROLS_SELECT_H
