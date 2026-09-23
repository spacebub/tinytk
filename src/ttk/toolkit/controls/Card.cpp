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

#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/controls/Card.h"
#include "ttk/util/Format.h"

namespace ttk {
    Card::Card() {
        _takesPointer = true;
        hoverable = true;

        _row = append(Box::row());
        _row->spacing(8.0);
    }

    void Card::arrange(Typeface &type) {
        _row->place(BLRect{_box.x + 16.0, _box.y + _box.h - 16.0 - Theme::controlSmall, _box.w - 32.0,
                           Theme::controlSmall},
                    type);
    }

    void Card::paint(const Painter &painter) {
        lit = holds_pointer() || _carrying;

        Panel::paint(painter);

        const Theme::Palette &palette = Theme::palette();
        const BLFont &face = painter.font(600, Theme::fontMedium);
        double right = _box.x + _box.w - 16.0;

        if (_pills.size() != tags.size()) {
            _pills.assign(tags.size(), BLRect{});
        }

        for (size_t which = tags.size(); which-- > 0;) {
            const Tag &tag = tags[which];
            const BLRgba32 tone = Pill::tone_of(tag.kind);
            const BLRgba32 wash = Pill::wash_of(tag.kind);
            const BLRgba32 ink = palette.dark ? tone : Theme::darker(tone, 0.35);
            const BLFont &small = painter.font(600, Theme::fontSmall);
            double wide = painter.width(small, tag.text) + 22.0;

            if (tag.dot) {
                wide += 14.0;
            }

            const BLRect pill{right - wide, _box.y + 16.0, wide, 22.0};

            _pills[which] = pill;

            painter.round(pill, 11.0, wash);
            painter.outline(pill, 11.0, 1.0, Theme::alpha(ink, 0.3));

            double x = pill.x + 11.0;

            if (tag.dot) {
                painter.circle(BLPoint{x + 4.0, pill.y + (pill.h / 2.0)}, 4.0, ink);
                x += 14.0;
            }

            painter.label(small, BLRect{x, pill.y, pill.x + pill.w - 11.0 - x, pill.h}, Align::Start, tag.text, ink);

            right -= wide + 8.0;
        }

        painter.label(face, BLRect{_box.x + 16.0, _box.y + 16.0, right - _box.x - 24.0, 22.0}, Align::Start, title,
                      trouble ? palette.danger : palette.text);

        if (!subtitle.empty()) {
            painter.paragraph(painter.font(400, Theme::fontSmall), BLRect{_box.x + 16.0, _box.y + 46.0, _box.w - 32.0, 0.0},
                              subtitle, palette.faint);
        }

        if (!file.empty()) {
            const BLFont &mono = painter.font(Typeface::mono, Theme::fontTiny);
            const double unit = painter.width(mono, "M");
            const int room = unit > 0.0 ? static_cast<int>((_box.w - 32.0) / unit) : 0;

            painter.label(mono, BLRect{_box.x + 16.0, _box.y + 46.0, _box.w - 32.0, 18.0}, Align::Start,
                          Format::fit_path(file, room), palette.faint);
        }

        const double line = _row->box().y - 26.0;

        if (working) {
            const BLRect track{_box.x + 16.0, line + 5.0, _box.w - 32.0, 6.0};

            painter.round(track, 3.0, palette.sunken);
            painter.round(BLRect{track.x, track.y, track.w * std::clamp(progress, 0.0, 1.0), track.h}, 3.0,
                          palette.accent);
        } else if (!told.empty()) {
            painter.label(painter.font(400, Theme::fontSmall), BLRect{_box.x + 16.0, line, _box.w - 32.0, 16.0},
                          Align::Start, told, trouble ? palette.danger : palette.faint);
        }

        Widget::paint(painter);
    }

    bool Card::press(const Pointer &at) {
        _pressX = at.x;
        _pressY = at.y;
        _carrying = false;
        _armed = true;

        return true;
    }

    void Card::drag(const Pointer &at) {
        if (!draggable || !_armed) {
            return;
        }

        if (!_carrying) {
            if (std::abs(at.x - _pressX) < SLACK && std::abs(at.y - _pressY) < SLACK) {
                return;
            }

            _carrying = true;

            if (drag_started) {
                drag_started(_pressX, _pressY);
            }
        }

        if (drag_moved) {
            drag_moved(at.x, at.y);
        }
    }

    void Card::release(const Pointer &at) {
        const bool carried = _carrying;

        _armed = false;
        _carrying = false;

        if (carried) {
            if (drag_ended) {
                drag_ended();
            }

            return;
        }

        if (holds(at.x, at.y) && opened && at.y < _row->box().y) {
            opened();
        }
    }

    Cursor Card::cursor_at(double /*x*/, double /*y*/) const {
        return _carrying ? Cursor::Grabbing : Cursor::Default;
    }

    void Card::hover(const Pointer &at) {
        std::string said;

        for (size_t which = 0; which < _pills.size() && which < tags.size(); ++which) {
            if (const BLRect &pill = _pills[which];
                at.x >= pill.x && at.x < pill.x + pill.w && at.y >= pill.y && at.y < pill.y + pill.h) {
                said = tags[which].hint;

                break;
            }
        }

        hint = said;
    }

    void Card::leave() {
        Panel::leave();
        hint.clear();
    }

    // The buttons are on the card: the light stays while the pointer is on them.
    void Card::within(bool /*inside*/) {
        invalidate();
    }

    void Card::slide_from(const double x, const double y, const double now) {
        _slideX.set(static_cast<float>(x));
        _slideY.set(static_cast<float>(y));
        _slideX.run(0.0F, now, SETTLING, Anim::Curve::CubicOut);
        _slideY.run(0.0F, now, SETTLING, Anim::Curve::CubicOut);

        wake();
    }

    bool Card::advance(const double now) {
        // The slide is in the box here, so the grid lays it out again. What it was
        // has to be damaged before that, since the live list is not ordered.
        const BLRect was = _box;

        _slideX.advance(now);
        _slideY.advance(now);

        if (sliding()) {
            invalidate(was);
            invalidate();
        }

        return sliding();
    }
}
