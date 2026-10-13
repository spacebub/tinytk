// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cstring>

#include "support/Canvas.h"
#include "ttk/toolkit/Compose.h"

namespace tests {
    namespace {

        bool &loaded() {
            static bool value = false;

            return value;
        }

    }

    ttk::Typeface &fonts() {
        static ttk::Typeface type = [] {
            ttk::Typeface made;

            loaded() = made.load();

            return made;
        }();

        return type;
    }

    bool fonts_loaded() {
        fonts();

        return loaded();
    }

    Canvas::Canvas(const int width, const int height) : _root(fonts()), _width(width), _height(height) {
        _surface.attach(width, height);
        _root.resize(width, height);
        _root.set_now(0.0);
    }

    Canvas::~Canvas() {
        _surface.detach();
    }

    std::size_t Canvas::frame() {
        const std::size_t painted = ttk::compose(_root, _surface);

        _surface.present();

        return painted;
    }

    std::size_t Canvas::frame_at(const double now) {
        _root.set_now(now);
        _root.advance(now);

        return frame();
    }

    std::size_t Canvas::full() {
        _root.damage_all();

        return frame();
    }

    std::vector<std::uint32_t> Canvas::pixels() const {
        BLImageData data{};
        std::vector<std::uint32_t> copy(static_cast<std::size_t>(_width) * static_cast<std::size_t>(_height));

        if (_surface.image().get_data(&data) != BL_SUCCESS) {
            return copy;
        }

        for (int y = 0; y < _height; ++y) {
            std::memcpy(copy.data() + (static_cast<std::ptrdiff_t>(y) * _width),
                        static_cast<const std::uint8_t *>(data.pixel_data)
                                + (static_cast<std::ptrdiff_t>(y) * data.stride),
                        static_cast<std::size_t>(_width) * 4);
        }

        return copy;
    }

    std::uint32_t Canvas::at(const int x, const int y) const {
        BLImageData data{};

        if (x < 0 || y < 0 || x >= _width || y >= _height || _surface.image().get_data(&data) != BL_SUCCESS) {
            return 0;
        }

        std::uint32_t pixel = 0;

        std::memcpy(&pixel,
                    static_cast<const std::uint8_t *>(data.pixel_data) + (static_cast<std::ptrdiff_t>(y) * data.stride)
                            + (static_cast<std::ptrdiff_t>(x) * 4),
                    4);

        return pixel;
    }

    bool Canvas::matches_full() {
        const std::vector<std::uint32_t> partial = pixels();

        full();

        return partial == pixels();
    }
}
