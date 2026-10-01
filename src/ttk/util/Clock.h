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
    //! Timers of the frame loop and a way to reach the interface thread from another thread.
    //!
    //! Timer callbacks run on the interface thread when the frame loop finds them due, which is checked once per
    //! turn of the loop, so they can run up to a frame late. A pending timer limits how long the loop sleeps.
    class Clock {
    public:
        Clock() = default;
        virtual ~Clock() = default;

        Clock(const Clock &) = delete;
        Clock &operator=(const Clock &) = delete;
        Clock(Clock &&) = delete;
        Clock &operator=(Clock &&) = delete;

        //! Runs `what` every `seconds` until \ref cancel() is called with the returned id.
        //!
        //! The first run is `seconds` from now and each next one `seconds` after the previous run. A non-positive
        //! `seconds` runs `what` once, on the next turn of the loop. Call it on the interface thread only.
        virtual int every(double seconds, std::function<void()> what) = 0;

        //! Runs `what` once, `seconds` from now, and returns an id that \ref cancel() accepts until then.
        //!
        //! Call it on the interface thread only.
        virtual int after(double seconds, std::function<void()> what) = 0;

        //! Removes the timer `id` returned by \ref every() or \ref after(). Does nothing for an id that is not
        //! pending. Safe to call from inside a timer's callback. Call it on the interface thread only.
        virtual void cancel(int id) = 0;

        //! Queues `what` to run on the interface thread on the next turn of the loop, and wakes the loop.
        //!
        //! Safe to call from any thread. Queued functions run in the order they were posted.
        virtual void post(std::function<void()> what) = 0;
    };
}


#endif //TTK_UTIL_CLOCK_H
