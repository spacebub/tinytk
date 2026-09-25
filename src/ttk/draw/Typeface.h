// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_DRAW_TYPEFACE_H
#define TTK_DRAW_TYPEFACE_H


#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <blend2d/blend2d.h>

namespace ttk {
    // Type, such as Blend2D gives it: a face off the filesystem, a size, and glyphs
    // filled as paths. There is no font engine to configure and no atlas, but also
    // no hinting, and nothing here goes anywhere near DirectWrite.
    class Typeface {
    public:
        // Weights, as the application asks for them.
        static constexpr int regular = 400;
        static constexpr int semibold = 600;
        static constexpr int bold = 700;

        // The fixed width face, for paths, command lines and run output.
        static constexpr int mono = 1;
        static constexpr int monoBold = 2;

        // The weight to ask at() for, given what a label wants.
        static int pick(int weight, bool fixed);

        // False when the platform has no face this can find, which the caller turns
        // into a clean exit rather than a crash.
        bool load();

        const BLFont &at(int weight, float size);

        // A run is shaped once and kept, and rasterised once into a mask the first time
        // it is drawn: a label is measured, elided and drawn every paint, and Blend2D
        // would otherwise shape it and fill every glyph outline again each time.

        // The advance width, which is what a layout needs. The ink may be narrower.
        float width(const BLFont &font, std::string_view run);

        // The same without keeping the run: for a candidate that is measured once and
        // never drawn, which would otherwise crowd the drawn runs out of the cache.
        float width_once(const BLFont &font, std::string_view run);

        // `run` shortened until it fits, with an ellipsis where anything was dropped.
        // Measured with the tracking it will be drawn with, when there is any.
        std::string elide(const BLFont &font, std::string_view run, float room,
                          float tracking = 0.0F);

        // `top` is the top of the line box. The baseline is worked out from the face.
        void draw(BLContext &context, const BLFont &font, BLPoint top, std::string_view run,
                  BLRgba32 tone);

        // Letter-spaced, which fill_utf8_text has no notion of: the run is drawn one
        // character at a time with the tracking added to each advance.
        float width_tracked(const BLFont &font, std::string_view run, float tracking);

        void draw_tracked(BLContext &context, const BLFont &font, BLPoint top, std::string_view run,
                         BLRgba32 tone, float tracking);

        // Centred vertically inside a box `height` tall starting at `top`.
        void draw_centred(BLContext &context, const BLFont &font, BLPoint top, float height,
                         std::string_view run, BLRgba32 tone);

        // Laid down glyph by glyph off a cache of glyph masks, and kept nowhere: for a
        // line drawn once and scrolled past, where a mask of the whole line would be
        // made only to be thrown away. Cut with an ellipsis where it would pass `room`.
        void draw_once(BLContext &context, const BLFont &font, BLPoint top, std::string_view run,
                       BLRgba32 tone, double room);

        // The byte offset in `run`, on a character boundary, nearest `x` across it.
        size_t nearest(const BLFont &font, std::string_view run, float x);

        [[nodiscard]] float line_height(const BLFont &font) const;

    private:
        struct Shaped {
            BLGlyphBuffer buffer;
            size_t used = 0;
            float width = 0.0F;

            // The ink, relative to the origin on the baseline.
            BLBox ink{};

            // Coverage only. The tone is applied when it is laid down.
            BLImage mask;
            BLPointI maskAt{};
            bool masked = false;
        };

        struct Glyph {
            // Coverage, in rows of `stride` bytes: a multiple of the chunk a row of it
            // is merged in, and zero past its width so the merge needs no edge.
            std::vector<std::uint8_t> pixels;
            int stride = 0;
            int wide = 0;
            int tall = 0;

            // The mask's top left, relative to the glyph's origin on the baseline.
            BLPointI at{};
            bool made = false;
        };

        // One font's glyphs, by id and by which fraction of a pixel each was
        // rasterised across, so a run keeps its fractional advances without each
        // glyph landing blurred.
        struct Glyphs {
            std::vector<Glyph> held;
            size_t used = 0;
        };

        // A line composed from glyphs, kept for the frames it stays on screen through.
        struct Row {
            BLImage mask;
            BLPointI at{};
            int wide = 0;
            int tall = 0;
            size_t used = 0;
        };

        Shaped &shaped(const BLFont &font, std::string_view run);

        Glyphs &glyphs(const BLFont &font);

        static void make(const BLFont &font, std::uint32_t id, std::uint32_t shift, Glyph &into);

        // Where a run in `_scratch` is cut to fit, with the ellipsis that marks it.
        struct Cut {
            size_t shown = 0;
            bool made = false;
            std::uint32_t dot = 0;
            double dotWide = 0.0;
        };

        // Shapes `run` into `_scratch`, its pens into `_pens`, and finds the cut.
        Cut shape_to(const BLFont &font, std::string_view run, double room);

        // Lays the run in `_scratch`, up to its cut, into a mask of its own.
        void compose(const BLFont &font, const Cut &cut, double room, Row &into);

        std::string elide_once(const BLFont &font, std::string_view run, float room, float tracking);

        // Lays a run down at `origin` on the baseline, from its mask.
        void lay(BLContext &context, const BLFont &font, Shaped &made, BLPoint origin,
                 BLRgba32 tone);

        // Faces are held by weight. A size makes a BLFont out of one.
        std::map<int, BLFontFace> _faces;
        std::map<long long, BLFont> _fonts;

        // Keyed by the font's address, which the map above keeps still, and the run.
        static_assert(std::is_same_v<decltype(_fonts), std::map<long long, BLFont>>,
                      "shaped runs are keyed by font address; the fonts must not move");

        struct Elided {
            std::string text;
            size_t used = 0;
        };

        std::unordered_map<std::string, Shaped> _shaped;
        std::unordered_map<std::string, Elided> _elided;
        std::unordered_map<std::uintptr_t, Glyphs> _glyphs;

        // Bumped on every lookup. What tells the two caches which entries are cold.
        size_t _asked = 0;
        size_t _maskBytes = 0;

        // Kept between calls: measuring allocates nothing per candidate.
        BLGlyphBuffer _scratch;
        std::vector<size_t> _cuts;
        std::vector<double> _pens;

        std::unordered_map<std::string, Row> _rows;
    };
}


#endif //TTK_DRAW_TYPEFACE_H
