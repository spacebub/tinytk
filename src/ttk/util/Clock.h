// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_UTIL_CLOCK_H
#define TTK_UTIL_CLOCK_H


#include <functional>

namespace ttk {
    // The frame loop's timers and its way back from a worker thread, without the window
    // around them. A service wants to be run again later. It has nothing to draw, and
    // depending on the window is what made the folder graph cyclic.
    class Clock {
    public:
        Clock() = default;
        virtual ~Clock() = default;

        Clock(const Clock &) = delete;
        Clock &operator=(const Clock &) = delete;
        Clock(Clock &&) = delete;
        Clock &operator=(Clock &&) = delete;

        // Runs `what` every `seconds` until cancelled. Keeps the loop awake.
        virtual int every(double seconds, std::function<void()> what) = 0;

        // Runs `what` once, `seconds` from now.
        virtual int after(double seconds, std::function<void()> what) = 0;

        virtual void cancel(int id) = 0;

        // Runs `what` on the interface thread, from any thread, and wakes the loop.
        virtual void post(std::function<void()> what) = 0;
    };
}


#endif //TTK_UTIL_CLOCK_H
