// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_LAYOUT_WRAP_H
#define TTK_LAYOUT_WRAP_H


#include "ttk/toolkit/Widget.h"

namespace ttk {
    // A row that starts another line rather than run past its width.
    class Wrap : public Widget {
    public:
        Wrap *spacing(double across, double down);

        double natural_width(Typeface &type) override;
        double natural_height(Typeface &type, double width) override;

        void arrange(Typeface &type) override;

    private:
        double lay(Typeface &type, double width, bool place) const;

        double _across = 8.0;
        double _down = 8.0;
    };
}


#endif //TTK_LAYOUT_WRAP_H
