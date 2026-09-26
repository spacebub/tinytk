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

#include <SDL3/SDL.h>

#include "ttk/draw/Surface.h"

namespace ttk {
    namespace {

    }

    bool Surface::sync(SDL_Window *window) {
        if (!_ready) {
            return attach(window);
        }

        if (_path == Path::Shared) {
            int wide = 0;
            int tall = 0;

            SDL_GetWindowSizeInPixels(window, &wide, &tall);

            return wide == _width && tall == _height ? retarget() : attach(window);
        }

        SDL_Surface *surface = SDL_GetWindowSurface(window);

        if (surface == nullptr) {
            // Minimized, or the platform has nothing to give right now.
            detach();

            return false;
        }

        if (surface->w == _width && surface->h == _height) {
            if (surface == _surface && surface->pixels == _pixels) {
                return true;
            }

            // Off the direct path our own buffer still holds the frame, so there is
            // nothing to re-wrap. SDL's buffer is new though, and a frame with no damage
            // would present nothing into it and leave whatever was in that memory on screen.
            if (_path == Path::Copied) {
                _surface = surface;
                _pixels = surface->pixels;

                damage_all();

                return true;
            }
        }

        return attach(window);
    }

    bool Surface::attach(SDL_Window *window) {
        release();

        // Wayland has no framebuffer for SDL to hand out, so the frame goes into
        // shared memory of ours where that is built in. Where it is not, or the
        // compositor has no wl_shm, SDL's fallback below is a GPU texture.
        if (WlShm::available(window) && (_shm.opened() || _shm.open(window))) {
            return attach_shared(window);
        }

        _shm.close();

        SDL_Surface *surface = SDL_GetWindowSurface(window);

        if (surface == nullptr) {
            return false;
        }

        // A window surface carries no meaningful alpha. Both of these are drawn into
        // as opaque, which is also the cheaper pipeline.
        if (surface->format != SDL_PIXELFORMAT_XRGB8888
            && surface->format != SDL_PIXELFORMAT_ARGB8888) {
            return false;
        }

        const char *driver = SDL_GetCurrentVideoDriver();

        _path = driver != nullptr
                && (SDL_strcmp(driver, "windows") == 0 || SDL_strcmp(driver, "x11") == 0)
            ? Path::Direct
            : Path::Copied;

        const BLResult made = _path == Path::Direct
            ? _image.create_from_data(surface->w, surface->h, BL_FORMAT_XRGB32, surface->pixels,
                                      surface->pitch)
            : _image.create(surface->w, surface->h, BL_FORMAT_XRGB32);

        if (made != BL_SUCCESS) {
            return false;
        }

        if (_context.begin(_image) != BL_SUCCESS) {
            _image.reset();

            return false;
        }

        _surface = surface;
        _pixels = surface->pixels;
        _width = surface->w;
        _height = surface->h;
        _ready = true;

        _damage.resize(_width, _height);
        damage_all();

        return true;
    }

    bool Surface::attach_shared(SDL_Window *window) {
        int wide = 0;
        int tall = 0;

        SDL_GetWindowSizeInPixels(window, &wide, &tall);

        if (wide <= 0 || tall <= 0) {
            return false;
        }

        _path = Path::Shared;
        _width = wide;
        _height = tall;

        _damage.resize(_width, _height);
        _damage.all();
        _moved.clear();

        return retarget();
    }

    bool Surface::retarget() {
        WlShm::Target target;

        // A frame about to be repainted whole has nothing to carry over from the last.
        if (!_shm.acquire(_width, _height, _damage.whole(), target)) {
            release();

            return false;
        }

        if (target.fresh) {
            damage_all();
        }

        if (_ready && target.pixels == _pixels) {
            return true;
        }

        if (_ready) {
            _context.end();
        }

        if (_image.create_from_data(_width, _height, BL_FORMAT_XRGB32, target.pixels, target.stride)
                != BL_SUCCESS
            || _context.begin(_image) != BL_SUCCESS) {
            release();

            return false;
        }

        _pixels = target.pixels;
        _ready = true;

        return true;
    }

    void Surface::release() {
        if (_ready) {
            _context.end();
            _image.reset();
        }

        _damage.resize(0, 0);
        _moved.clear();
        _surface = nullptr;
        _pixels = nullptr;
        _ready = false;
        _width = 0;
        _height = 0;
    }

    void Surface::detach() {
        release();

        _shm.close();
    }

    void Surface::damage(const BLRect &region) {
        if (_ready) {
            _damage.add(region);
        }
    }

    void Surface::damage_all() {
        if (!_ready) {
            return;
        }

        _damage.all();
        _moved.clear();
    }

    void Surface::shift(const BLRectI &wanted, const int dy) {
        if (!_ready || dy == 0) {
            return;
        }

        const int left = std::max(0, wanted.x);
        const int top = std::max(0, wanted.y);
        const int right = std::min(_width, wanted.x + wanted.w);
        const int bottom = std::min(_height, wanted.y + wanted.h);

        if (right <= left || bottom <= top) {
            return;
        }

        const BLRectI region{left, top, right - left, bottom - top};

        if (std::abs(dy) >= region.h) {
            damage(BLRect{static_cast<double>(region.x), static_cast<double>(region.y),
                          static_cast<double>(region.w), static_cast<double>(region.h)});

            return;
        }

        _context.flush(BL_CONTEXT_FLUSH_SYNC);

        BLImageData data{};

        if (_image.get_data(&data) != BL_SUCCESS) {
            return;
        }

        // Ours to write: the image is our own or wraps a buffer of the window's.
        auto *pixels = static_cast<uint8_t *>(data.pixel_data);
        const size_t wide = static_cast<size_t>(region.w) * 4;
        const size_t at = static_cast<size_t>(region.x) * 4;

        const auto row = [&](const int y) {
            return pixels + (static_cast<size_t>(y) * data.stride) + at;
        };

        if (dy < 0) {
            for (int y = region.y; y < region.y + region.h + dy; ++y) {
                SDL_memcpy(row(y), row(y - dy), wide);
            }
        } else {
            for (int y = region.y + region.h - 1; y >= region.y + dy; --y) {
                SDL_memcpy(row(y), row(y - dy), wide);
            }
        }

        // Whatever was due a repaint inside has gone with the pixels.
        const std::vector<BLRectI> pending = _damage.regions();

        for (const BLRectI &held : pending) {
            if (held.x < region.x + region.w && held.x + held.w > region.x
                && held.y < region.y + region.h && held.y + held.h > region.y) {
                damage(BLRect{static_cast<double>(held.x), static_cast<double>(held.y + dy),
                              static_cast<double>(held.w), static_cast<double>(held.h)});
            }
        }

        _moved.push_back(region);
    }

    bool Surface::attach(const int width, const int height) {
        detach();

        if (_image.create(width, height, BL_FORMAT_XRGB32) != BL_SUCCESS) {
            return false;
        }

        if (_context.begin(_image) != BL_SUCCESS) {
            _image.reset();

            return false;
        }

        _path = Path::Copied;
        _width = width;
        _height = height;
        _ready = true;

        _damage.resize(_width, _height);
        damage_all();

        return true;
    }

    void Surface::present() {
        if (!_ready) {
            return;
        }

        _context.flush(BL_CONTEXT_FLUSH_SYNC);
        _damage.clear();
        _moved.clear();
    }

    void Surface::present(SDL_Window *window) {
        if (!_ready || _damage.empty()) {
            return;
        }

        // Everything queued has to have landed in the pixels before the desktop reads
        // them. The context is synchronous, so this is the one place it has to be said.
        _context.flush(BL_CONTEXT_FLUSH_SYNC);

        if (_path == Path::Shared) {
            // A buffer committed before the surface has its role is a protocol error.
            // The damage stands, and the frame after the window is shown is drawn whole.
            if ((SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN) != 0) {
                return;
            }

            _presented.clear();
            _presented.insert(_presented.end(), _damage.regions().begin(), _damage.regions().end());
            _presented.insert(_presented.end(), _moved.begin(), _moved.end());

            if (_shm.present(_presented)) {
                _damage.clear();
                _moved.clear();
            }

            return;
        }

        // Off the direct path our own pixels have to be handed over, the damaged
        // rectangles alone unless the surface is one take() has not seen.
        if (_path == Path::Copied && !take(window)) {
            return;
        }

        std::vector<SDL_Rect> rects;

        rects.reserve(_damage.regions().size() + _moved.size());

        for (const BLRectI &region : _damage.regions()) {
            rects.push_back({.x = region.x, .y = region.y, .w = region.w, .h = region.h});
        }

        for (const BLRectI &region : _moved) {
            rects.push_back({.x = region.x, .y = region.y, .w = region.w, .h = region.h});
        }

        SDL_UpdateWindowSurfaceRects(window, rects.data(), static_cast<int>(rects.size()));

        _damage.clear();
        _moved.clear();
    }

    bool Surface::take(SDL_Window *window) {
        SDL_Surface *surface = SDL_GetWindowSurface(window);

        if (surface == nullptr || surface->w != _width || surface->h != _height) {
            return false;
        }

        BLImageData data{};

        if (_image.get_data(&data) != BL_SUCCESS) {
            return false;
        }

        // SDL rotates between surfaces of its own off the direct path. One not filled
        // before holds whatever was last in that memory, and only this frame's damage
        // would go over it. The image behind is whole, so all of it does.
        if (surface != _surface || surface->pixels != _pixels) {
            damage_all();
        }

        const auto *from = static_cast<const uint8_t *>(data.pixel_data);
        auto *to = static_cast<uint8_t *>(surface->pixels);

        const auto copy = [&](const BLRectI &region) {
            const size_t wide = static_cast<size_t>(region.w) * 4;
            const size_t at = static_cast<size_t>(region.x) * 4;

            for (int row = region.y; row < region.y + region.h; ++row) {
                SDL_memcpy(to + (static_cast<size_t>(row) * surface->pitch) + at,
                           from + (static_cast<size_t>(row) * data.stride) + at, wide);
            }
        };

        for (const BLRectI &region : _damage.regions()) {
            copy(region);
        }

        for (const BLRectI &region : _moved) {
            copy(region);
        }

        _surface = surface;
        _pixels = surface->pixels;

        return true;
    }

    bool Surface::save(const char *path) {
        if (!_ready) {
            return false;
        }

        _context.flush(BL_CONTEXT_FLUSH_SYNC);

        // The image wraps the pixels handed to the desktop, so this writes exactly
        // what is on screen.
        return _image.write_to_file(path) == BL_SUCCESS;
    }
}
