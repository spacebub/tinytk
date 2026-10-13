// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_TESTS_CANVAS_H
#define TTK_TESTS_CANVAS_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include <blend2d/blend2d.h>

#include "ttk/draw/Surface.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"

namespace tests {
// One process wide face set. Loads the first time it is asked for, and the
// cases that need text skip themselves when no font could be found.
ttk::Typeface &fonts();
bool fonts_loaded();

// A window's pixels with no window: an offscreen surface driven through the
// same compose step Shell::draw uses, plus the root it paints.
class Canvas {
public:
    explicit Canvas(int width = 400, int height = 240);
    ~Canvas();

    Canvas(const Canvas &) = delete;
    Canvas &operator=(const Canvas &) = delete;
    Canvas(Canvas &&) = delete;
    Canvas &operator=(Canvas &&) = delete;

    ttk::Root &root() { return _root; }
    ttk::Surface &surface() { return _surface; }
    [[nodiscard]] int width() const { return _width; }
    [[nodiscard]] int height() const { return _height; }

    // Settles, paints what is damaged and presents. Answers the pixels painted.
    std::size_t frame();

    // Advances the clock to `now`, then as frame().
    std::size_t frame_at(double now);

    // With the whole window damaged.
    std::size_t full();

    // A copy of every pixel, row by row, as 0xAARRGGBB.
    [[nodiscard]] std::vector<std::uint32_t> pixels() const;

    // One pixel, as 0xAARRGGBB. Outside the canvas it is 0.
    [[nodiscard]] std::uint32_t at(int x, int y) const;

    // Whether the pixels standing equal a fresh repaint of the whole window.
    // Repaints the window, so call it last in a case or paint again after.
    bool matches_full();

private:
    ttk::Surface _surface;
    ttk::Root _root;
    int _width;
    int _height;
};
}

#endif //TTK_TESTS_CANVAS_H
