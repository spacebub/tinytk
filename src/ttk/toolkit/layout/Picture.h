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
    //! Widget that draws an image scaled to its box.
    //!
    //! The picture does not size itself from the image, so give it a \ref fixedWidth and a \ref fixedHeight.
    class Picture : public Widget {
    public:
        //! Creates a picture that draws `image`.
        explicit Picture(BLImage image) : _image(std::move(image)) {}

        //! Draws the image stretched to \ref box(). Draws nothing for an empty image, and never paints children.
        void paint(const Painter &painter) override;

    private:
        BLImage _image;
    };
}


#endif //TTK_LAYOUT_PICTURE_H
