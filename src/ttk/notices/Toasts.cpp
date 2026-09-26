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

#include "ttk/notices/Toasts.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/controls/GlyphButton.h"

namespace ttk {

    Toasts::Toasts(std::function<void(int)> dismissed) : _dismissed(std::move(dismissed)) {}

    Toast::Toast(Notice message, std::function<void()> close)
        : _message(std::move(message)), _close(std::move(close)), _left(_message.duration) {
        _takesPointer = true;

        _shut = append(std::make_unique<GlyphButton>(Glyphs::Glyph::Close, [this] { this->close(); }));
        _shut->size(24.0)->tone(&Theme::Palette::faint, &Theme::Palette::text);
        _shut->fixedWidth = 24.0;
        _shut->fixedHeight = 24.0;
    }

    void Toasts::set_messages(const std::vector<Notice> &messages) {
        std::vector<int> wanted;

        wanted.reserve(messages.size());

        for (const Notice &message : messages) {
            wanted.push_back(message.id);
        }

        if (wanted == _shown) {
            return;
        }

        // What has gone is dropped and what is new is added. The rest stay as they
        // are, since rebuilding them would start their countdowns over.
        std::vector<const Widget *> gone;

        for (const Ptr &child : children()) {
            if (const int id = dynamic_cast<const Toast *>(child.get())->id();
                std::ranges::find(wanted, id) == wanted.end()) {
                gone.push_back(child.get());
            }
        }

        for (const Widget *child : gone) {
            erase(child);
        }

        for (const Notice &message : messages) {
            const int id = message.id;

            if (std::ranges::find(_shown, id) != _shown.end()) {
                continue;
            }

            append(std::make_unique<Toast>(message, [this, id] {
                if (_dismissed) {
                    _dismissed(id);
                }
            }));
        }

        _shown = std::move(wanted);

        if (root() != nullptr) {
            root()->relayout();
        }
    }

    void Toasts::arrange(Typeface &type) {
        // Inset from the window's corner, under the title bar.
        const double right = _box.x + _box.w - 18.0;

        double y = _box.y + Theme::barHeight + 18.0;

        for (const Ptr &child : children()) {
            const double tall = child->natural_height(type, 392.0);

            child->place(BLRect{right - 392.0, y, 392.0, tall}, type);

            y += tall + 10.0;
        }
    }

    void Toast::moved() {
        wake();
    }

    BLRgba32 Toast::tone() const {
        const Theme::Palette &palette = Theme::palette();

        switch (_message.severity) {
            case Severity::Error:
                return palette.danger;

            case Severity::Warning:
                return palette.warning;

            case Severity::Success:
                return palette.success;

            case Severity::Info:
                break;
        }

        return palette.accent;
    }

    BLRgba32 Toast::wash() const {
        const Theme::Palette &palette = Theme::palette();

        switch (_message.severity) {
            case Severity::Error:
                return palette.dangerSoft;

            case Severity::Warning:
                return palette.warningSoft;

            case Severity::Success:
                return palette.successSoft;

            case Severity::Info:
                break;
        }

        return palette.accentSoft;
    }

    double Toast::natural_height(Typeface &type, double /*width*/) {
        const BLFont &face = type.at(400, Theme::fontSmall);
        constexpr double room = WIDTH - 56.0;

        double tall = wrap_height(type, face, _message.body, room);

        if (!_message.title.empty()) {
            tall += wrap_height(type, type.at(Typeface::bold, Theme::fontSmall), _message.title, room)
                + 3.0;
        }

        return tall + 26.0;
    }

    void Toast::arrange(Typeface &type) {
        (void) type;

        _shut->place(BLRect{_box.x + _box.w - 32.0, _box.y + 9.0, 24.0, 24.0}, type);
    }

    void Toast::paint(const Painter &painter) {
        const Theme::Palette &palette = Theme::palette();
        const double here = _here.value();

        if (here <= 0.0) {
            return;
        }

        // The slide in and the fade out are the same tween, read as opacity.
        const BLRgba32 ink = Theme::alpha(tone(), here);
        const BLRgba32 text = Theme::alpha(palette.text, here);

        painter.round(_box, Theme::radius, Theme::alpha(wash(), here));
        painter.outline(_box, Theme::radius, 1.0, Theme::alpha(tone(), 0.42 * here));

        const double room = _box.w - 56.0;
        double y = _box.y + 13.0;

        if (!_message.title.empty()) {
            const BLFont &face = painter.font(Typeface::bold, Theme::fontSmall);

            painter.circle(BLPoint{_box.x + 20.0, y + (painter.line_height(face) / 2.0)}, 4.0, ink);

            y += painter.paragraph(face, BLRect{_box.x + 31.0, y, room - 15.0, 0.0}, _message.title,
                                   ink)
                + 3.0;

            painter.paragraph(painter.font(400, Theme::fontSmall),
                              BLRect{_box.x + 31.0, y, room - 15.0, 0.0}, _message.body, text);
        } else {
            const BLFont &face = painter.font(400, Theme::fontSmall);

            painter.circle(BLPoint{_box.x + 20.0, y + (painter.line_height(face) / 2.0)}, 4.0, ink);
            painter.paragraph(face, BLRect{_box.x + 31.0, y, room - 15.0, 0.0}, _message.body, text);
        }

        // Cut out of the frame's own shape, so the ends follow the corners instead of
        // sticking out past them.
        if (const BLRect bar = bar_box(); bar.w > 0.0) {
            painter.push(bar);
            painter.round(BLRect{_box.x + 1.0, _box.y + 1.0, _box.w - 2.0, _box.h - 2.0},
                          Theme::radius - 1.0, ink);
            painter.pop();
        }

        Widget::paint(painter);
    }

    void Toast::enter() {
        Widget::enter();
    }

    void Toast::leave() {
        Widget::leave();

        // The countdown resumes from here, not from the last tick before the hover.
        _ticked = 0.0;

        wake();
    }

    void Toast::close() {
        if (_going) {
            return;
        }

        _going = true;

        _here.run(0.0F, now(), 0.16, Anim::Curve::CubicOut);
        wake();
    }

    BLRect Toast::bar_box() const {
        if (_message.duration <= 0) {
            return BLRect{};
        }

        const double left = std::clamp(_left / _message.duration, 0.0, 1.0);

        return BLRect{_box.x + 1.0, _box.y + _box.h - 4.0, std::round((_box.w - 2.0) * left), 3.0};
    }

    bool Toast::advance(const double now) {
        if (!_started) {
            _started = true;

            _here.run(1.0F, now, 0.16, Anim::Curve::CubicOut);
        }

        const bool sliding = _here.live();

        _here.advance(now);

        // Anything but an error counts down. Hovering holds it.
        const bool counting = _message.duration > 0 && !_going && !hovered();

        if (counting) {
            if (_ticked > 0.0) {
                _left -= (now - _ticked) * 1000.0;
            }

            _ticked = now;

            if (_left <= 0.0) {
                close();
            }
        } else {
            _ticked = now;
        }

        if (_going && !_here.live() && _close) {
            const std::function<void()> going = _close;

            _close = nullptr;

            // The last frame of the fade has to be painted without it, whatever the
            // application does with the news and whenever it gets round to it.
            invalidate();

            going();

            return false;
        }

        if (sliding || _here.live()) {
            _bar = bar_box().w;

            invalidate();

            return true;
        }

        // Sitting still: only the bar moves, and only when it has lost a whole pixel.
        if (const double wide = bar_box().w; wide != _bar) {
            const double was = _bar;

            _bar = wide;

            invalidate(BLRect{_box.x + 1.0, _box.y + _box.h - 4.0, std::max(was, wide), 3.0});
        }

        // An error, or one the pointer is resting on, sits still until something
        // happens to it: close() and leave() wake it.
        if (!counting) {
            return false;
        }

        // One pixel of the bar is this many seconds of the countdown.
        return sleep_until(now + (_message.duration / std::max(_box.w - 2.0, 1.0) / 1000.0));
    }
}
