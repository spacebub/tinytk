// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_DRAW_WLSHM_H
#define TTK_DRAW_WLSHM_H


#include <memory>
#include <span>

#include <SDL3/SDL.h>
#include <blend2d/blend2d.h>

namespace ttk {
    //! Framebuffer for a window on Wayland, made of shared memory buffers attached to the window's `wl_surface`.
    //!
    //! Keeps a ring of at most four `wl_shm` buffers in XRGB8888, and each frame is drawn straight into one the
    //! compositor is not holding. Wayland events are read on an event queue of its own, apart from SDL's. Only
    //! built with the `TTK_WLSHM` option on Linux, otherwise \ref available() and every other query return false.
    class WlShm {
    public:
        WlShm();
        ~WlShm();

        WlShm(const WlShm &) = delete;
        WlShm &operator=(const WlShm &) = delete;
        WlShm(WlShm &&) = delete;
        WlShm &operator=(WlShm &&) = delete;

        //! Tests whether `window` is on SDL's Wayland video driver and exposes its `wl_display` and `wl_surface`.
        //! Returns false for a null `window`.
        static bool available(SDL_Window *window);

        //! Closes anything open, then binds `wl_shm` on the display of `window` with a roundtrip to the compositor.
        //!
        //! Returns false when `window` exposes no Wayland display or surface, or the compositor offers no `wl_shm`.
        bool open(SDL_Window *window) ;

        //! Destroys every buffer and the Wayland objects behind them, including buffers the compositor still holds.
        //!
        //! Call it before SDL closes the display. The destructor calls it too.
        void close();

        //! Tests whether \ref open() succeeded and \ref close() has not been called since.
        bool opened() ;

        //! Pixels of the buffer a frame is drawn into, filled by \ref acquire().
        struct Target {
            //! First row of the buffer, in XRGB8888.
            void *pixels = nullptr;
            //! Bytes from one row to the next, a multiple of 64.
            int stride = 0;

            //! True when the buffer does not hold the last frame presented, so the caller must repaint it whole.
            bool fresh = false;
        };

        //! Stores in `target` the buffer the next frame of `width` by `height` pixels is drawn into.
        //!
        //! Takes the free buffer presented most recently. With none free it makes a new one while there are fewer
        //! than four, and otherwise takes back the one presented longest ago. Buffers of another size are freed once
        //! the compositor releases them. Unless `whole` is true, the buffer is first brought up to date with the
        //! frame on screen, or \ref Target::fresh is set when there is none to copy. The buffer stays held until
        //! \ref present(), so a second call at the same size returns it again. Returns false when not opened,
        //! for a non-positive size, or when no buffer can be made.
        bool acquire(int width, int height, bool whole, Target &target) ;

        //! Attaches the held buffer to the surface, damages `regions` in buffer pixels, commits and flushes.
        //!
        //! Compositors without buffer damage get the whole surface damaged. Returns false with nothing sent when no
        //! buffer is held. Afterwards none is, and \ref acquire() must be called for the next frame.
        bool present(std::span<const BLRectI> regions) ;

        //! Opaque state of the implementation.
        struct Impl;

    private:
        std::unique_ptr<Impl> _impl;
    };
}


#endif //TTK_DRAW_WLSHM_H
