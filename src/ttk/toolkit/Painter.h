// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_TOOLKIT_PAINTER_H
#define TTK_TOOLKIT_PAINTER_H


#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <blend2d/blend2d.h>

#include "ttk/draw/Typeface.h"

namespace ttk {
    //! One line of a wrapped run, with the byte range of the run it came from.
    struct Fold {
        //! Text of the line in UTF-8, without the space or newline it broke at.
        std::string text;
        //! Byte offset in the run where the line starts.
        size_t from = 0;
        //! Byte offset in the run just past the line's last character. The space or newline it broke at is outside.
        size_t to = 0;
    };

    //! Returns `run` wrapped into lines at most `room` pixels wide in `font`.
    //!
    //! Lines break at spaces, and a newline always ends a line. A word wider than `room` is cut at the last UTF-8
    //! character that fits, keeping at least one character per line. An empty run gives a single empty line.
    //! The lines are kept in a cache shared by every caller, so copy them to keep them past the next call.
    const std::vector<Fold> &fold_spans(Typeface &type, const BLFont &font, std::string_view run,
                                       double room);

    //! Returns the height of `run` wrapped by \ref fold_spans() at `room` pixels wide, or 0 for an empty run.
    double wrap_height(Typeface &type, const BLFont &font, std::string_view run, double room);

    //! Horizontal placement of text inside a box.
    enum class Align : std::uint8_t {
        //! Against the left edge.
        Start,
        //! Centred across the box.
        Centre,
        //! Against the right edge.
        End,
    };

    //! Drawing interface handed to \ref Widget::paint(): shapes, text and clipping over a Blend2D context.
    //!
    //! A painter is made for one damaged rectangle at a time, \ref clip(), and the context is already clipped to it.
    //! All coordinates are in window pixels and all text is UTF-8. \ref fill(), \ref round(), \ref outline() and
    //! \ref circle() draw nothing for an empty box or a fully transparent `tone`.
    class Painter {
    public:
        //! Creates a painter drawing into `context` with fonts from `type`, for the damaged rectangle `clip`.
        Painter(BLContext &context, Typeface &type, const BLRectI &clip)
            : _context(context), _type(type), _clip(clip) {}

        //! Returns the Blend2D context, for drawing the painter does not cover. Restore any state changed on it.
        BLContext &context() const { return _context; }

        //! Returns the typeface fonts are taken from.
        Typeface &type() const { return _type; }

        //! Returns the rectangle being painted, narrowed by \ref push() while one is in effect.
        const BLRectI &clip() const { return _clip; }

        //! Tests whether any of `box` lies inside \ref clip(). Use it to skip painting what is not being repainted.
        bool needed(const BLRect &box) const;

        //! Fills `box` with `tone`.
        void fill(const BLRect &box, BLRgba32 tone) const;
        //! Fills `box` with `tone` with its corners rounded by `radius`.
        //!
        //! `radius` is clamped to half the shorter side, and a non-positive `radius` fills a plain rectangle.
        void round(const BLRect &box, double radius, BLRgba32 tone) const;

        //! Draws a border `width` thick with outer corner radius `radius`, inside `box` rather than straddling its
        //! edge.
        //!
        //! Draws nothing unless `box` is wider and taller than twice `width`.
        void outline(const BLRect &box, double radius, double width, BLRgba32 tone) const;

        //! Fills a circle of `radius` around `centre`. Draws nothing when `radius` is not positive.
        void circle(BLPoint centre, double radius, BLRgba32 tone) const;

        //! Fills `shape` with `tone`.
        void path(const BLPath &shape, BLRgba32 tone) const;
        //! Strokes `shape` `width` thick with round caps and joins.
        //!
        //! The stroke width, caps and join stay set on \ref context() afterwards.
        void stroke(const BLPath &shape, double width, BLRgba32 tone) const;

        //! Returns the font of `weight` at `size` from \ref type(). The font is owned by the typeface.
        const BLFont &font(int weight, float size) const;

        //! Returns the advance width of `run` in `font`, in pixels.
        double width(const BLFont &font, std::string_view run) const;
        //! Returns the height of one line of `font`, its ascent plus its descent.
        double line_height(const BLFont &font) const;

        //! Returns `run` shortened to fit `room` pixels in `font`, ending in an ellipsis where anything was cut.
        //! Returns `run` unchanged when it fits.
        std::string elide(const BLFont &font, std::string_view run, double room) const;

        //! Draws `run` on one line with `top` at the top-left of the line box. The baseline comes from the font.
        //!
        //! The run is neither wrapped nor elided. Draws nothing for an empty run.
        void text(const BLFont &font, BLPoint top, std::string_view run, BLRgba32 tone) const;

        //! Draws `run` on one line vertically centred in `box` and placed across it by `align`.
        //!
        //! The run is elided to `box.w` first. Draws nothing for an empty run or a box with no width.
        void label(const BLFont &font, const BLRect &box, Align align, std::string_view run,
                   BLRgba32 tone) const;

        //! Draws `run` against the left edge of `box`, vertically centred and cut with an ellipsis at its right edge.
        //!
        //! Meant for the rows of a long list. Rows are kept in a cache of their own, so scrolling through many of them
        //! does not evict what \ref label() and \ref text() keep. Draws nothing for an empty run or a box with no
        //! width.
        void row(const BLFont &font, const BLRect &box, std::string_view run, BLRgba32 tone) const;

        //! Draws `run` letter-spaced, adding `spacing` pixels after each character, with `top` at the top-left of the
        //! line box. The run is neither wrapped nor elided.
        void tracked(const BLFont &font, BLPoint top, std::string_view run, BLRgba32 tone,
                     double spacing) const;

        //! Draws `run` wrapped by \ref fold_spans() at `box.w` from the top of `box` down and returns the height used.
        //!
        //! `box.h` is ignored and nothing is cut at the bottom. An empty run draws nothing but still returns the height
        //! of one line.
        double paragraph(const BLFont &font, const BLRect &box, std::string_view run,
                         BLRgba32 tone) const;

        //! Returns the height of `run` wrapped at `room` pixels wide in `font`, or 0 for an empty run.
        double wrap_height(const BLFont &font, std::string_view run, double room) const;

        //! Narrows \ref clip() and the context's clip to their intersection with `box` rounded out to whole pixels.
        //!
        //! Saves the context state first. Every call must be matched by \ref pop(), and calls nest.
        void push(const BLRect &box) const;
        //! Restores the context state and the \ref clip() saved by the matching \ref push().
        void pop() const;

    private:
        BLContext &_context;
        Typeface &_type;

        mutable BLRectI _clip;
        mutable std::vector<BLRectI> _held;

        // Kept between outlines, so one allocates nothing.
        mutable BLPath _ring;
    };
}


#endif //TTK_TOOLKIT_PAINTER_H
