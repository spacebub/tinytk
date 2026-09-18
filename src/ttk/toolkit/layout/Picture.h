// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_LAYOUT_PICTURE_H
#define TTK_LAYOUT_PICTURE_H


#include "ttk/toolkit/Widget.h"

namespace ttk {
    // A picture at a fixed size, which the interface only ever needs for the mark.
    class Picture : public Widget {
    public:
        explicit Picture(BLImage image) : _image(std::move(image)) {}

        void paint(const Painter &painter) override;

    private:
        BLImage _image;
    };
}


#endif //TTK_LAYOUT_PICTURE_H
