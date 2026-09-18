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
    // The toasts, newest at the bottom.
    class Toasts : public ttk::Widget {
    public:
        explicit Toasts(std::function<void(int)> dismissed);

        void set_messages(const std::vector<Notice> &messages);

        void arrange(Typeface &type) override;

    private:
        std::function<void(int)> _dismissed;

        std::vector<int> _shown;
    };

    // One toast.
    class Toast : public ttk::Widget {
    public:
        Toast(Notice message, std::function<void()> close);

        [[nodiscard]] int id() const { return _message.id; }

        double natural_height(Typeface &type, double width) override;

        void arrange(Typeface &type) override;

        void paint(const ttk::Painter &painter) override;

        bool press(const ttk::Pointer &at) override { return holds(at.x, at.y); }

        void enter() override;
        void leave() override;

        bool advance(double now) override;

        // Starts the fade out. The stack drops it when it finishes.
        void close();

    protected:
        void moved() override;

    private:
        [[nodiscard]] BLRgba32 tone() const;
        [[nodiscard]] BLRgba32 wash() const;

        // The countdown bar along the bottom edge, rounded to whole pixels.
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
