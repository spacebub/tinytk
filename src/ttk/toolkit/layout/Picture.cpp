// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "ttk/toolkit/layout/Picture.h"

namespace ttk {
    void Picture::paint(const Painter &painter) {
        if (!_image.is_empty()) {
            painter.context().blit_image(_box, _image);
        }
    }
}
