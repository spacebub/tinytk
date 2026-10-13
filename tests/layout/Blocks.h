// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_TESTS_LAYOUT_BLOCKS_H
#define TTK_TESTS_LAYOUT_BLOCKS_H

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include <blend2d/blend2d.h>
#include <gtest/gtest.h>

#include "support/Canvas.h"
#include "ttk/toolkit/Painter.h"
#include "ttk/toolkit/Widget.h"

namespace tests {
// A solid rectangle with a natural size of its own, so a layout can be checked without text.
class Block final : public ttk::Widget {
public:
    explicit Block(const double width = 0.0, const double height = 0.0, const std::uint32_t fill = 0xff3366aaU)
        : wide(width), tall(height), colour(fill) {}

    // Natural size before fixedWidth, fixedHeight and the floors.
    double wide;
    double tall;

    // When positive, the height is this area spread over the width it is measured at, as wrapped text is.
    double area = 0.0;

    std::uint32_t colour;

    int painted = 0;

    void answers() { _takesPointer = true; }

    double natural_width(ttk::Typeface & /*type*/) override { return fixedWidth >= 0.0 ? fixedWidth : wide; }

    double natural_height(ttk::Typeface & /*type*/, const double width) override {
        if (fixedHeight >= 0.0) {
            return fixedHeight;
        }

        return area > 0.0 ? area / std::max(1.0, width) : tall;
    }

    void paint(const ttk::Painter &painter) override {
        ++painted;

        painter.fill(_box, BLRgba32(colour));

        Widget::paint(painter);
    }
};

inline ::testing::AssertionResult boxed(const BLRect &got, const BLRect &want) {
    if (got.x == want.x && got.y == want.y && got.w == want.w && got.h == want.h) {
        return ::testing::AssertionSuccess();
    }

    return ::testing::AssertionFailure() << "box {" << got.x << ", " << got.y << ", " << got.w << ", " << got.h
                                         << "}, wanted {" << want.x << ", " << want.y << ", " << want.w << ", "
                                         << want.h << "}";
}

// As Canvas::matches_full(), allowing the one level of rounding an antialiased corner cut by a strip's edge
// can differ by.
inline bool nearly_full(Canvas &canvas) {
    const std::vector<std::uint32_t> partial = canvas.pixels();

    canvas.full();

    return std::ranges::equal(partial, canvas.pixels(), [](const std::uint32_t a, const std::uint32_t b) {
        for (unsigned shift = 0; shift < 32; shift += 8) {
            if (std::abs(static_cast<int>((a >> shift) & 0xffU) - static_cast<int>((b >> shift) & 0xffU)) > 1) {
                return false;
            }
        }

        return true;
    });
}
}

#endif //TTK_TESTS_LAYOUT_BLOCKS_H
