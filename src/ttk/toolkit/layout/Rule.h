// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_LAYOUT_RULE_H
#define TTK_LAYOUT_RULE_H


#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Painter.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
    // The hairline a panel divides itself with.
    class Rule : public Widget {
    public:
        Rule() { fixedHeight = 1.0; }

        void paint(const Painter &painter) override {
            painter.fill(BLRect{_box.x, _box.y, _box.w, 1.0}, Theme::palette().border);
        }
    };
}


#endif //TTK_LAYOUT_RULE_H
