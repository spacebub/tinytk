// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_DRAW_DAMAGE_H
#define TTK_DRAW_DAMAGE_H


#include <vector>

#include <blend2d/blend2d.h>

namespace ttk {
    //! Set of rectangles of a frame that changed and must be repainted and presented.
    //!
    //! All rectangles are in pixels and clipped to the frame size given to \ref resize(). Until then the frame has no
    //! pixels, and every rectangle is dropped.
    class Damage {
    public:
        //! Maximum number of rectangles retained by \ref add().
        static constexpr size_t CROWDED = 32;

        //! Clean pixels \ref add() repaints to join two rectangles rather than keep them apart, each costing a walk
        //! of the widget tree.
        static constexpr int64_t WALK = int64_t{128} * 128;

        //! Sets the frame size to `width` by `height` pixels and clears all rectangles.
        void resize(int width, int height);

        //! Adds `region` to the set.
        //!
        //! `region` is first clipped to the frame and rounded outward to whole pixels. It is dropped when nothing
        //! of it is left or a rectangle already in the set covers it, and otherwise replaces the rectangles it
        //! covers. It is joined with the rectangle whose bounding rectangle with it adds the fewest clean pixels,
        //! when that is at most \ref WALK or the set already holds \ref CROWDED rectangles, and the join goes on
        //! while it brings another rectangle within \ref WALK.
        void add(const BLRect &region);

        //! Replaces the set with a single rectangle covering the whole frame, or empties it when the frame has no
        //! pixels.
        void all();

        //! Also damages where the parts of pending rectangles inside `region` land when its pixels move down by
        //! `dy` pixels, or up when `dy` is negative. The pending rectangles stay, and the strip the move uncovers is
        //! left for the caller to damage.
        void shift(const BLRectI &region, int dy);

        //! Removes all rectangles.
        void clear() { _regions.clear(); }

        //! Tests whether the set holds no rectangles.
        [[nodiscard]] bool empty() const { return _regions.empty(); }

        //! Tests whether a single rectangle in the set covers the whole frame.
        [[nodiscard]] bool whole() const;

        //! Returns the rectangles in the set.
        [[nodiscard]] const std::vector<BLRectI> &regions() const { return _regions; }

        //! Returns `region` rounded outward to whole pixels and clipped to the frame, or an empty rectangle when
        //! none of it is inside the frame.
        [[nodiscard]] BLRectI clamp_to(const BLRect &region) const;

    private:
        std::vector<BLRectI> _regions;

        // Scratch for shift(), kept so a frame allocates nothing.
        std::vector<BLRectI> _moving;

        int _width = 0;
        int _height = 0;
    };
}


#endif //TTK_DRAW_DAMAGE_H
