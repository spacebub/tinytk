// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_TESTS_DRAW_PIXELS_H
#define TTK_TESTS_DRAW_PIXELS_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include <blend2d/blend2d.h>

namespace tests {
    // One 32-bit pixel of `image`, as 0xAARRGGBB, or 0 outside it.
    inline std::uint32_t pixel(const BLImage &image, const int x, const int y) {
        BLImageData data{};

        if (x < 0 || y < 0 || x >= image.width() || y >= image.height() || image.get_data(&data) != BL_SUCCESS) {
            return 0;
        }

        std::uint32_t value = 0;

        std::memcpy(&value,
                    static_cast<const std::uint8_t *>(data.pixel_data) + (static_cast<std::ptrdiff_t>(y) * data.stride)
                            + (static_cast<std::ptrdiff_t>(x) * 4),
                    4);

        return value;
    }

    // Every pixel of a 32-bit `image`, row by row.
    inline std::vector<std::uint32_t> pixels(const BLImage &image) {
        std::vector<std::uint32_t> all;

        all.reserve(static_cast<std::size_t>(image.width()) * static_cast<std::size_t>(image.height()));

        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                all.push_back(pixel(image, x, y));
            }
        }

        return all;
    }

    // Bounds of the pixels of a 32-bit `image` that differ from `ground`, right and bottom exclusive.
    struct Ink {
        int x0 = 0;
        int y0 = 0;
        int x1 = 0;
        int y1 = 0;
        std::size_t count = 0;

        [[nodiscard]] bool any() const { return count != 0; }
    };

    inline Ink ink(const BLImage &image, const std::uint32_t ground) {
        Ink found{.x0 = image.width(), .y0 = image.height()};

        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (pixel(image, x, y) == ground) {
                    continue;
                }

                found.x0 = std::min(found.x0, x);
                found.y0 = std::min(found.y0, y);
                found.x1 = std::max(found.x1, x + 1);
                found.y1 = std::max(found.y1, y + 1);
                ++found.count;
            }
        }

        return found;
    }

    // Whether every channel of `one` and `two` is within `slack` levels.
    inline bool close(const std::uint32_t one, const std::uint32_t two, const int slack) {
        for (unsigned shift = 0; shift < 32; shift += 8) {
            const int a = static_cast<int>((one >> shift) & 0xffU);
            const int b = static_cast<int>((two >> shift) & 0xffU);

            if (a - b > slack || b - a > slack) {
                return false;
            }
        }

        return true;
    }
}

#endif //TTK_TESTS_DRAW_PIXELS_H
