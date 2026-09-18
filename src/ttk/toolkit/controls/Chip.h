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
    // A small box of type that explains itself on hover and does nothing else: the
    // tokens a custom command is written with, or a count beside a panel's title.
    class Chip : public Widget {
    public:
        Chip(std::string text, std::string about);

        void set_text(std::string text);

        // Drawn in the warning tone, for a budget with nothing left in it.
        void set_tight(bool value);

        // The proportional face, rather than the fixed width one a token wants.
        Chip *plain();

        double natural_width(Typeface &type) override;
        double natural_height(Typeface &type, double width) override;

        void paint(const Painter &painter) override;

    private:
        [[nodiscard]] const BLFont &face(Typeface &type) const;

        std::string _text;

        bool _mono = true;
        bool _tight = false;
    };
}


#endif //TTK_CONTROLS_CHIP_H
