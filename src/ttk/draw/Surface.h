// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_DRAW_SURFACE_H
#define TTK_DRAW_SURFACE_H


#include <vector>

#include <SDL3/SDL.h>
#include <blend2d/blend2d.h>

#include "ttk/draw/Damage.h"

namespace ttk {
    // The window's own pixels, drawn into directly where the video driver has a
    // framebuffer of its own and through a buffer of ours where it has not. Only the
    // rectangles that changed are repainted, and only those are presented.
    class Surface {
    public:
        // Takes, or retakes, the window's surface. Called again after every resize,
        // which is when SDL throws the old one away.
        bool attach(SDL_Window *window);

        // A target of its own, with no window behind it.
        bool attach(int width, int height);

        // True where the video driver has a framebuffer of its own, which is the only
        // case in which the window's pixels are drawn into in place.
        [[nodiscard]] bool direct() const { return _direct; }

        // Re-takes the surface if SDL has replaced it since the last look. Asked every
        // frame: an expose arriving before the resize event would otherwise draw into
        // memory SDL has already freed.
        bool sync(SDL_Window *window);

        // Lets go of the surface before SDL frees it.
        void detach();

        [[nodiscard]] bool ready() const { return _ready; }

        [[nodiscard]] int width() const { return _width; }
        [[nodiscard]] int height() const { return _height; }

        BLContext &context() { return _context; }

        [[nodiscard]] const BLImage &image() const { return _image; }

        // Marks a region for repaint. Rectangles outside the surface are dropped, and
        // ones that overlap are left alone: painting a pixel twice is cheaper than
        // working out that it would be.
        void damage(const BLRect &region);
        void damage_all();

        // Moves the pixels of `region` down by `dy` (up when negative) instead of
        // repainting them, carrying any pending damage inside it along. The region is
        // still presented whole.
        void shift(const BLRectI &wanted, int dy);

        [[nodiscard]] bool dirty() const { return !_damage.empty() || !_moved.empty(); }

        [[nodiscard]] const std::vector<BLRectI> &regions() const { return _damage.regions(); }

        // Pushes the damaged rectangles and forgets them.
        void present(SDL_Window *window);

        // Settles the pixels and forgets the damage, for a target with no window.
        void present();

        // The window's pixels to a PNG, which Blend2D encodes itself.
        bool save(const char *path);

    private:
        // Copies the damaged rectangles of our own buffer into SDL's.
        bool take(SDL_Window *window);

        BLImage _image;
        BLContext _context;

        // False where SDL keeps the window's pixels in a block it reallocates on every
        // reconfigure: a fresh block can land on the old address, so nothing about the
        // surface says it moved. Those draw into a buffer of ours and are copied over.
        bool _direct = true;

        // What was wrapped, so a replacement can be spotted.
        SDL_Surface *_surface = nullptr;
        void *_pixels = nullptr;

        Damage _damage;

        // Presented but not repainted.
        std::vector<BLRectI> _moved;

        int _width = 0;
        int _height = 0;
        bool _ready = false;
    };
}


#endif //TTK_DRAW_SURFACE_H
