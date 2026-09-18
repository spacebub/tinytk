// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_BUTTON_H
#define TTK_CONTROLS_BUTTON_H


#include <functional>
#include <string>

#include "ttk/draw/Anim.h"
#include "ttk/draw/Glyphs.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
    class Button : public Widget {
    public:
        enum class Kind : std::uint8_t {
            Default,
            Primary,
            Danger,
            Ghost,
        };

        Button(std::string text, std::function<void()> clicked);

        void set_text(std::string text);

        Button *kind(Kind value);
        Button *glyph(Glyphs::Glyph glyph);
        Button *compact(bool value = true);
        Button *busy(bool value);
        Button *tooltip(std::string text);

        // Never takes a row's spare width.
        double natural_width(Typeface &type) override;
        double natural_height(Typeface &type, double width) override;

        void paint(const Painter &painter) override;

        bool press(const Pointer &at) override;
        void release(const Pointer &at) override;
        void enter() override;
        void leave() override;

        [[nodiscard]] bool takes_focus() const override { return enabled(); }
        bool key(const Key &pressed) override;

        bool advance(double now) override;

    private:
        [[nodiscard]] BLRgba32 ink() const;

        std::string _text;
        Glyphs::Glyph _glyph{};
        std::function<void()> _clicked;

        Kind _kind = Kind::Default;
        bool _compact = false;
        bool _busy = false;

        Anim::Tween _lit;
        Anim::Tween _give;

        // The three dots, while busy.
        int _tick = 0;
        double _ticked = 0.0;
    };
}


#endif //TTK_CONTROLS_BUTTON_H
