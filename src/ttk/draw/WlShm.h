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
    // A framebuffer of our own on Wayland, where SDL has none: a ring of wl_shm
    // buffers attached to the window's wl_surface, the frame drawn straight into
    // whichever the compositor is not holding. Built with TTK_WLSHM, otherwise every
    // call answers that there is nothing here.
    class WlShm {
    public:
        WlShm();
        ~WlShm();

        WlShm(const WlShm &) = delete;
        WlShm &operator=(const WlShm &) = delete;
        WlShm(WlShm &&) = delete;
        WlShm &operator=(WlShm &&) = delete;

        // True where the window is on the Wayland driver and hands out its display
        // and surface.
        static bool available(SDL_Window *window);

        // Binds wl_shm on the window's display. False where the compositor has none.
        bool open(SDL_Window *window) ;

        // Lets go of every buffer and the display objects behind them. Called before
        // SDL closes the display.
        void close();

        bool opened() ;

        struct Target {
            void *pixels = nullptr;
            int stride = 0;

            // The buffer does not hold the last frame and the caller repaints it whole.
            bool fresh = false;
        };

        // The buffer the next frame is drawn into, at the window's pixel size, brought
        // up to the last frame where the caller is not about to repaint the `whole` of
        // it. Held until presented, so asking again in the same frame answers the same
        // one.
        bool acquire(int width, int height, bool whole, Target &target) ;

        // Attaches the held buffer, damages `regions` and commits. False with no
        // buffer held, in which case nothing reached the compositor.
        bool present(std::span<const BLRectI> regions) ;

        struct Impl;

    private:
        std::unique_ptr<Impl> _impl;
    };
}


#endif //TTK_DRAW_WLSHM_H
