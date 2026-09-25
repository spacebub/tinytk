// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_LAYOUT_SCROLL_H
#define TTK_LAYOUT_SCROLL_H


#include "ttk/toolkit/Widget.h"

namespace ttk {
    // A view onto something taller than itself, with a bar down the right. The content
    // is a single child, placed at a negative offset and clipped.
    class Scroll : public Widget {
    public:
        Scroll();

        [[nodiscard]] Widget *content() const { return _content; }

        // Takes ownership and becomes the one thing scrolled.
        Widget *hold(Ptr child);

        [[nodiscard]] double offset() const { return _offset; }

        // Lands there at once, and stops a glide on its way somewhere else.
        void scroll_to(double offset);

        // Brings a box in the content into view, moving as little as possible.
        void reveal(const BLRect &wanted);

        // How tall the content turned out, which is what the bar is measured against.
        [[nodiscard]] double reach() const { return _reach; }

        // For a list drawn by hand rather than held as a child.
        void set_reach(double reach);

        [[nodiscard]] bool scrollable() const;

        // True with the last of the content in view, and for content that all fits.
        [[nodiscard]] bool at_end() const;

        // Measures the content again where it stands, for one that grew or shrank on
        // its own. A view at the end stays at the end. `lost` is how much came off the
        // top, which the offset follows so what was on screen stays there.
        void refit(Typeface &type, double lost = 0.0);

        void arrange(Typeface &type) override;

        bool clips(BLRect &region) const override;

        void paint(const Painter &painter) override;

        bool wheel(double steps, const Pointer &at) override;

        bool press(const Pointer &at) override;
        void drag(const Pointer &at) override;
        void release(const Pointer &at) override;

        Widget *at(double x, double y) override;

        bool advance(double now) override;

    private:
        void settle(double offset);

        [[nodiscard]] BLRect lane() const;
        [[nodiscard]] BLRect thumb() const;

        Widget *_content = nullptr;

        double _offset = 0.0;
        double _reach = 0.0;

        // The wheel moves a goal. The offset closes in on it a fixed share per unit
        // time, so turns run together rather than each starting a curve of its own.
        double _goal = 0.0;
        double _via = 0.0;
        double _along = 0.0;
        double _glided = 0.0;
        bool _gliding = false;

        // Where the thumb was when it was grabbed.
        double _grabbed = 0.0;
        double _from = 0.0;
        bool _dragging = false;
    };
}


#endif //TTK_LAYOUT_SCROLL_H
