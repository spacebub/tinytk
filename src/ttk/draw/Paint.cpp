// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <tuple>
#include <vector>

#ifdef __AVX2__
#include <immintrin.h>
#define TTK_BLUR_AVX2
#define TTK_BLUR_SIMD
#elif defined(__SSE2__) || defined(_M_X64)
#include <emmintrin.h>
#define TTK_BLUR_SIMD
#endif

#include "ttk/draw/Paint.h"

namespace ttk {
    namespace {

        // How many blurred sprites are kept before the lot is thrown away.
        constexpr size_t KEPT = 24;

        // A CSS blur radius is two standard deviations.
        double sigma_of(const double blur) {
            return blur * 0.5;
        }

        // Three box passes approximate a Gaussian closely enough that nothing here can
        // tell the difference. This is the usual radius for one of them, capped where a
        // window's sum would no longer fit sixteen bits.
        int box_of(const double blur) {
            const int radius = static_cast<int>(std::lround(sigma_of(blur) * 1.88));

            return std::clamp(radius, 1, 127);
        }

#ifdef TTK_BLUR_SIMD
        // The vector the passes are written against: eight words of a pixel each
        // under SSE2, sixteen under AVX2. The blur only ever needs these operations,
        // and the AVX2 pack and unpack work within each half, which `order` undoes.
        struct Lane {
#ifdef TTK_BLUR_AVX2
            using V = __m256i;

            static constexpr int WORDS = 16;

            static V zero() { return _mm256_setzero_si256(); }
            static V set1(const short value) { return _mm256_set1_epi16(value); }
            static V load(const void *at) { return _mm256_loadu_si256(static_cast<const V *>(at)); }
            static void store(void *at, const V v) { _mm256_storeu_si256(static_cast<V *>(at), v); }
            static V add(const V a, const V b) { return _mm256_add_epi16(a, b); }
            static V sub(const V a, const V b) { return _mm256_sub_epi16(a, b); }
            static V mulhi(const V a, const V b) { return _mm256_mulhi_epu16(a, b); }
            static V mullo(const V a, const V b) { return _mm256_mullo_epi16(a, b); }
            static V subs(const V a, const V b) { return _mm256_subs_epu16(a, b); }
            static V equal(const V a, const V b) { return _mm256_cmpeq_epi16(a, b); }
            static V andnot(const V a, const V b) { return _mm256_andnot_si256(a, b); }
            static V shift7(const V v) { return _mm256_srli_epi16(v, 7); }
            static V lo(const V v) { return _mm256_unpacklo_epi8(v, zero()); }
            static V hi(const V v) { return _mm256_unpackhi_epi8(v, zero()); }
            static V pack(const V a, const V b) { return _mm256_packus_epi16(a, b); }
            static V zip8(const V a, const V b) { return _mm256_unpacklo_epi8(a, b); }
            static V zip16lo(const V a, const V b) { return _mm256_unpacklo_epi16(a, b); }
            static V zip16hi(const V a, const V b) { return _mm256_unpackhi_epi16(a, b); }

            static V widen(const uint8_t *at) {
                return _mm256_cvtepu8_epi16(_mm_loadu_si128(reinterpret_cast<const __m128i *>(at)));
            }

            static void narrow(uint8_t *at, const V words) {
                const V packed = _mm256_permute4x64_epi64(pack(words, zero()), 0x08);

                _mm_storeu_si128(reinterpret_cast<__m128i *>(at), _mm256_castsi256_si128(packed));
            }

            static V scan(V v) {
                v = add(v, _mm256_slli_si256(v, 2));
                v = add(v, _mm256_slli_si256(v, 4));
                v = add(v, _mm256_slli_si256(v, 8));

                const V tops = _mm256_shuffle_epi32(_mm256_shufflehi_epi16(v, 0xff), 0xff);

                return add(v, _mm256_permute2x128_si256(tops, tops, 0x08));
            }

            static V last(const V v) {
                const V tops = _mm256_shuffle_epi32(_mm256_shufflehi_epi16(v, 0xff), 0xff);

                return _mm256_permute2x128_si256(tops, tops, 0x11);
            }

            static void order(V &a, V &b) {
                const V first = _mm256_permute2x128_si256(a, b, 0x20);
                const V second = _mm256_permute2x128_si256(a, b, 0x31);

                a = first;
                b = second;
            }
#else
            using V = __m128i;

            static constexpr int WORDS = 8;

            static V zero() { return _mm_setzero_si128(); }
            static V set1(const short value) { return _mm_set1_epi16(value); }
            static V load(const void *at) { return _mm_loadu_si128(static_cast<const V *>(at)); }
            static void store(void *at, const V v) { _mm_storeu_si128(static_cast<V *>(at), v); }
            static V add(const V a, const V b) { return _mm_add_epi16(a, b); }
            static V sub(const V a, const V b) { return _mm_sub_epi16(a, b); }
            static V mulhi(const V a, const V b) { return _mm_mulhi_epu16(a, b); }
            static V mullo(const V a, const V b) { return _mm_mullo_epi16(a, b); }
            static V subs(const V a, const V b) { return _mm_subs_epu16(a, b); }
            static V equal(const V a, const V b) { return _mm_cmpeq_epi16(a, b); }
            static V andnot(const V a, const V b) { return _mm_andnot_si128(a, b); }
            static V shift7(const V v) { return _mm_srli_epi16(v, 7); }
            static V lo(const V v) { return _mm_unpacklo_epi8(v, zero()); }
            static V hi(const V v) { return _mm_unpackhi_epi8(v, zero()); }
            static V pack(const V a, const V b) { return _mm_packus_epi16(a, b); }
            static V zip8(const V a, const V b) { return _mm_unpacklo_epi8(a, b); }
            static V zip16lo(const V a, const V b) { return _mm_unpacklo_epi16(a, b); }
            static V zip16hi(const V a, const V b) { return _mm_unpackhi_epi16(a, b); }

            static V widen(const uint8_t *at) {
                return lo(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(at)));
            }

            static void narrow(uint8_t *at, const V words) {
                _mm_storel_epi64(reinterpret_cast<__m128i *>(at), pack(words, zero()));
            }

            static V scan(V v) {
                v = add(v, _mm_slli_si128(v, 2));
                v = add(v, _mm_slli_si128(v, 4));
                v = add(v, _mm_slli_si128(v, 8));

                return v;
            }

            static V last(const V v) {
                return _mm_shuffle_epi32(_mm_shufflehi_epi16(v, 0xff), 0xff);
            }

            static void order(V & /*unused*/, V & /*unused*/) {}
#endif
        };

        using V = Lane::V;

        // Rows of coverage are this many bytes across, so every pass reads and writes
        // whole vectors.
        constexpr int LANES = Lane::WORDS * 2;

        // floor(x / span) over the lanes, exact for x up to 255 * span: the magic
        // quotient is at most one too high, and multiplying back finds when.
        struct Divider {
            V magic;
            V span;
            V one;
        };

        Divider divider(const int span) {
            return {.magic = Lane::set1(static_cast<short>((65536 + span - 1) / span)),
                    .span = Lane::set1(static_cast<short>(span)),
                    .one = Lane::set1(1)};
        }

        V divide(const V x, const Divider &d) {
            const V q = Lane::mulhi(x, d.magic);
            const V back = Lane::mullo(q, d.span);
            const V over = Lane::subs(back, x);
            const V fine = Lane::equal(over, Lane::zero());

            return Lane::sub(q, Lane::andnot(fine, d.one));
        }

        // floor(x / 255) over the lanes, exact for x up to 255 * 255.
        V div255(const V x) {
            return Lane::shift7(Lane::mulhi(x, Lane::set1(-32639)));
        }
#else
        constexpr int LANES = 16;
#endif

        // One horizontal box pass, in place. Each output is the difference of two
        // prefix sums, so the cost does not depend on the radius, and the sums wrap at
        // sixteen bits since a window never holds more than 255 * span. Past either
        // edge the window reads zero: the sprite is transparent there.
        void blur_across(uint8_t *pixels, const int stride, const int width, const int height,
                         const int radius, std::vector<uint16_t> &prefix) {
            const int span = (radius * 2) + 1;
            const size_t lead = static_cast<size_t>(radius) + 1;

            prefix.assign(lead + static_cast<size_t>(stride) + static_cast<size_t>(span) + LANES, 0);

            for (int y = 0; y < height; ++y) {
                uint8_t *row = pixels + (static_cast<size_t>(y) * static_cast<size_t>(stride));
                uint16_t *sums = prefix.data() + lead;

#ifdef TTK_BLUR_SIMD
                V carry = Lane::zero();

                for (int x = 0; x < width; x += Lane::WORDS) {
                    const V v = Lane::add(Lane::scan(Lane::widen(row + x)), carry);

                    Lane::store(sums + x, v);

                    carry = Lane::last(v);
                }
#else
                uint16_t running = 0;

                for (int x = 0; x < width; ++x) {
                    running = static_cast<uint16_t>(running + row[x]);
                    sums[x] = running;
                }
#endif

                const uint16_t total = sums[width - 1];

                for (auto at = static_cast<size_t>(width); at < static_cast<size_t>(stride) + span + Lane::WORDS; ++at) {
                    sums[at] = total;
                }

#ifdef TTK_BLUR_SIMD
                const Divider d = divider(span);

                for (int x = 0; x < width; x += Lane::WORDS) {
                    const V sum = Lane::sub(Lane::load(prefix.data() + x + span), Lane::load(prefix.data() + x));

                    Lane::narrow(row + x, divide(sum, d));
                }
#else
                for (int x = 0; x < width; ++x) {
                    const auto sum = static_cast<uint16_t>(prefix[static_cast<size_t>(x) + span] - prefix[x]);

                    row[x] = static_cast<uint8_t>(sum / span);
                }
#endif
            }
        }

        // One vertical box pass, `from` to `to`, with a running sum per column.
        void blur_down(const uint8_t *from, uint8_t *to, const int stride, const int height,
                       const int radius) {
            const int span = (radius * 2) + 1;

            const auto row = [&](const int y) {
                return from + (static_cast<size_t>(y) * static_cast<size_t>(stride));
            };

#ifdef TTK_BLUR_SIMD
            const Divider d = divider(span);

            for (int x = 0; x < stride; x += LANES) {
                V lo = Lane::zero();
                V hi = Lane::zero();

                const auto add = [&](const int y) {
                    const V v = Lane::load(row(y) + x);

                    lo = Lane::add(lo, Lane::lo(v));
                    hi = Lane::add(hi, Lane::hi(v));
                };

                const auto drop = [&](const int y) {
                    const V v = Lane::load(row(y) + x);

                    lo = Lane::sub(lo, Lane::lo(v));
                    hi = Lane::sub(hi, Lane::hi(v));
                };

                for (int y = 0; y <= radius && y < height; ++y) {
                    add(y);
                }

                for (int y = 0; y < height; ++y) {
                    Lane::store(to + (static_cast<size_t>(y) * stride) + x, Lane::pack(divide(lo, d), divide(hi, d)));

                    if (y + radius + 1 < height) {
                        add(y + radius + 1);
                    }

                    if (y - radius >= 0) {
                        drop(y - radius);
                    }
                }
            }
#else
            std::vector<uint32_t> sums(static_cast<size_t>(stride), 0);

            for (int y = 0; y <= radius && y < height; ++y) {
                for (int x = 0; x < stride; ++x) {
                    sums[x] += row(y)[x];
                }
            }

            for (int y = 0; y < height; ++y) {
                uint8_t *line = to + (static_cast<size_t>(y) * static_cast<size_t>(stride));

                for (int x = 0; x < stride; ++x) {
                    line[x] = static_cast<uint8_t>(sums[x] / static_cast<uint32_t>(span));
                }

                if (y + radius + 1 < height) {
                    for (int x = 0; x < stride; ++x) {
                        sums[x] += row(y + radius + 1)[x];
                    }
                }

                if (y - radius >= 0) {
                    for (int x = 0; x < stride; ++x) {
                        sums[x] -= row(y - radius)[x];
                    }
                }
            }
#endif
        }

#ifdef TTK_BLUR_SIMD
        // A vector of coverages to the tint premultiplied by each, as the sprite is.
        void tint_row(uint32_t *line, const uint8_t *cover, const int width, const BLRgba32 tint) {
            const V zero = Lane::zero();
            const V alpha = Lane::set1(static_cast<short>(tint.value >> 24U));
            const V red = Lane::set1(static_cast<short>((tint.value >> 16U) & 0xffU));
            const V green = Lane::set1(static_cast<short>((tint.value >> 8U) & 0xffU));
            const V blue = Lane::set1(static_cast<short>(tint.value & 0xffU));

            for (int x = 0; x < width; x += Lane::WORDS) {
                const V solid = div255(Lane::mullo(Lane::widen(cover + x), alpha));
                const V r = div255(Lane::mullo(solid, red));
                const V g = div255(Lane::mullo(solid, green));
                const V b = div255(Lane::mullo(solid, blue));

                const V bg = Lane::zip8(Lane::pack(b, zero), Lane::pack(g, zero));
                const V ra = Lane::zip8(Lane::pack(r, zero), Lane::pack(solid, zero));

                V first = Lane::zip16lo(bg, ra);
                V second = Lane::zip16hi(bg, ra);

                Lane::order(first, second);
                Lane::store(line + x, first);
                Lane::store(line + x + (Lane::WORDS / 2), second);
            }
        }
#else
        // The tint at every coverage the blur can leave, premultiplied as the sprite is.
        std::array<uint32_t, 256> tone_of(const BLRgba32 tint) {
            const uint32_t alpha = tint.value >> 24U;
            const uint32_t red = (tint.value >> 16U) & 0xffU;
            const uint32_t green = (tint.value >> 8U) & 0xffU;
            const uint32_t blue = tint.value & 0xffU;

            std::array<uint32_t, 256> made{};

            for (uint32_t at = 0; at < 256; ++at) {
                const uint32_t solid = (alpha * at) / 255U;

                made[at] = (solid << 24U) | (((red * solid) / 255U) << 16U)
                    | (((green * solid) / 255U) << 8U) | ((blue * solid) / 255U);
            }

            return made;
        }
#endif

        using Shape = std::tuple<int, int, int, int, uint32_t>;

        std::map<Shape, BLImage> sprites;

        // What a failed rasterisation hands back, so the failure is not cached as a shape.
        const BLImage &nothing() {
            static const BLImage empty;

            return empty;
        }

    }

    double Paint::bleed(const double blur) {
        // Three box passes reach one and a half box widths, near three sigma. Short of
        // that the tail is cut off at the sprite's border and reads as an edge.
        return std::ceil(sigma_of(blur) * 3.0) + 2.0;
    }

    const BLImage &Paint::shadow(const int width, const int height, const double radius,
                                 const double blur, const BLRgba32 tint) {
        const Shape shape{width, height, static_cast<int>(std::lround(radius * 4.0)),
                          static_cast<int>(std::lround(blur * 4.0)), tint.value};

        const auto found = sprites.find(shape);

        if (found != sprites.end()) {
            return found->second;
        }

        // A resize walks the card through a new size every frame, and each is a shape
        // of its own. Without a ceiling the cache would grow for as long as the drag.
        if (sprites.size() > KEPT) {
            sprites.clear();
        }

        const int pad = static_cast<int>(bleed(blur));
        const int across = width + (pad * 2);
        const int down = height + (pad * 2);

        BLImage sprite;
        BLImage cast;

        // One tint throughout, so only its coverage is blurred and the tint goes back
        // on after: a byte a pixel rather than four channels.
        if (sprite.create(across, down, BL_FORMAT_PRGB32) != BL_SUCCESS
            || cast.create(across, down, BL_FORMAT_A8) != BL_SUCCESS) {
            return nothing();
        }

        {
            BLContext context(cast);

            context.clear_all();
            context.fill_round_rect(BLRect{static_cast<double>(pad), static_cast<double>(pad),
                                           static_cast<double>(width), static_cast<double>(height)},
                                    radius, radius, BLRgba32(0xffffffff));
        }

        BLImageData held{};
        BLImageData data{};

        if (cast.make_mutable(&held) != BL_SUCCESS || sprite.make_mutable(&data) != BL_SUCCESS) {
            return nothing();
        }

        // Packed copies with rows padded to a vector. The vertical pass reads rows it
        // has already written, so it goes from one to the other.
        const int stride = ((across + LANES - 1) / LANES) * LANES;
        std::vector<uint8_t> cover(static_cast<size_t>(stride) * static_cast<size_t>(down));
        std::vector<uint8_t> other(cover.size());

        for (int y = 0; y < down; ++y) {
            std::memcpy(cover.data() + (static_cast<size_t>(y) * static_cast<size_t>(stride)),
                        static_cast<const uint8_t *>(held.pixel_data) + (static_cast<ptrdiff_t>(y) * held.stride),
                        static_cast<size_t>(across));
        }

        const int box = box_of(blur);
        std::vector<uint16_t> prefix;
        uint8_t *blurred = cover.data();
        uint8_t *spare = other.data();

        for (int pass = 0; pass < 3; ++pass) {
            blur_across(blurred, stride, across, down, box, prefix);
            blur_down(blurred, spare, stride, down, box);
            std::swap(blurred, spare);
        }

#ifdef TTK_BLUR_SIMD
        // The sprite's rows are exactly as wide as asked, so the last few pixels of
        // one go through a vector of their own.
        const int whole = across & ~(Lane::WORDS - 1);
#else
        const std::array<uint32_t, 256> tone = tone_of(tint);
#endif

        for (int y = 0; y < down; ++y) {
            auto *line = reinterpret_cast<uint32_t *>(static_cast<uint8_t *>(data.pixel_data)
                                                      + (static_cast<ptrdiff_t>(y) * data.stride));
            const uint8_t *from = blurred + (static_cast<size_t>(y) * static_cast<size_t>(stride));

#ifdef TTK_BLUR_SIMD
            tint_row(line, from, whole, tint);

            if (whole < across) {
                alignas(32) uint32_t tail[Lane::WORDS];

                tint_row(tail, from + whole, Lane::WORDS, tint);
                std::memcpy(line + whole, tail, static_cast<size_t>(across - whole) * sizeof(uint32_t));
            }
#else
            for (int x = 0; x < across; ++x) {
                line[x] = tone[from[x]];
            }
#endif
        }

        return sprites.emplace(shape, std::move(sprite)).first->second;
    }

    BLGradient Paint::down(const BLRect &box) {
        return BLGradient(BLLinearGradientValues(box.x, box.y, box.x, box.y + box.h));
    }

    BLImage Paint::load(const std::string &path) {
        BLImage image;

        image.read_from_file(path.c_str());

        return image;
    }

    void Paint::cover(BLContext &context, const BLRect &box, const BLImage &source,
                      const double radius) {
        const BLSizeI size = source.size();

        if (size.w <= 0 || size.h <= 0) {
            return;
        }

        // The larger of the two ratios fills the box. The rest is cropped.
        const double scale = std::max(box.w / size.w, box.h / size.h);
        const double wide = size.w * scale;
        const double tall = size.h * scale;
        const double x = box.x + ((box.w - wide) * 0.5);
        const double y = box.y + ((box.h - tall) * 0.5);
        const double corner = std::min(radius, std::min(box.w, box.h) / 2.0);

        if (corner <= 0.0) {
            context.save();
            context.clip_to_rect(box);
            context.blit_image(BLRect{x, y, wide, tall}, source, BLRectI{0, 0, size.w, size.h});
            context.restore();

            return;
        }

        // Blend2D clips to rectangles only, so the corners come off the shape the image
        // is poured into.
        BLMatrix2D at = BLMatrix2D::make_translation(x, y);

        at.scale(scale, scale);

        context.fill_round_rect(box, corner, corner, BLPattern(source, BL_EXTEND_MODE_PAD, at));
    }
}
