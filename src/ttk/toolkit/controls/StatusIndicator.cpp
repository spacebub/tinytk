// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "ttk/toolkit/controls/StatusIndicator.h"

#include <string_view>

#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"

namespace ttk {
    namespace {

        // The tile is dark in both shades.
        constexpr BLRgba32 TILE{0x9e000000};

        // Left pad, dot, gap and right pad around the word.
        constexpr double PAD = 10.0;
        constexpr double DOT = 3.5;
        constexpr double GAP = 6.0;
        constexpr double TEXT = PAD + (DOT * 2.0) + GAP;

        BLRgba32 tone_of(const StatusIndicator::Status status) {
            const Theme::Palette &palette = Theme::of();

            switch (status) {
                case StatusIndicator::Status::Launching:
                    return palette.accentHover;
                case StatusIndicator::Status::Running:
                    return palette.success;
                case StatusIndicator::Status::Stopping:
                case StatusIndicator::Status::Failed:
                    return palette.danger;
                case StatusIndicator::Status::Empty:
                case StatusIndicator::Status::Closed:
                    break;
            }

            return palette.muted;
        }

        std::string_view label_of(const StatusIndicator::Status status) {
            switch (status) {
                case StatusIndicator::Status::Launching:
                    return "Launching";
                case StatusIndicator::Status::Running:
                    return "Running";
                case StatusIndicator::Status::Stopping:
                    return "Stopping";
                case StatusIndicator::Status::Failed:
                    return "Failed";
                case StatusIndicator::Status::Empty:
                case StatusIndicator::Status::Closed:
                    break;
            }

            return "Closed";
        }

    }

    bool StatusIndicator::beats(const Status status) {
        return status == Status::Launching || status == Status::Stopping;
    }

    std::string StatusIndicator::say_of(const Status status, const std::string &reason) {
        switch (status) {
            case Status::Launching:
                return "It has been started and is loading.";
            case Status::Running:
                return "It is up.";
            case Status::Stopping:
                return "It has been asked to quit. One still loading does not hear until it is up.";
            case Status::Failed:
                return reason.empty() ? "It did not start." : reason;
            case Status::Empty:
            case Status::Closed:
                break;
        }

        return "It has been closed.";
    }

    double StatusIndicator::width_of(Typeface &type, const Status status) {
        if (status == Status::Empty) {
            return 0.0;
        }

        return type.width(type.at(600, Theme::fontTiny), label_of(status)) + TEXT + PAD;
    }

    BLRect StatusIndicator::render(const Painter &painter, const BLPoint at, const Status status,
                                   const bool dim) {
        if (status == Status::Empty) {
            return {};
        }

        const BLFont &face = painter.font(600, Theme::fontTiny);
        const std::string_view said = label_of(status);

        const BLRect box{at.x, at.y, painter.width(face, said) + TEXT + PAD, HEIGHT};
        const BLRgba32 tone = tone_of(status);
        const double radius = box.h / 2.0;

        painter.round(box, radius, TILE);
        painter.outline(box, radius, 1.0, Theme::alpha(tone, 0.5));

        painter.circle(BLPoint{box.x + PAD + DOT, box.y + (box.h / 2.0)}, DOT,
                       dim && beats(status) ? Theme::alpha(tone, 0.2) : tone);
        painter.label(face, BLRect{box.x + TEXT, box.y, box.w - TEXT, box.h}, Align::Start, said, tone);

        return box;
    }

    void StatusIndicator::set(const Status status, std::string reason) {
        if (_status == status && _reason == reason) {
            return;
        }

        _status = status;
        _reason = std::move(reason);
        _dim = false;

        retell();
        invalidate();

        if (beats(_status)) {
            wake();
        }
    }

    StatusIndicator *StatusIndicator::on_click(std::function<void()> clicked, std::string about) {
        _clicked = std::move(clicked);
        _about = std::move(about);
        _takesPointer = _clicked != nullptr;
        cursor = _takesPointer ? Cursor::Pointer : Cursor::Default;

        retell();

        return this;
    }

    void StatusIndicator::retell() {
        hint = _status == Status::Empty ? std::string() : say_of(_status, _reason);

        if (!hint.empty() && !_about.empty()) {
            hint += " " + _about;
        }
    }

    double StatusIndicator::natural_width(Typeface &type) {
        return fixedWidth >= 0.0 ? fixedWidth : width_of(type, _status);
    }

    void StatusIndicator::paint(const Painter &painter) {
        render(painter, BLPoint{_box.x, _box.y}, _status, _dim);
    }

    bool StatusIndicator::press(const Pointer & /*at*/) { return _clicked != nullptr; }

    void StatusIndicator::release(const Pointer &at) {
        if (_clicked && holds(at.x, at.y)) {
            _clicked();
        }
    }

    bool StatusIndicator::advance(const double now) {
        if (!beats(_status)) {
            return false;
        }

        if (now - _blinked >= BEAT) {
            _blinked = now;
            _dim = !_dim;

            invalidate();
        }

        return sleep_until(_blinked + BEAT);
    }
}
