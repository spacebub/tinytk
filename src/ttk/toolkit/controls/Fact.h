// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_FACT_H
#define TTK_CONTROLS_FACT_H


#include <functional>
#include <string>

#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/layout/Box.h"

namespace ttk {
    // A small caption over a value, which may be a path that opens.
    class Fact : public Box {
    public:
        Fact(std::string label, std::string value);

        void set_value(std::string value) const;

        Fact *path(bool value = true);
        Fact *on_click(std::string tip, std::function<void()> clicked);

    private:
        Label *_caption = nullptr;
        Label *_value = nullptr;
    };
}


#endif //TTK_CONTROLS_FACT_H
