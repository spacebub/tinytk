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


#include <cstdint>
#include <vector>

#include <SDL3/SDL.h>
#include <blend2d/blend2d.h>

#include "ttk/draw/Damage.h"
#include "ttk/draw/WlShm.h"

namespace ttk {
    //! Pixels of a window, drawn into with a Blend2D context and presented by damaged rectangles.
    //!
    //! On the Windows and X11 video drivers the context draws straight into SDL's window framebuffer. On Wayland,
    //! where shared memory is available, it draws into buffers handed to the compositor as they are. Elsewhere it
    //! draws into a buffer of its own whose damaged rectangles are copied to SDL's window surface when presented.
    //! The pixels are 32-bit XRGB and opaque. Sizes and rectangles are in pixels.
    class Surface {
    public:
        //! Releases any previous target, takes the surface of `window` and damages all of it.
        //!
        //! Returns false when `window` has no surface, its pixel format is not 32-bit XRGB or ARGB, or the target
        //! cannot be created. SDL replaces the window surface on every resize, which \ref sync() handles.
        bool attach(SDL_Window *window);

        //! Releases any previous target and creates one of `width` by `height` pixels with no window behind it,
        //! damaged whole. Returns false when the image cannot be created.
        bool attach(int width, int height);

        //! Tests whether the context draws in place into the window's framebuffer, which only the Windows and X11
        //! video drivers allow.
        [[nodiscard]] bool direct() const { return _path == Path::Direct; }

        //! Brings the target in line with `window`. Call it before painting every frame.
        //!
        //! Calls \ref attach() when nothing is attached or the size changed, takes over a window surface SDL replaced
        //! since the last call, and on Wayland binds the context to a buffer the compositor is not holding. Returns
        //! false when there is nothing to draw into, such as while the window is minimized, and then the surface is
        //! detached.
        bool sync(SDL_Window *window);

        //! Releases the target and any Wayland shared memory. Call it before SDL frees the window surface.
        void detach();

        //! Tests whether a target is attached and the context can draw.
        [[nodiscard]] bool ready() const { return _ready; }

        //! Returns the width of the target in pixels, or 0 when nothing is attached.
        [[nodiscard]] int width() const { return _width; }

        //! Returns the height of the target in pixels, or 0 when nothing is attached.
        [[nodiscard]] int height() const { return _height; }

        //! Returns the context that draws into the target. It can only draw while \ref ready() is true, and
        //! \ref attach() and \ref sync() can bind it to new pixels.
        BLContext &context() { return _context; }

        //! Returns the image the context draws into, which wraps the window's own pixels when \ref direct() is true
        //! or on Wayland.
        [[nodiscard]] const BLImage &image() const { return _image; }

        //! Marks `region` for repainting and presenting, as \ref Damage::add() does. Ignored while nothing is
        //! attached.
        void damage(const BLRect &region);

        //! Marks the whole target for repainting and presenting and drops pending shifts. Ignored while nothing is
        //! attached.
        void damage_all();

        //! Moves the pixels of `wanted`, clipped to the target, down by `dy` pixels, or up when `dy` is negative,
        //! instead of repainting them.
        //!
        //! Damage pending inside the region is also added `dy` further down. The strip the move uncovers is not
        //! damaged, so the caller must damage it. The region is presented whole by the next \ref present() that has
        //! damage. When `dy` is at least the height of the region, the region is damaged instead. Does nothing when
        //! `dy` is zero, nothing is attached or the region lies outside the target.
        void shift(const BLRectI &wanted, int dy);

        //! Tests whether anything was damaged or shifted since the last present.
        [[nodiscard]] bool dirty() const { return !_damage.empty() || !_moved.empty(); }

        //! Returns the damaged rectangles waiting to be repainted.
        [[nodiscard]] const std::vector<BLRectI> &regions() const { return _damage.regions(); }

        //! Flushes the context, hands the damaged and shifted rectangles to `window` and clears them.
        //!
        //! Does nothing while no rectangle is damaged, even after \ref shift(). The rectangles are kept for the next
        //! call when the window is hidden on Wayland, the compositor cannot take the buffer, or the window surface no
        //! longer matches the target in size.
        void present(SDL_Window *window);

        //! Flushes the context and clears all damage and shifts, for a target with no window.
        void present();

        //! Flushes the context and writes the pixels to the image file at `path`, in the format its extension names,
        //! such as PNG. Returns false when nothing is attached or the file cannot be written.
        bool save(const char *path);

    private:
        enum class Path : std::uint8_t {
            Direct,

            Copied,

            Shared,
        };

        bool attach_shared(SDL_Window *window);

        bool retarget();

        bool take(SDL_Window *window);

        void release();

        BLImage _image;
        BLContext _context;

        Path _path = Path::Direct;

        WlShm _shm;

        // What was wrapped, so a replacement can be spotted. Off the direct path SDL
        // keeps the window's pixels in a block it reallocates on every reconfigure: a
        // fresh block can land on the old address, so nothing about the surface says
        // it moved.
        SDL_Surface *_surface = nullptr;
        void *_pixels = nullptr;

        Damage _damage;

        std::vector<BLRectI> _moved;

        // Handed over each frame, kept so a frame allocates nothing.
        std::vector<BLRectI> _presented;

        int _width = 0;
        int _height = 0;
        bool _ready = false;
    };
}


#endif //TTK_DRAW_SURFACE_H
