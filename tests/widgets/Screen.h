// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_TESTS_WIDGETS_SCREEN_H
#define TTK_TESTS_WIDGETS_SCREEN_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <utility>

#include <gtest/gtest.h>

#include "support/Canvas.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Root.h"

namespace widgets {
    class Painted : public ::testing::Test {
    protected:
        void SetUp() override {
            if (!tests::fonts_loaded()) {
                GTEST_SKIP() << "no font installed";
            }
        }

        void TearDown() override { ttk::Theme::set_mode(ttk::Theme::Mode::System); }
    };

    const inline ttk::Theme::Palette &palette() {
        return ttk::Theme::palette();
    }

    inline std::uint32_t background() {
        return palette().background.value;
    }

    // Mounted on the popup layer, which keeps the box a child was placed at.
    template <typename Kind>
    Kind *mount(tests::Canvas &canvas, std::unique_ptr<Kind> made, const BLRect &box) {
        Kind *raw = canvas.root().layer(ttk::Root::POPUPS)->append(std::move(made));

        raw->place(box, tests::fonts());
        canvas.frame();

        return raw;
    }

    inline std::size_t area(const BLRect &box) {
        const double left = std::floor(box.x);
        const double top = std::floor(box.y);

        return static_cast<std::size_t>(std::ceil(box.x + box.w) - left)
               * static_cast<std::size_t>(std::ceil(box.y + box.h) - top);
    }

    inline int channel(const std::uint32_t colour, const unsigned shift) {
        return static_cast<int>((colour >> shift) & 0xffU);
    }

    // The largest difference across red, green and blue.
    inline int distance(const std::uint32_t one, const std::uint32_t two) {
        int widest = 0;

        for (const unsigned shift : {0U, 8U, 16U}) {
            widest = std::max(widest, std::abs(channel(one, shift) - channel(two, shift)));
        }

        return widest;
    }

    inline bool near(const std::uint32_t one, const std::uint32_t two, const int slack = 2) {
        return distance(one, two) <= slack;
    }

    // `over` laid on the opaque `under` with its own alpha.
    inline std::uint32_t over(const std::uint32_t under, const std::uint32_t over) {
        const double alpha = channel(over, 24) / 255.0;
        std::uint32_t out = 0xff000000U;

        for (const unsigned shift : {0U, 8U, 16U}) {
            const double mixed = (channel(under, shift) * (1.0 - alpha)) + (channel(over, shift) * alpha);

            out |= static_cast<std::uint32_t>(std::lround(mixed)) << shift;
        }

        return out;
    }

    inline std::uint32_t over(const BLRgba32 under, const BLRgba32 above) {
        return over(under.value, above.value);
    }

    struct Ink {
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;
        std::size_t pixels = 0;
    };

    // The bounds of every pixel in `region` that differs from `ground`.
    inline Ink ink(const tests::Canvas &canvas, const BLRectI &region, const std::uint32_t ground) {
        Ink found{.left = region.x + region.w, .top = region.y + region.h, .right = region.x, .bottom = region.y};

        for (int y = region.y; y < region.y + region.h; ++y) {
            for (int x = region.x; x < region.x + region.w; ++x) {
                if (canvas.at(x, y) != ground) {
                    found.left = std::min(found.left, x);
                    found.top = std::min(found.top, y);
                    found.right = std::max(found.right, x + 1);
                    found.bottom = std::max(found.bottom, y + 1);
                    ++found.pixels;
                }
            }
        }

        return found;
    }

    // Pixels in `region` within `slack` of `colour` on every channel.
    inline std::size_t count(const tests::Canvas &canvas, const BLRectI &region, const std::uint32_t colour,
                             const int slack = 0) {
        std::size_t total = 0;

        for (int y = region.y; y < region.y + region.h; ++y) {
            for (int x = region.x; x < region.x + region.w; ++x) {
                total += distance(canvas.at(x, y), colour) <= slack ? 1U : 0U;
            }
        }

        return total;
    }

    inline std::size_t count(const tests::Canvas &canvas, const BLRectI &region, const BLRgba32 colour,
                             const int slack = 0) {
        return count(canvas, region, colour.value, slack);
    }

    // Pixels anywhere on the canvas outside `box` that differ from the background.
    inline std::size_t outside(const tests::Canvas &canvas, const BLRectI &box) {
        std::size_t total = 0;

        for (int y = 0; y < canvas.height(); ++y) {
            for (int x = 0; x < canvas.width(); ++x) {
                const bool in = x >= box.x && x < box.x + box.w && y >= box.y && y < box.y + box.h;

                total += !in && canvas.at(x, y) != background() ? 1U : 0U;
            }
        }

        return total;
    }

    // How far the pixels of `region` stand from `ground`, summed.
    inline std::size_t contrast(const tests::Canvas &canvas, const BLRectI &region, const std::uint32_t ground) {
        std::size_t total = 0;

        for (int y = region.y; y < region.y + region.h; ++y) {
            for (int x = region.x; x < region.x + region.w; ++x) {
                total += static_cast<std::size_t>(distance(canvas.at(x, y), ground));
            }
        }

        return total;
    }

    // The pixel of `region` that stands farthest from `ground`, the fullest ink of thin text.
    inline std::uint32_t peak(const tests::Canvas &canvas, const BLRectI &region, const std::uint32_t ground) {
        std::uint32_t found = ground;

        for (int y = region.y; y < region.y + region.h; ++y) {
            for (int x = region.x; x < region.x + region.w; ++x) {
                if (distance(canvas.at(x, y), ground) > distance(found, ground)) {
                    found = canvas.at(x, y);
                }
            }
        }

        return found;
    }

    inline BLRectI pixels_of(const BLRect &box) {
        return BLRectI{static_cast<int>(box.x), static_cast<int>(box.y), static_cast<int>(box.w),
                       static_cast<int>(box.h)};
    }

    inline void hover(tests::Canvas &canvas, const double x, const double y) {
        canvas.root().motion(x, y);
    }

    inline void click(tests::Canvas &canvas, const double x, const double y) {
        canvas.root().motion(x, y);
        canvas.root().press(ttk::Pointer{.x = x, .y = y});
        canvas.root().release(ttk::Pointer{.x = x, .y = y});
    }

    inline ttk::Key key(const int code) {
        return ttk::Key{.code = code};
    }
}

#endif //TTK_TESTS_WIDGETS_SCREEN_H
