// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_TESTS_CONTROLS_MOUNT_H
#define TTK_TESTS_CONTROLS_MOUNT_H

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include <blend2d/blend2d.h>
#include <gtest/gtest.h>

#include "support/Canvas.h"
#include "ttk/toolkit/Event.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/Widget.h"

namespace controls {
// Holds its children at a fixed box on the page, so a control is hit, focused and painted through the root.
class Holder final : public ttk::Widget {
public:
    BLRect spot{};

    void arrange(ttk::Typeface &type) override {
        for (const Ptr &child : children()) {
            child->place(spot, type);
        }
    }
};

template <typename Kind>
Kind *mount(tests::Canvas &canvas, std::unique_ptr<Kind> widget, const BLRect &spot) {
    auto holder = std::make_unique<Holder>();

    holder->spot = spot;

    Kind *raw = holder->append(std::move(widget));

    canvas.root().content()->add(std::move(holder));
    canvas.root().relayout();
    canvas.frame();

    return raw;
}

inline void click(ttk::Root &root, const double x, const double y, const bool shift = false) {
    root.motion(x, y);
    root.press(ttk::Pointer{.x = x, .y = y, .shift = shift});
    root.release(ttk::Pointer{.x = x, .y = y});
}

inline void sweep(ttk::Root &root, const double fromX, const double fromY, const double toX, const double toY) {
    root.motion(fromX, fromY);
    root.press(ttk::Pointer{.x = fromX, .y = fromY});
    root.motion(toX, toY);
    root.release(ttk::Pointer{.x = toX, .y = toY});
}

inline ttk::Key key(const int code, const bool ctrl = false, const bool shift = false) {
    return ttk::Key{.code = code, .ctrl = ctrl, .shift = shift};
}

inline int channel(const std::uint32_t pixel, const unsigned shift) {
    return static_cast<int>((pixel >> shift) & 0xffU);
}

inline int distance(const std::uint32_t one, const std::uint32_t two) {
    int total = 0;

    for (unsigned shift = 0; shift < 24; shift += 8) {
        total += std::abs(channel(one, shift) - channel(two, shift));
    }

    return total;
}

// Whether `pixel` is closer to `ink` than to `ground`, for edges that antialiasing blends.
inline bool leans(const std::uint32_t pixel, const BLRgba32 ink, const BLRgba32 ground) {
    return distance(pixel, ink.value) < distance(pixel, ground.value);
}

inline int count_near(const tests::Canvas &canvas, const BLRectI &area, const BLRgba32 ink, const int slack = 24) {
    int found = 0;

    for (int y = area.y; y < area.y + area.h; ++y) {
        for (int x = area.x; x < area.x + area.w; ++x) {
            if (distance(canvas.at(x, y), ink.value) <= slack) {
                ++found;
            }
        }
    }

    return found;
}

inline std::vector<std::uint32_t> pixels_in(const tests::Canvas &canvas, const BLRectI &area) {
    std::vector<std::uint32_t> out;

    for (int y = area.y; y < area.y + area.h; ++y) {
        for (int x = area.x; x < area.x + area.w; ++x) {
            out.push_back(canvas.at(x, y));
        }
    }

    return out;
}

class Painted : public ::testing::Test {
protected:
    void SetUp() override {
        if (!tests::fonts_loaded()) {
            GTEST_SKIP() << "no font installed";
        }
    }
};

struct Calls {
    std::vector<std::string> texts;

    auto recorder() {
        return [this](const std::string &text) { texts.push_back(text); };
    }
};
}

#endif //TTK_TESTS_CONTROLS_MOUNT_H
