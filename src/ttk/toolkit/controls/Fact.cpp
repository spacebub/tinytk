// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "ttk/toolkit/controls/Fact.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/layout/Box.h"

namespace ttk {
    Fact::Fact(std::string label, std::string value) : Box(Flow::Column) {
        spacing(3.0);

        _caption = append(std::make_unique<Label>(std::move(label)));
        _caption->section();

        _value = append(std::make_unique<Label>(std::move(value)));
        _value->font(600, Theme::fontBody)->tone(&Theme::Palette::text);
    }

    void Fact::set_value(std::string value) const {
        _value->set_text(std::move(value));
    }

    Fact *Fact::path(const bool value) {
        _value->path(value);

        return this;
    }

    Fact *Fact::on_click(std::string tip, std::function<void()> clicked) {
        _value->on_click(std::move(clicked));
        _value->hint = std::move(tip);

        return this;
    }
}
