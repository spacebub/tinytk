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
    //! All rectangles are in pixels and clipped to the frame size given to \ref resize().
    class Damage {
    public:
        //! Number of rectangles above which \ref add() merges all of them into their bounding rectangle.
        static constexpr size_t CROWDED = 12;

        //! Sets the frame size to `width` by `height` pixels and clears all rectangles.
        void resize(int width, int height);

        //! Adds `region` to the set.
        //!
        //! `region` is first clipped to the frame and rounded outward to whole pixels. It is dropped when nothing
        //! of it is left, or when a rectangle already in the set covers it. Once the set holds more than
        //! \ref CROWDED rectangles, they are replaced by their bounding rectangle.
        void add(const BLRect &region);

        //! Replaces the set with a single rectangle covering the whole frame.
        void all();

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

        int _width = 0;
        int _height = 0;
    };
}


#endif //TTK_DRAW_DAMAGE_H
