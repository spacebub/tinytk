// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_NOTICES_TOASTS_H
#define TTK_NOTICES_TOASTS_H


#include <functional>
#include <vector>

#include "ttk/draw/Anim.h"
#include "ttk/notices/Notice.h"
#include "ttk/toolkit/controls/GlyphButton.h"

namespace ttk {
    //! Stack of \ref Toast widgets in the top-right corner of its box, below the title bar, newest at the bottom.
    //!
    //! The stack mirrors a list of notices, normally \ref Notifier::messages(), given to \ref set_messages().
    class Toasts : public ttk::Widget {
    public:
        //! Creates an empty stack. `dismissed` is called with a notice's id once its toast has faded out, whether
        //! it was closed by the user or by its countdown.
        //!
        //! The toast stays in the stack, painting nothing, until a later \ref set_messages() leaves its id out, so
        //! `dismissed` should remove the notice from the list, as \ref Notifier::dismiss() does.
        explicit Toasts(std::function<void(int)> dismissed);

        //! Matches the toasts to `messages` by id and asks the root for a new layout when anything changed.
        //!
        //! Toasts whose id is missing from `messages` are removed and a toast is appended for each new id.
        //! Toasts already shown are kept as they are, so their countdowns carry on.
        void set_messages(const std::vector<Notice> &messages);

        //! Places the toasts top down, 392 pixels wide and 10 pixels apart, inset 18 pixels from the right edge
        //! and from the bottom of the title bar.
        void arrange(Typeface &type) override;

    private:
        std::function<void(int)> _dismissed;

        std::vector<int> _shown;
    };

    //! Card that shows one \ref Notice in its severity's colour, with a close button and a countdown bar.
    //!
    //! The toast fades in when it first animates. A notice with a positive duration counts down while the pointer
    //! is not over the toast and closes itself at zero. The bar along the bottom edge shows the time left.
    class Toast : public ttk::Widget {
    public:
        //! Creates a toast for `message`. `close` is called once, when the fade out started by \ref close() has
        //! finished.
        Toast(Notice message, std::function<void()> close);

        //! Returns the id of the notice shown.
        [[nodiscard]] int id() const { return _message.id; }

        //! Returns the height of the title and body wrapped to the toast's fixed width, plus padding. `width` is
        //! ignored.
        double natural_height(Typeface &type, double width) override;

        //! Places the close button in the top-right corner.
        void arrange(Typeface &type) override;

        //! Paints the card, its text, the countdown bar and the close button, faded by the slide-in and fade-out
        //! animation.
        void paint(const ttk::Painter &painter) override;

        //! Takes every press inside the box, so presses do not reach what lies under the toast.
        bool press(const ttk::Pointer &at) override { return holds(at.x, at.y); }

        //! Called when the pointer moves onto the toast. Pauses the countdown while the pointer stays.
        void enter() override;

        //! Called when the pointer moves off the toast. Resumes the countdown from where it was paused.
        void leave() override;

        //! Steps the fade and the countdown, closes the toast when the countdown runs out, and calls the close
        //! callback once the fade out has finished.
        //!
        //! While only the bar moves it sleeps until the bar has lost a whole pixel. A toast without a countdown,
        //! or one under the pointer, sleeps until \ref close() or \ref leave() wakes it.
        bool advance(double now) override;

        //! Starts the fade out, after which the close callback is called. Does nothing when already closing.
        void close();

    protected:
        //! Called when the toast is placed. Wakes it so the next frame animates it.
        void moved() override;

    private:
        [[nodiscard]] BLRgba32 tone() const;
        [[nodiscard]] BLRgba32 wash() const;

        [[nodiscard]] BLRect bar_box() const;

        static constexpr double WIDTH = 392.0;

        Notice _message;
        std::function<void()> _close;

        ttk::GlyphButton *_shut = nullptr;

        double _left = 0.0;
        double _ticked = 0.0;

        // The bar's width as last drawn, so only a whole pixel of it is repainted.
        double _bar = -1.0;

        bool _going = false;
        bool _started = false;

        Anim::Tween _here;
    };
}


#endif //TTK_NOTICES_TOASTS_H
