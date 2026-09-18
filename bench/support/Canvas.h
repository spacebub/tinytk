// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_BENCH_CANVAS_H
#define TTK_BENCH_CANVAS_H

#include <cstddef>
#include <memory>
#include <utility>

#include <blend2d/blend2d.h>

#include "ttk/draw/Surface.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"

namespace bench {
    // One process wide face set. Loading one costs several milliseconds of file
    // reading that no benchmark wants to measure.
    ttk::Typeface &fonts();
    bool fonts_loaded();

    // A window's pixels with no window: the surface and compose step Shell drives,
    // minus SDL.
    class Canvas {
    public:
        explicit Canvas(int width = 1280, int height = 800);
        ~Canvas();

        Canvas(const Canvas &) = delete;
        Canvas &operator=(const Canvas &) = delete;
        Canvas(Canvas &&) = delete;
        Canvas &operator=(Canvas &&) = delete;

        ttk::Root &ui() { return *_root; }
        ttk::Typeface &type() const { return fonts(); }
        BLContext &context() { return _surface.context(); }
        const BLImage &image() const { return _surface.image(); }
        int width() const { return _surface.width(); }
        int height() const { return _surface.height(); }

        // Mirrors Shell::draw. Answers the pixels painted.
        std::size_t frame();
        std::size_t frame_at(double now);

        // Paints what is owed without moving the clock, as the shell's expose path
        // does. A tween started before it will not have advanced by the time it draws.
        std::size_t pending();

        // With the whole window damaged.
        std::size_t full();

        void resize(int width, int height);

        // Advances the frame clock by one refresh at `hz`.
        double tick(double hz = 280.0);
        double now() const { return _now; }

        // Lays the tree out without painting, so a layout can be measured alone.
        void settle();

        // Back to how it started: nothing in the page or the layers, nothing hovered,
        // held or focused, no damage standing and the clock at zero.
        void bare();

    private:
        ttk::Surface _surface;
        std::unique_ptr<ttk::Root> _root;
        double _now = 0.0;
    };

    // Alone in the page, placed at the size it asks for when one is not given.
    template <typename Kind>
    Kind *mount(Canvas &canvas, std::unique_ptr<Kind> widget, const double width = 0.0,
                const double height = 0.0) {
        canvas.bare();

        Kind *raw = canvas.ui().content()->append(std::move(widget));

        // Root attaches the tree in settle(). Without it the widget has no root and
        // popups, damage and animation all go nowhere.
        canvas.ui().relayout();
        canvas.ui().settle();

        ttk::Typeface &type = canvas.type();
        const double across = width > 0.0 ? width : raw->wanted_width(type);
        const double down = height > 0.0 ? height : raw->wanted_height(type, across);

        raw->place(BLRect{0.0, 0.0, across, down}, type);

        return raw;
    }

    // One widget through one Painter, with no tree, damage or clip above it.
    std::size_t paint_once(Canvas &canvas, ttk::Widget &widget);
}

#endif //TTK_BENCH_CANVAS_H
