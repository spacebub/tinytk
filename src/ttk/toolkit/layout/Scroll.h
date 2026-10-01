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
    //! View onto a single content widget taller than itself, scrolled vertically with a bar at the right edge.
    //!
    //! The content is laid out at the width of the view and at its own wanted height, shifted up by \ref offset()
    //! and clipped to the view. The bar is drawn over the content in a lane \ref Theme::lane pixels wide rather
    //! than taking width from it, so content should keep a right margin wider than that lane.
    class Scroll : public Widget {
    public:
        Scroll();

        //! Returns the widget given to \ref hold(), or null when there is none.
        [[nodiscard]] Widget *content() const { return _content; }

        //! Destroys all children, then adds `child` as the content and returns it.
        Widget *hold(Ptr child);

        //! Returns the scroll offset in pixels, which is how far the top of the content lies above the top of the
        //! view. Always a whole number between 0 and \ref reach() less the view height.
        [[nodiscard]] double offset() const { return _offset; }

        //! Scrolls to `offset` at once and stops any wheel glide in progress.
        //!
        //! `offset` is clamped to the scrollable range and rounded to whole pixels. When nothing above the view
        //! clips it, the window moves the pixels already painted and repaints only what came into view.
        void scroll_to(double offset);

        //! Scrolls the least distance that brings `wanted`, in window coordinates, into view.
        //!
        //! Does nothing when `wanted` is already fully in view. A box taller than the view ends up with its top
        //! or bottom edge on the edge of the view it was crossing.
        void reveal(const BLRect &wanted);

        //! Returns the height of the content in pixels, which the offset range and the bar are measured against.
        //!
        //! Set by \ref arrange() and \ref refit() while there is content, or by \ref set_reach() otherwise.
        [[nodiscard]] double reach() const { return _reach; }

        //! Sets \ref reach() to `reach` and clamps the offset to the new range.
        //!
        //! Meant for a scroller without content whose list is painted by hand. \ref arrange() replaces the value
        //! whenever there is content.
        void set_reach(double reach);

        //! Tests whether the content is more than a pixel taller than the view. The bar is shown only then.
        [[nodiscard]] bool scrollable() const;

        //! Tests whether the view is within a pixel of the end of the content. Also true for content that fits.
        [[nodiscard]] bool at_end() const;

        //! Measures the content again, lays it out and repaints, for content that grew or shrank on its own.
        //!
        //! A view at the end stays at the end. Otherwise the offset is lowered by `lost`, the height in pixels
        //! removed above the view, so what was on screen stays in place. Does nothing without content.
        void refit(Typeface &type, double lost = 0.0);

        //! Measures the content at the view width, or at its \ref Widget::minWidth when that is wider, clamps the
        //! offset and places the content shifted up by it, at least as tall as the view. Does nothing without
        //! content.
        void arrange(Typeface &type) override;

        //! Stores the view box in `region` and returns true, so the content is clipped to the view.
        bool clips(BLRect &region) const override;

        //! Paints the content, then the bar thumb when \ref scrollable(). The thumb is more opaque while it is dragged.
        void paint(const Painter &painter) override;

        //! Starts or extends a smooth scroll of 52 pixels per notch and returns true. A positive `steps` scrolls
        //! toward the top. Returns false when not \ref scrollable(), so the wheel goes to the parent.
        bool wheel(double steps, const Pointer &at) override;

        //! Handles a press in the bar lane and returns true when it was taken.
        //!
        //! A press on the thumb starts dragging it. A press on the track above or below the thumb scrolls by one
        //! view height. Presses left of the lane, or any press while not \ref scrollable(), are not taken.
        bool press(const Pointer &at) override;

        //! Scrolls to follow the pointer while the thumb is being dragged. Does nothing otherwise.
        void drag(const Pointer &at) override;

        //! Ends a thumb drag started by \ref press().
        void release(const Pointer &at) override;

        //! Returns this scroller for a point in the bar lane while \ref scrollable(), ahead of the content under
        //! it. Otherwise behaves like \ref Widget::at().
        Widget *at(double x, double y) override;

        //! Steps a smooth scroll started by \ref wheel() and returns whether it is still moving.
        bool advance(double now) override;

    private:
        void settle(double offset);

        [[nodiscard]] BLRect lane() const;
        [[nodiscard]] BLRect thumb() const;

        Widget *_content = nullptr;

        double _offset = 0.0;
        double _reach = 0.0;

        // The wheel moves a goal that the offset closes in on by a fixed share per unit time, so quick notches
        // run together into one glide.
        double _goal = 0.0;
        double _via = 0.0;
        double _along = 0.0;
        double _glided = 0.0;
        bool _gliding = false;

        double _grabbed = 0.0;
        double _from = 0.0;
        bool _dragging = false;
    };
}


#endif //TTK_LAYOUT_SCROLL_H
