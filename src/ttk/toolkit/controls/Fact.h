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
    //! Column of a small upper case caption over a value in semibold body type.
    class Fact : public Box {
    public:
        //! Creates a fact with `label` as the caption, styled by \ref Label::section(), and `value` below it.
        Fact(std::string label, std::string value);

        //! Sets the value text to `value`, as \ref Label::set_text() does.
        void set_value(std::string value) const;

        //! Sets whether the value is shown as a file path, as \ref Label::path() does, and returns this fact.
        Fact *path(bool value = true);

        //! Makes the value a link that calls `clicked`, with `tip` as its tooltip, and returns this fact.
        Fact *on_click(std::string tip, std::function<void()> clicked);

    private:
        Label *_caption = nullptr;
        Label *_value = nullptr;
    };
}


#endif //TTK_CONTROLS_FACT_H
