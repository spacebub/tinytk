// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

#if defined(__SSE2__) || defined(_M_X64)
#include <emmintrin.h>
#define TTK_MERGE_SSE2
#endif

#include "ttk/draw/Typeface.h"
#include "ttk/util/Cache.h"

namespace ttk {
    namespace {

        // Candidate faces per weight, in order of preference. The first that loads wins.
        // Blend2D reads a file and offers no enumeration, so these are guesses at where the
        // Debian, Ubuntu, Arch and Fedora layouts keep them.
        struct Face {
            int weight;
            const char *path;
        };

        const Face FACES[] = {
#ifdef _WIN32
            {Typeface::regular, "C:/Windows/Fonts/segoeui.ttf"},
            {Typeface::semibold, "C:/Windows/Fonts/seguisb.ttf"},
            {Typeface::bold, "C:/Windows/Fonts/segoeuib.ttf"},
            {Typeface::mono, "C:/Windows/Fonts/consola.ttf"},
            {Typeface::monoBold, "C:/Windows/Fonts/consolab.ttf"},
#elifdef __APPLE__
            {Typeface::regular, "/System/Library/Fonts/SFNS.ttf"},
            {Typeface::regular, "/System/Library/Fonts/Helvetica.ttc"},
            {Typeface::bold, "/System/Library/Fonts/SFNS.ttf"},
            {Typeface::mono, "/System/Library/Fonts/Menlo.ttc"},
            {Typeface::mono, "/System/Library/Fonts/Monaco.ttf"},
#else
            {.weight = Typeface::regular, .path = "/usr/share/fonts/noto/NotoSans-Regular.ttf"},
            {.weight = Typeface::regular, .path = "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf"},
            {.weight = Typeface::regular, .path = "/usr/share/fonts/cantarell/Cantarell-Regular.otf"},
            {.weight = Typeface::regular, .path = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"},
            {.weight = Typeface::regular, .path = "/usr/share/fonts/TTF/DejaVuSans.ttf"},
            {.weight = Typeface::regular, .path = "/usr/share/fonts/dejavu/DejaVuSans.ttf"},
            {.weight = Typeface::regular, .path = "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"},
            {.weight = Typeface::regular, .path = "/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf"},

            {.weight = Typeface::semibold, .path = "/usr/share/fonts/noto/NotoSans-SemiBold.ttf"},
            {.weight = Typeface::semibold, .path = "/usr/share/fonts/truetype/noto/NotoSans-SemiBold.ttf"},
            {.weight = Typeface::semibold, .path = "/usr/share/fonts/noto/NotoSans-Bold.ttf"},
            {.weight = Typeface::semibold, .path = "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf"},
            {.weight = Typeface::semibold, .path = "/usr/share/fonts/cantarell/Cantarell-Bold.otf"},
            {.weight = Typeface::semibold, .path = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"},
            {.weight = Typeface::semibold, .path = "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf"},
            {.weight = Typeface::semibold, .path = "/usr/share/fonts/dejavu/DejaVuSans-Bold.ttf"},
            {.weight = Typeface::semibold, .path = "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf"},
            {.weight = Typeface::semibold, .path = "/usr/share/fonts/liberation-sans/LiberationSans-Bold.ttf"},

            {.weight = Typeface::bold, .path = "/usr/share/fonts/noto/NotoSans-Bold.ttf"},
            {.weight = Typeface::bold, .path = "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf"},
            {.weight = Typeface::bold, .path = "/usr/share/fonts/cantarell/Cantarell-Bold.otf"},
            {.weight = Typeface::bold, .path = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"},
            {.weight = Typeface::bold, .path = "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf"},
            {.weight = Typeface::bold, .path = "/usr/share/fonts/dejavu/DejaVuSans-Bold.ttf"},
            {.weight = Typeface::bold, .path = "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf"},
            {.weight = Typeface::bold, .path = "/usr/share/fonts/liberation-sans/LiberationSans-Bold.ttf"},

            {.weight = Typeface::mono, .path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"},
            {.weight = Typeface::mono, .path = "/usr/share/fonts/TTF/DejaVuSansMono.ttf"},
            {.weight = Typeface::mono, .path = "/usr/share/fonts/dejavu/DejaVuSansMono.ttf"},
            {.weight = Typeface::mono, .path = "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf"},
            {.weight = Typeface::mono, .path = "/usr/share/fonts/liberation-mono/LiberationMono-Regular.ttf"},
            {.weight = Typeface::mono, .path = "/usr/share/fonts/noto/NotoSansMono-Regular.ttf"},
            {.weight = Typeface::mono, .path = "/usr/share/fonts/truetype/noto/NotoSansMono-Regular.ttf"},

            {.weight = Typeface::monoBold, .path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf"},
            {.weight = Typeface::monoBold, .path = "/usr/share/fonts/TTF/DejaVuSansMono-Bold.ttf"},
            {.weight = Typeface::monoBold, .path = "/usr/share/fonts/dejavu/DejaVuSansMono-Bold.ttf"},
            {.weight = Typeface::monoBold, .path = "/usr/share/fonts/truetype/liberation/LiberationMono-Bold.ttf"},
            {.weight = Typeface::monoBold, .path = "/usr/share/fonts/liberation-mono/LiberationMono-Bold.ttf"},
            {.weight = Typeface::monoBold, .path = "/usr/share/fonts/noto/NotoSansMono-Bold.ttf"},
            {.weight = Typeface::monoBold, .path = "/usr/share/fonts/truetype/noto/NotoSansMono-Bold.ttf"},
#endif
        };

        // A key for the font cache: the size to a quarter of a pixel is finer than
        // anything here asks for.
        // The byte length of the UTF-8 character starting at `at`.
        size_t step(const std::string_view run, const size_t at) {
            const auto lead = static_cast<unsigned char>(run[at]);
            const size_t wide = lead < 0x80 ? 1 : lead < 0xe0 ? 2 : lead < 0xf0 ? 3 : 4;

            return std::min(wide, run.size() - at);
        }

        // How many runs, answers and words are kept before the coldest quarter goes.
        constexpr size_t SHAPED = 4096;
        constexpr size_t ELIDED = 4096;
        constexpr size_t WORDS = 4096;

        // How many fonts keep a table of glyphs before the one longest unasked for goes.
        constexpr size_t TABLES = 8;

        // How many composed lines are kept. A screenful and the frames they scroll
        // through, not the whole log.
        constexpr size_t ROWS = 256;

        // How many positions across a pixel a glyph is kept at.
        constexpr std::uint32_t SHIFTS = 4;

        // Room left either side of a composed line for ink that reaches past its pens.
        constexpr int SLACK = 2;

        // A glyph's rows are merged into a line this many bytes at a time, as one
        // vector operation each.
        constexpr int CHUNK = 16;

        // The fuller of two coverages, over one chunk: what filling both outlines at
        // once would give where they share a pixel. Spelled out for the vector units,
        // since a build for size leaves the loop scalar and a screen is a hundred
        // thousand of these.
        void merge(std::uint8_t *into, const std::uint8_t *from) {
#if defined(TTK_MERGE_SSE2)
            _mm_storeu_si128(reinterpret_cast<__m128i *>(into),
                             _mm_max_epu8(_mm_loadu_si128(reinterpret_cast<const __m128i *>(into)),
                                          _mm_loadu_si128(reinterpret_cast<const __m128i *>(from))));
#else
            for (int at = 0; at < CHUNK; ++at) {
                into[at] = std::max(into[at], from[at]);
            }
#endif
        }

        constexpr std::string_view ELLIPSIS = "\xe2\x80\xa6";

        // A glyph's ink, in pixels relative to its origin, whichever way the face is
        // scaled and skewed. Blend2D answers the bounds with y already pointing down,
        // and the matrix turns it down again, so y goes in the other way.
        BLBox ink_of(const BLFont &font, const BLBoxI &design) {
            const BLFontMatrix &m = font.matrix();
            BLBox out{std::numeric_limits<double>::max(), std::numeric_limits<double>::max(),
                      std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest()};

            for (const double x : {static_cast<double>(design.x0), static_cast<double>(design.x1)}) {
                for (const double y : {static_cast<double>(-design.y0), static_cast<double>(-design.y1)}) {
                    const double px = (m.m00 * x) + (m.m10 * y);
                    const double py = (m.m01 * x) + (m.m11 * y);

                    out.x0 = std::min(out.x0, px);
                    out.y0 = std::min(out.y0, py);
                    out.x1 = std::max(out.x1, px);
                    out.y1 = std::max(out.y1, py);
                }
            }

            return out;
        }

        long long key(const int weight, const float size) {
            return (static_cast<long long>(weight) * 100000)
                   + static_cast<long long>(std::lround(size * 4.0F));
        }

    }

    bool Typeface::load() {
        for (const Face &face : FACES) {
            // The first candidate for a weight that loads is the one kept.
            if (_faces.contains(face.weight)) {
                continue;
            }

            BLFontFace loaded;

            if (loaded.create_from_file(face.path) != BL_SUCCESS) {
                continue;
            }

            _faces[face.weight] = loaded;
        }

        if (_faces.empty()) {
            return false;
        }

        // A weight with nothing of its own falls back to one that did load, so a
        // machine carrying a single face still draws every label.
        for (const int weight : {regular, semibold, bold, mono, monoBold}) {
            if (!_faces.contains(weight)) {
                _faces[weight] = _faces.begin()->second;
            }
        }

        return true;
    }

    int Typeface::pick(const int weight, const bool fixed) {
        if (!fixed) {
            return weight;
        }

        return weight >= semibold ? monoBold : mono;
    }

    const BLFont &Typeface::at(const int weight, const float size) {
        const long long at = key(weight, size);
        const auto found = _fonts.find(at);

        if (found != _fonts.end()) {
            return found->second;
        }

        BLFont font;

        font.create_from_face(_faces[weight], size);

        return _fonts.emplace(at, std::move(font)).first->second;
    }

    Typeface::Shaped &Typeface::shaped(const BLFont &font, const std::string_view run) {
        if (_maskBytes >= (32U << 20U)) {
            _shaped.clear();
            _maskBytes = 0;
        } else {
            Cache::evict_oldest(_shaped, SHAPED, [this](const Shaped &gone) {
                _maskBytes -= std::min(_maskBytes, static_cast<size_t>(gone.mask.width())
                                                       * static_cast<size_t>(gone.mask.height()));
            });
        }

        auto [made, fresh] = Cache::run_entry(
            _shaped, Cache::RunView{.font = reinterpret_cast<std::uintptr_t>(&font), .run = run});

        made.used = ++_asked;

        if (fresh) {
            BLTextMetrics metrics{};

            made.buffer.set_utf8_text(run.data(), run.size());
            font.shape(made.buffer);

            if (font.get_text_metrics(made.buffer, metrics) == BL_SUCCESS) {
                made.width = static_cast<float>(metrics.advance.x);
                made.ink = metrics.bounding_box;
            }
        }

        return made;
    }

    namespace {

        // Blend2D fills a mask only where it lands square on the pixels.
        bool maskable(const BLContext &context, const BLPoint origin) {
            const BLMatrix2D &at = context.final_transform();

            if (at.type() > BL_TRANSFORM_TYPE_TRANSLATE) {
                return false;
            }

            const double x = static_cast<double>(std::lround(origin.x)) + at.m20;
            const double y = static_cast<double>(std::lround(origin.y)) + at.m21;

            return x == std::floor(x) && y == std::floor(y);
        }

    }

    void Typeface::lay(BLContext &context, const BLFont &font, Shaped &made, const BLPoint origin,
                       const BLRgba32 tone) {
        if (!maskable(context, origin)) {
            context.fill_glyph_run(origin, font, made.buffer.glyph_run(), tone);

            return;
        }

        if (!made.masked) {
            made.masked = true;

            // The metrics box is only ever sideways. Up and down come from the face,
            // whichever of its ascent and its tallest glyph reaches further. A pixel of
            // room each way for the antialiasing to fall into.
            const BLFontMetrics face = font.metrics();
            const double rise = std::max({static_cast<double>(face.ascent),
                                          std::abs(static_cast<double>(face.y_min)),
                                          std::abs(static_cast<double>(face.y_max))});
            const double fall = std::max({static_cast<double>(face.descent),
                                          std::abs(static_cast<double>(face.y_min)),
                                          std::abs(static_cast<double>(face.y_max))});

            const int left = static_cast<int>(std::floor(made.ink.x0)) - 1;
            const int top = -static_cast<int>(std::ceil(rise)) - 1;
            const int wide = static_cast<int>(std::ceil(made.ink.x1)) - left + 1;
            const int tall = static_cast<int>(std::ceil(fall)) - top + 1;

            if (wide > 0 && tall > 0 && made.mask.create(wide, tall, BL_FORMAT_A8) == BL_SUCCESS) {
                BLContext into(made.mask);

                into.clear_all();
                into.fill_glyph_run(BLPoint{static_cast<double>(-left), static_cast<double>(-top)},
                                    font, made.buffer.glyph_run(), BLRgba32(0xffffffff));
                into.end();

                made.maskAt = BLPointI{left, top};
                _maskBytes += static_cast<size_t>(wide) * static_cast<size_t>(tall);
            }
        }

        if (made.mask.is_empty()) {
            return;
        }

        // Whole pixels: the mask was rendered against an origin on one, and a run a
        // fraction over lands blurred rather than a fraction over.
        context.fill_mask(BLPointI{static_cast<int>(std::lround(origin.x)) + made.maskAt.x,
                                   static_cast<int>(std::lround(origin.y)) + made.maskAt.y},
                          made.mask, tone);
    }

    float Typeface::width(const BLFont &font, const std::string_view run) {
        return run.empty() ? 0.0F : shaped(font, run).width;
    }

    float Typeface::width_once(const BLFont &font, const std::string_view run) {
        if (run.empty()) {
            return 0.0F;
        }

        BLTextMetrics metrics{};

        _scratch.set_utf8_text(run.data(), run.size());
        font.shape(_scratch);

        return font.get_text_metrics(_scratch, metrics) == BL_SUCCESS
            ? static_cast<float>(metrics.advance.x)
            : 0.0F;
    }

    float Typeface::width_word(const BLFont &font, const std::string_view word) {
        if (word.empty()) {
            return 0.0F;
        }

        Cache::evict_oldest(_words, WORDS);

        auto [held, fresh] = Cache::run_entry(
            _words, Cache::RunView{.font = reinterpret_cast<std::uintptr_t>(&font), .run = word});

        held.used = ++_asked;

        if (fresh) {
            held.width = width_once(font, word);
        }

        return held.width;
    }

    std::string Typeface::elide(const BLFont &font, const std::string_view run, const float room,
                                const float tracking) {
        // The answer is kept, not the candidates: a label elides to the same string every
        // paint, and putting the candidates in the shape cache would evict what is drawn.
        Cache::evict_oldest(_elided, ELIDED);

        const auto wide = static_cast<std::int64_t>(std::lround(room * 4.0));
        const auto space = static_cast<std::uint32_t>(std::lround(tracking * 16.0));

        auto [held, fresh] = Cache::run_entry(
            _elided, Cache::RunView{.font = reinterpret_cast<std::uintptr_t>(&font),
                                    .at = (wide << 32) | space,
                                    .run = run});

        held.used = ++_asked;

        if (fresh) {
            held.text = elide_once(font, run, room, tracking);
        }

        return held.text;
    }

    std::string Typeface::elide_once(const BLFont &font, const std::string_view run, const float room,
                                    const float tracking) {
        // Candidates are measured but never drawn, so they are not kept: eliding a path
        // would otherwise fill the shape cache with runs nothing asks for again.
        const auto measure = [&](const std::string_view text) {
            return tracking > 0.0F ? width_tracked(font, text, tracking) : width_once(font, text);
        };

        if ((tracking > 0.0F ? width_tracked(font, run, tracking) : width(font, run)) <= room) {
            return std::string(run);
        }

        // Where a character may be cut. A prefix only grows wider as it lengthens, so
        // the longest one that fits is found by bisection rather than one cut at a time.
        _cuts.clear();
        _cuts.push_back(0);

        for (size_t at = 0; at < run.size(); at += step(run, at)) {
            _cuts.push_back(at + step(run, at));
        }

        std::string candidate;
        size_t low = 0;
        size_t high = _cuts.size();
        size_t best = _cuts.size();

        while (low < high) {
            const size_t mid = low + ((high - low) / 2);

            candidate.assign(run.substr(0, _cuts[mid]));
            candidate += "\xe2\x80\xa6";

            if (measure(candidate) <= room) {
                best = mid;
                low = mid + 1;
            } else {
                high = mid;
            }
        }

        if (best == _cuts.size()) {
            return {};
        }

        candidate.assign(run.substr(0, _cuts[best]));
        candidate += "\xe2\x80\xa6";

        return candidate;
    }

    float Typeface::width_tracked(const BLFont &font, const std::string_view run, const float tracking) {
        float total = 0.0F;

        for (size_t at = 0; at < run.size();) {
            const size_t wide = step(run, at);

            total += width(font, run.substr(at, wide)) + tracking;
            at += wide;
        }

        // The tracking after the last character is not part of the run.
        return run.empty() ? 0.0F : total - tracking;
    }

    void Typeface::draw_tracked(BLContext &context, const BLFont &font, const BLPoint top,
                           const std::string_view run, const BLRgba32 tone, const float tracking) {
        const BLFontMetrics metrics = font.metrics();
        const double baseline = std::floor(top.y + metrics.ascent + 0.5);
        double x = top.x;

        for (size_t at = 0; at < run.size();) {
            const size_t wide = step(run, at);
            Shaped &one = shaped(font, run.substr(at, wide));

            lay(context, font, one, BLPoint{x, baseline}, tone);

            x += one.width + tracking;
            at += wide;
        }
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static): see the header.
    float Typeface::line_height(const BLFont &font) const {
        const BLFontMetrics metrics = font.metrics();

        return metrics.ascent + metrics.descent;
    }

    void Typeface::draw(BLContext &context, const BLFont &font, const BLPoint top,
                    const std::string_view run, const BLRgba32 tone) {
        if (run.empty()) {
            return;
        }

        const BLFontMetrics metrics = font.metrics();

        lay(context, font, shaped(font, run), BLPoint{top.x, std::floor(top.y + metrics.ascent + 0.5)},
            tone);
    }

    void Typeface::draw_centred(BLContext &context, const BLFont &font, const BLPoint top,
                           const float height, const std::string_view run, const BLRgba32 tone) {
        if (run.empty()) {
            return;
        }

        const BLFontMetrics metrics = font.metrics();
        const double baseline =
            top.y + ((static_cast<double>(height) + metrics.ascent - metrics.descent) * 0.5);

        lay(context, font, shaped(font, run), BLPoint{top.x, std::floor(baseline + 0.5)}, tone);
    }

    Typeface::Glyphs &Typeface::glyphs(const BLFont &font) {
        Cache::evict_oldest(_glyphs, TABLES);

        Glyphs &table = _glyphs[reinterpret_cast<std::uintptr_t>(&font)];

        table.used = ++_asked;

        return table;
    }

    void Typeface::make(const BLFont &font, const std::uint32_t id, const std::uint32_t shift, Glyph &into) {
        into.made = true;

        BLBoxI design{};

        if (font.get_glyph_bounds(&id, sizeof(id), &design, 1) != BL_SUCCESS
            || design.x1 <= design.x0 || design.y1 <= design.y0) {
            return;
        }

        const BLBox ink = ink_of(font, design);
        const double slide = static_cast<double>(shift) / SHIFTS;

        // A pixel of room each way for the antialiasing to fall into.
        const int left = static_cast<int>(std::floor(ink.x0 + slide)) - 1;
        const int top = static_cast<int>(std::floor(ink.y0)) - 1;
        const int wide = static_cast<int>(std::ceil(ink.x1 + slide)) - left + 2;
        const int tall = static_cast<int>(std::ceil(ink.y1)) - top + 2;

        BLImage mask;

        if (mask.create(wide, tall, BL_FORMAT_A8) != BL_SUCCESS) {
            return;
        }

        std::uint32_t one = id;
        BLGlyphPlacement still{};
        BLGlyphRun run{};

        run.glyph_data = &one;
        run.placement_data = &still;
        run.size = 1;
        run.placement_type = BL_GLYPH_PLACEMENT_TYPE_ADVANCE_OFFSET;
        run.glyph_advance = sizeof(one);
        run.placement_advance = sizeof(still);

        BLContext raster(mask);

        raster.clear_all();
        raster.fill_glyph_run(BLPoint{slide - left, static_cast<double>(-top)}, font, run,
                              BLRgba32(0xffffffff));
        raster.end();

        BLImageData data{};

        if (mask.get_data(&data) != BL_SUCCESS) {
            return;
        }

        into.stride = ((wide + CHUNK - 1) / CHUNK) * CHUNK;
        into.pixels.assign(static_cast<size_t>(into.stride) * static_cast<size_t>(tall), 0);

        for (int y = 0; y < tall; ++y) {
            std::memcpy(into.pixels.data() + (static_cast<size_t>(y) * static_cast<size_t>(into.stride)),
                        static_cast<const std::uint8_t *>(data.pixel_data)
                            + (static_cast<std::ptrdiff_t>(y) * data.stride),
                        static_cast<size_t>(wide));
        }

        into.wide = wide;
        into.tall = tall;
        into.at = BLPointI{left, top};
    }

    Typeface::Cut Typeface::shape_to(const BLFont &font, const std::string_view run, const double room) {
        const double m00 = font.matrix().m00;

        _scratch.set_utf8_text(run.data(), run.size());
        font.shape(_scratch);

        const size_t count = _scratch.size();
        const BLGlyphPlacement *placed = _scratch.placement_data();

        _pens.resize(count + 1);

        double pen = 0.0;

        for (size_t at = 0; at < count; ++at) {
            _pens[at] = pen;
            pen += placed[at].advance.x * m00;
        }

        _pens[count] = pen;

        Cut cut{.shown = count};

        if (pen <= room) {
            return cut;
        }

        // Cut where it would pass the room, with the ellipsis in the last of it.
        const Shaped &dots = shaped(font, ELLIPSIS);

        cut.dotWide = dots.width;
        cut.dot = dots.buffer.size() > 0 ? dots.buffer.content()[0] : 0;

        for (cut.shown = 0; cut.shown < count && _pens[cut.shown + 1] + cut.dotWide <= room; ++cut.shown) {
        }

        cut.made = cut.shown < count;

        return cut;
    }

    void Typeface::draw_once(BLContext &context, const BLFont &font, const BLPoint top,
                             const std::string_view run, const BLRgba32 tone, const double room) {
        if (run.empty() || room <= 0.0) {
            return;
        }

        const BLPoint origin{top.x, std::floor(top.y + font.metrics().ascent + 0.5)};

        if (!maskable(context, origin)) {
            const Cut cut = shape_to(font, run, room);
            BLGlyphRun part = _scratch.glyph_run();

            part.size = cut.shown;
            context.fill_glyph_run(origin, font, part, tone);

            if (cut.made) {
                context.fill_glyph_run(BLPoint{origin.x + _pens[cut.shown], origin.y}, font,
                                       shaped(font, ELLIPSIS).buffer.glyph_run(), tone);
            }

            return;
        }

        Cache::evict_oldest(_rows, ROWS);

        auto [made, fresh] = Cache::run_entry(
            _rows, Cache::RunView{.font = reinterpret_cast<std::uintptr_t>(&font),
                                  .at = std::lround(room * 4.0),
                                  .run = run});

        made.used = ++_asked;

        if (fresh) {
            compose(font, shape_to(font, run, room), room, made);
        }

        if (!made.mask.is_empty()) {
            context.fill_mask(BLPointI{static_cast<int>(std::lround(origin.x)) + made.at.x,
                                       static_cast<int>(std::lround(origin.y)) + made.at.y},
                              made.mask, BLRectI{0, 0, made.wide, made.tall}, tone);
        }
    }

    void Typeface::compose(const BLFont &font, const Cut &cut, const double room, Row &into) {
        const BLFontMetrics face = font.metrics();
        const BLFontMatrix &scale = font.matrix();
        const std::uint32_t *ids = _scratch.content();
        const BLGlyphPlacement *placed = _scratch.placement_data();

        // The line box as lay() sizes one: the face's rise and fall, whichever of its
        // ascent and its tallest glyph reaches further, and a pixel each way.
        const double rise = std::max({static_cast<double>(face.ascent),
                                      std::abs(static_cast<double>(face.y_min)),
                                      std::abs(static_cast<double>(face.y_max))});
        const double fall = std::max({static_cast<double>(face.descent),
                                      std::abs(static_cast<double>(face.y_min)),
                                      std::abs(static_cast<double>(face.y_max))});
        const int above = static_cast<int>(std::ceil(rise)) + 1;
        const int tall = above + static_cast<int>(std::ceil(fall)) + 1;
        const double reach = std::min(room, cut.made ? _pens[cut.shown] + cut.dotWide : _pens.back());
        const int wide = static_cast<int>(std::ceil(reach)) + (SLACK * 2);

        // Wider than shown by a chunk, so a glyph at the right edge is merged whole.
        const int span = wide + CHUNK;

        if (wide <= 0 || tall <= 0 || into.mask.create(span, tall, BL_FORMAT_A8) != BL_SUCCESS) {
            return;
        }

        BLImageData data{};

        if (into.mask.make_mutable(&data) != BL_SUCCESS) {
            into.mask.reset();

            return;
        }

        auto *pixels = static_cast<std::uint8_t *>(data.pixel_data);
        const std::ptrdiff_t stride = data.stride;

        for (int y = 0; y < tall; ++y) {
            std::memset(pixels + (y * stride), 0, static_cast<size_t>(span));
        }

        std::vector<Glyph> &held = glyphs(font).held;

        const auto stamp = [&](const std::uint32_t id, const double pen, const int lift) {
            const double whole = std::floor(pen);
            auto shift = static_cast<std::uint32_t>(((pen - whole) * SHIFTS) + 0.5);
            int column = static_cast<int>(whole) + SLACK;

            if (shift == SHIFTS) {
                shift = 0;
                column += 1;
            }

            const size_t slot = (static_cast<size_t>(id) * SHIFTS) + shift;

            if (slot >= held.size()) {
                held.resize(slot + 1);
            }

            Glyph &one = held[slot];

            if (!one.made) {
                make(font, id, shift, one);
            }

            if (one.pixels.empty()) {
                return;
            }

            const int dx = column + one.at.x;
            const int dy = above + one.at.y + lift;
            const int from = std::max(0, -dy);
            const int to = std::min(one.tall, tall - dy);

            if (dx >= 0 && dx + one.stride <= span) {
                for (int y = from; y < to; ++y) {
                    const std::uint8_t *src = one.pixels.data()
                        + (static_cast<size_t>(y) * static_cast<size_t>(one.stride));
                    std::uint8_t *dst = pixels + ((dy + y) * stride) + dx;

                    for (int chunk = 0; chunk < one.stride; chunk += CHUNK) {
                        merge(dst + chunk, src + chunk);
                    }
                }

                return;
            }

            // Over an edge of the line: only what lands inside it.
            const int first = std::max(0, -dx);
            const int last = std::min(one.wide, span - dx);

            for (int y = from; y < to; ++y) {
                const std::uint8_t *src = one.pixels.data()
                    + (static_cast<size_t>(y) * static_cast<size_t>(one.stride));
                std::uint8_t *dst = pixels + ((dy + y) * stride) + dx;

                for (int x = first; x < last; ++x) {
                    dst[x] = std::max(dst[x], src[x]);
                }
            }
        };

        for (size_t at = 0; at < cut.shown; ++at) {
            stamp(ids[at], _pens[at] + (placed[at].placement.x * scale.m00),
                  static_cast<int>(std::lround(placed[at].placement.y * scale.m11)));
        }

        if (cut.made && cut.dot != 0) {
            stamp(cut.dot, _pens[cut.shown], 0);
        }

        into.at = BLPointI{-SLACK, -above};
        into.wide = wide;
        into.tall = tall;
    }

    size_t Typeface::nearest(const BLFont &font, const std::string_view run, const float x) {
        if (run.empty()) {
            return 0;
        }

        _scratch.set_utf8_text(run.data(), run.size());
        font.shape(_scratch);

        const size_t count = _scratch.size();
        const BLGlyphPlacement *placed = _scratch.placement_data();
        const BLGlyphInfo *info = _scratch.info_data();
        const double m00 = font.matrix().m00;

        size_t best = 0;
        double closest = std::numeric_limits<double>::max();
        double pen = 0.0;

        for (size_t at = 0; at < count; ++at) {
            if (const double gap = std::abs(pen - x); gap < closest) {
                closest = gap;
                best = info[at].cluster;
            }

            pen += placed[at].advance.x * m00;
        }

        return std::abs(pen - x) < closest ? run.size() : std::min(best, run.size());
    }
}
