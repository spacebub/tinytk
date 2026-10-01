// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_CHIP_H
#define TTK_CONTROLS_CHIP_H


#include <string>

#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
    //! Small piece of text that shows a tooltip on hover and does nothing when clicked.
    //!
    //! Drawn in small fixed width type by default, such as a token of a command template or a count beside a
    //! title. A background and border appear only on hover or while \ref set_tight() is on.
    class Chip : public Widget {
    public:
        //! Creates a chip showing `text` with `about` as its tooltip, \ref Widget::hint.
        Chip(std::string text, std::string about);

        //! Sets the text to `text` and repaints. The layout is not redone, so a longer text is elided to the
        //! current box until the next layout.
        void set_text(std::string text);

        //! Sets whether the chip is drawn in the warning colours, such as for an exhausted budget, and repaints.
        void set_tight(bool value);

        //! Switches the chip to the proportional face and returns it.
        Chip *plain();

        //! Returns the width of the text plus padding, or \ref fixedWidth when it is set.
        double natural_width(Typeface &type) override;

        //! Returns the line height of the face plus padding, or \ref fixedHeight when it is set.
        double natural_height(Typeface &type, double width) override;

        //! Paints the text, with a background and border while hovered or tight. Paints nothing when the text is
        //! empty.
        void paint(const Painter &painter) override;

    private:
        [[nodiscard]] const BLFont &face(Typeface &type) const;

        std::string _text;

        bool _mono = true;
        bool _tight = false;
    };
}


#endif //TTK_CONTROLS_CHIP_H
