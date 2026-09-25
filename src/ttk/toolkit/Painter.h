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
    // One folded line, and where it came from in the run it was folded out of, so a
    // selection over the lines can be read back off the run itself.
    struct Fold {
        std::string text;
        size_t from = 0;
        size_t to = 0;
    };

    // Word wrapping, as much of it as the interface asks for: break on spaces and on
    // newlines, and a word wider than the line is broken wherever it lands.
    const std::vector<Fold> &fold_spans(Typeface &type, const BLFont &font, std::string_view run,
                                       double room);

    // How tall `run` wraps to at `room` wide.
    double wrap_height(Typeface &type, const BLFont &font, std::string_view run, double room);

    enum class Align : std::uint8_t {
        Start,
        Centre,
        End,
    };

    // The drawing vocabulary of the interface: a rectangle, a run of type, a path and
    // an icon, since a rasteriser offers none of them and a widget needs all four.
    class Painter {
    public:
        Painter(BLContext &context, Typeface &type, const BLRectI &clip)
            : _context(context), _type(type), _clip(clip) {}

        BLContext &context() const { return _context; }

        Typeface &type() const { return _type; }

        const BLRectI &clip() const { return _clip; }

        // False when nothing of `box` is in the damaged rectangle being painted.
        bool needed(const BLRect &box) const;

        void fill(const BLRect &box, BLRgba32 tone) const;
        void round(const BLRect &box, double radius, BLRgba32 tone) const;

        // Drawn inside the box rather than straddling its edge.
        void outline(const BLRect &box, double radius, double width, BLRgba32 tone) const;

        void circle(BLPoint centre, double radius, BLRgba32 tone) const;

        void path(const BLPath &shape, BLRgba32 tone) const;
        void stroke(const BLPath &shape, double width, BLRgba32 tone) const;

        const BLFont &font(int weight, float size) const;

        double width(const BLFont &font, std::string_view run) const;
        double line_height(const BLFont &font) const;

        std::string elide(const BLFont &font, std::string_view run, double room) const;

        // Baseline worked out from the face. `top` is the top of the line box.
        void text(const BLFont &font, BLPoint top, std::string_view run, BLRgba32 tone) const;

        // Vertically centred in `box`, and placed across it by `align`. Elided to fit.
        void label(const BLFont &font, const BLRect &box, Align align, std::string_view run,
                   BLRgba32 tone) const;

        // One row of many: vertically centred in `box` and cut with an ellipsis at its
        // right edge, laid down from glyph masks and kept nowhere, so a thousand rows
        // scrolled past cost the label cache nothing.
        void row(const BLFont &font, const BLRect &box, std::string_view run, BLRgba32 tone) const;

        void tracked(const BLFont &font, BLPoint top, std::string_view run, BLRgba32 tone,
                     double spacing) const;

        // Wrapped at `box.w`, from the top. Answers the height it took.
        double paragraph(const BLFont &font, const BLRect &box, std::string_view run,
                         BLRgba32 tone) const;

        // How tall `run` wraps to at `room` wide, drawing nothing.
        double wrap_height(const BLFont &font, std::string_view run, double room) const;

        // Narrows both the rasteriser's clip and the rectangle `needed` answers for,
        // so a row scrolled out of view is skipped rather than drawn and thrown away.
        void push(const BLRect &box) const;
        void pop() const;

    private:
        BLContext &_context;
        Typeface &_type;

        mutable BLRectI _clip;
        mutable std::vector<BLRectI> _held;
    };
}


#endif //TTK_TOOLKIT_PAINTER_H
