// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/layout/Panel.h"

namespace ttk {
    void Panel::paint(const Painter &painter) {
        const Theme::Palette &palette = Theme::palette();

        painter.round(_box, rounding, inset ? palette.sunken : palette.surface);

        if (hoverable && lit) {
            painter.round(_box, rounding, palette.hover);
        }

        if (bordered) {
            painter.outline(_box, rounding, 1.0, lit ? palette.borderStrong : palette.border);
        }

        Widget::paint(painter);
    }
}
