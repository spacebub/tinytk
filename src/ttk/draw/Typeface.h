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
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <blend2d/blend2d.h>

#include "ttk/util/Cache.h"

namespace ttk {
//! Font faces loaded from system font files, and cached shaping, measuring and drawing of UTF-8 text runs.
//!
//! One face is loaded per weight and glyphs are filled by Blend2D without hinting or fallback fonts. All sizes
//! and positions are in pixels. A run is shaped once and kept, keyed by its font's address and its text, and
//! drawn from a coverage mask made on its first draw whenever the context only translates by whole pixels.
//! Every method that takes a `font` expects one returned by \ref at() of the same typeface.
class Typeface {
public:
    //! Weight of regular text.
    static constexpr int regular = 400;
    //! Weight of semibold text.
    static constexpr int semibold = 600;
    //! Weight of bold text.
    static constexpr int bold = 700;

    //! Weight that selects the regular monospace face, for paths, command lines and program output.
    static constexpr int mono = 1;
    //! Weight that selects the bold monospace face.
    static constexpr int monoBold = 2;

    //! A stretch of a line drawn in a colour of its own by the \ref draw_once() that takes bands.
    struct Band {
        //! Pixels from the start of the line to where the stretch begins.
        double from = 0.0;
        //! Pixels from the start of the line to where the stretch ends.
        double to = 0.0;
        //! Colour of the stretch.
        BLRgba32 tone{};
    };

    //! Returns `weight` when `fixed` is false. Otherwise returns \ref monoBold for a `weight` of \ref semibold
    //! or more and \ref mono below it.
    static int pick(int weight, bool fixed);

    //! Loads a face for each of \ref regular, \ref semibold, \ref bold, \ref mono and \ref monoBold.
    //!
    //! Each weight takes the first file that loads from a list of well-known system font paths. A weight with
    //! none takes the loaded face of the lowest weight value, which is a monospace face when one loaded. Returns
    //! false when no face loads at all.
    bool load();

    //! Returns the font of `weight` at `size` pixels, created on first use.
    //!
    //! The font keeps its address for the life of the typeface. Sizes are told apart to a quarter pixel.
    //! `weight` must be one of the five weight constants: any other value, or a call before \ref load(), gives
    //! a font with no face, and that font stays cached.
    const BLFont &at(int weight, float size);

    //! Returns the advance width of `run`, or 0 when it is empty. The ink may be narrower or wider.
    //!
    //! The shaped run is kept for later measuring and drawing.
    float width(const BLFont &font, std::string_view run);

    //! Returns the advance width of `run` without keeping the shaped run, or 0 when it is empty.
    //!
    //! Use it for text measured once and never drawn, which would otherwise push drawn runs out of the cache.
    float width_once(const BLFont &font, std::string_view run);

    //! Returns the advance width of `word`, or 0 when it is empty, kept in a cache of words apart from shaped runs.
    //!
    //! Use it for wrapping, which measures the same words again at every width a paragraph is laid out at.
    float width_word(const BLFont &font, std::string_view word);

    //! Returns `run` unchanged when it fits in `room`, otherwise its longest prefix that fits with an ellipsis
    //! after it, cut on a character boundary.
    //!
    //! Returns an empty string when not even the ellipsis fits. A positive `tracking` measures as
    //! \ref width_tracked() does. Results are cached by font, run, `room` to a quarter pixel and `tracking`.
    //! The string returned is the cache entry, valid until it is evicted, which takes thousands of further calls.
    //! Copy it to keep it longer.
    const std::string &elide(const BLFont &font, std::string_view run, float room, float tracking = 0.0F);

    //! Draws `run` in `tone` with the top-left of its line box at `top`. Does nothing when `run` is empty.
    //!
    //! The baseline is `top.y` plus the font's ascent, rounded to a whole pixel. When the run is drawn from its
    //! mask, `top.x` is rounded to a whole pixel too.
    void draw(BLContext &context, const BLFont &font, BLPoint top, std::string_view run, BLRgba32 tone);

    //! Returns the width of `run` drawn with `tracking` pixels between neighbouring characters, or 0 when it is
    //! empty.
    //!
    //! Each character is measured on its own, so kerning and ligatures across characters do not apply.
    float width_tracked(const BLFont &font, std::string_view run, float tracking);

    //! Draws `run` in `tone` one character at a time, adding `tracking` pixels after each advance, with the
    //! top-left of its line box at `top` as \ref draw() places it.
    void draw_tracked(
            BLContext &context, const BLFont &font, BLPoint top, std::string_view run, BLRgba32 tone, float tracking);

    //! Draws `run` in `tone` centred vertically in a box `height` tall whose top-left is `top`. Does nothing when
    //! `run` is empty.
    //!
    //! The run sits on the \ref baseline() of the box's middle, which centres the font's capitals in the box.
    void draw_centred(
            BLContext &context, const BLFont &font, BLPoint top, float height, std::string_view run, BLRgba32 tone);

    //! Draws `run` in `tone` with the top-left of its line box at `top`, cut with an ellipsis where it would pass
    //! `room` pixels. Does nothing when `run` is empty or `room` is not positive.
    //!
    //! The shaped run is not kept. When the context only translates by whole pixels, the line is composed from
    //! cached glyph masks at quarter-pixel positions, and the composed line is kept for the few hundred lines
    //! drawn most recently. Use it for long text such as logs, where lines are drawn and scrolled past.
    void
    draw_once(BLContext &context, const BLFont &font, BLPoint top, std::string_view run, BLRgba32 tone, double room);

    //! Draws `run` as the \ref draw_once() above does, with each of `bands` in its own colour and the rest in `tone`.
    //!
    //! `bands` are in order and do not overlap. A pixel column belongs to the band its centre falls in, so a band
    //! that starts and ends on character boundaries of a fixed-width face colours whole characters. Drawn from the
    //! same composed line as a run without bands, which costs one blit per band.
    void draw_once(BLContext &context,
                   const BLFont &font,
                   BLPoint top,
                   std::string_view run,
                   BLRgba32 tone,
                   double room,
                   std::span<const Band> bands);

    //! Returns the byte offset in `run` of the character boundary nearest `x` pixels from its start.
    //!
    //! Returns 0 for an empty `run` and the size of `run` when `x` is nearest its end.
    size_t nearest(const BLFont &font, std::string_view run, float x);

    //! Returns the ascent plus the descent of `font`.
    [[nodiscard]] float line_height(const BLFont &font) const;

    //! Returns how far the capitals of `font` reach above the baseline: the face's cap height, or for a face
    //! that gives none, the top of its H. Kept per face.
    float caps(const BLFont &font);

    //! Returns the baseline that centres the capitals of `font` on `middle`, rounded to a whole pixel.
    //!
    //! Faces differ in how their capitals sit in their line box, so a line mixing faces sets every part on the
    //! baseline of its main face.
    double baseline(const BLFont &font, double middle);

    //! Returns how far the ink of `font` can reach outside a line box of \ref line_height(), in pixels.
    //!
    //! Covers glyphs that rise above the ascent or fall below the descent, the pixel of room each way left for
    //! antialiasing and the rounding of the baseline to a whole pixel. Pad each side of a line's box by it before
    //! testing it against a clip.
    [[nodiscard]] static double bleed(const BLFont &font);

private:
    struct Shaped {
        BLGlyphBuffer buffer;
        size_t used = 0;
        float width = 0.0F;

        BLBox ink{};

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

    struct Row {
        BLImage mask;
        BLPointI at{};
        int wide = 0;
        int tall = 0;
        size_t used = 0;
    };

    Shaped &shaped(const BLFont &font, std::string_view run);

    struct MaskRows {
        int above = 0;
        int below = 0;
    };

    // One answer for lay(), compose() and bleed(), which must agree on how far a mask reaches.
    static MaskRows mask_rows(const BLFont &font);

    Glyphs &glyphs(const BLFont &font);

    static void make(const BLFont &font, std::uint32_t id, std::uint32_t shift, Glyph &into);

    struct Cut {
        size_t shown = 0;
        bool made = false;
        std::uint32_t dot = 0;
        double dotWide = 0.0;
    };

    Cut shape_to(const BLFont &font, std::string_view run, double room);

    // Lays the run in `_scratch`, up to its cut, into a mask of its own.
    void compose(const BLFont &font, const Cut &cut, double room, Row &into);

    Row &composed(const BLFont &font, std::string_view run, double room);

    std::string elide_once(const BLFont &font, std::string_view run, float room, float tracking);

    void lay(BLContext &context, const BLFont &font, Shaped &made, BLPoint origin, BLRgba32 tone);

    std::map<int, BLFontFace> _faces;
    std::map<long long, BLFont> _fonts;

    // Keyed by the font's address, which the map above keeps still, and the run.
    static_assert(std::is_same_v<decltype(_fonts), std::map<long long, BLFont>>,
                  "shaped runs are keyed by font address; the fonts must not move");

    struct Elided {
        std::string text;
        size_t used = 0;
    };

    struct Measured {
        float width = 0.0F;
        size_t used = 0;
    };

    Cache::RunMap<Shaped> _shaped;
    Cache::RunMap<Elided> _elided;
    Cache::RunMap<Measured> _words;
    std::unordered_map<std::uintptr_t, Glyphs> _glyphs;
    // The top of the H in design units, for faces that give no cap height.
    std::unordered_map<std::uint64_t, float> _capitals;

    // Bumped on every lookup. Every cache reads it to tell which entries are cold.
    size_t _asked = 0;
    size_t _maskBytes = 0;

    // Kept between calls: measuring allocates nothing per candidate.
    BLGlyphBuffer _scratch;
    std::vector<size_t> _cuts;
    std::vector<double> _pens;
    std::vector<BLBoxI> _bounds;

    Cache::RunMap<Row> _rows;
};
}


#endif //TTK_DRAW_TYPEFACE_H
