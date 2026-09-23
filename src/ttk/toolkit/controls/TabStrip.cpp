// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <utility>

#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/controls/TabStrip.h"

namespace ttk {
    TabStrip::TabStrip(std::function<void(int)> selected) : _selected(std::move(selected)) {
        _takesPointer = true;
        cursor = Cursor::Pointer;
    }

    void TabStrip::set_tabs(std::vector<Tab> tabs) {
        _tabs.clear();

        for (Tab &tab : tabs) {
            Held held;

            held.tab = std::move(tab);
            _tabs.push_back(std::move(held));
        }

        _over = -1;

        if (_current >= 0 && std::cmp_greater_equal(_current, _tabs.size())) {
            _current = -1;
        }

        for (size_t index = 0; index < _tabs.size(); ++index) {
            _tabs[index].on.set(std::cmp_equal(index, _current) ? 1.0F : 0.0F);
        }

        if (root() != nullptr) {
            root()->relayout();
        }
    }

    void TabStrip::set_current(const int index) {
        if (index == _current) {
            return;
        }

        _current = index;

        for (size_t at = 0; at < _tabs.size(); ++at) {
            _tabs[at].on.toward(std::cmp_equal(at, _current) ? 1.0F : 0.0F, now(), 0.14, Anim::Curve::CubicOut);
        }

        wake();
    }

    void TabStrip::set_badge(const int index, const bool badge) {
        if (index < 0 || std::cmp_greater_equal(index, _tabs.size()) || _tabs[static_cast<size_t>(index)].tab.badge == badge) {
            return;
        }

        _tabs[static_cast<size_t>(index)].tab.badge = badge;
        invalidate();
    }

    double TabStrip::natural_width(Typeface &type) {
        const BLFont &face = type.at(600, Theme::fontBody);
        double total = 0.0;

        for (Held &held : _tabs) {
            held.width = type.width(face, held.tab.label);
            total += held.width + SIDES + GAP;
        }

        return std::max(0.0, total - GAP);
    }

    double TabStrip::natural_height(Typeface & /*type*/, double /*width*/) {
        return Theme::control;
    }

    void TabStrip::arrange(Typeface &type) {
        const BLFont &face = type.at(600, Theme::fontBody);
        double at = _box.x;

        for (Held &held : _tabs) {
            held.width = type.width(face, held.tab.label);
            held.box = BLRect{at, _box.y, held.width + SIDES, _box.h};
            at += held.box.w + GAP;
        }
    }

    void TabStrip::paint(const Painter &painter) {
        const Theme::Palette &palette = Theme::palette();

        for (size_t index = 0; index < _tabs.size(); ++index) {
            const Held &held = _tabs[index];
            const double on = held.on.value();
            const bool lit = std::cmp_equal(index, _over);

            painter.label(painter.font(on > 0.5 ? 600 : 400, Theme::fontBody), held.box, Align::Centre, held.tab.label,
                          on > 0.5 || lit ? palette.text : palette.faint);

            if (on > 0.0) {
                const double wide = held.width + 12.0;

                painter.round(BLRect{held.box.x + ((held.box.w - wide) / 2.0), held.box.y + held.box.h - 2.0, wide, 2.0},
                              1.0, Theme::alpha(palette.accent, on));
            }

            if (held.tab.badge && on <= 0.5) {
                painter.circle(BLPoint{held.box.x + held.box.w - 11.0, held.box.y + 16.0}, 3.0, palette.accent);
            }
        }
    }

    int TabStrip::at_point(const double x, const double y) const {
        for (size_t index = 0; index < _tabs.size(); ++index) {
            if (const BLRect &box = _tabs[index].box;
                x >= box.x && x < box.x + box.w && y >= box.y && y < box.y + box.h) {
                return static_cast<int>(index);
            }
        }

        return -1;
    }

    bool TabStrip::press(const Pointer &at) {
        return at_point(at.x, at.y) >= 0;
    }

    void TabStrip::release(const Pointer &at) {
        if (const int index = at_point(at.x, at.y); index >= 0 && _selected) {
            _selected(index);
        }
    }

    void TabStrip::hover(const Pointer &at) {
        if (const int over = at_point(at.x, at.y); over != _over) {
            _over = over;
            invalidate();
        }
    }

    void TabStrip::leave() {
        Widget::leave();
        _over = -1;
        invalidate();
    }

    bool TabStrip::advance(const double now) {
        bool live = false;

        for (Held &held : _tabs) {
            live = live || held.on.live();
            held.on.advance(now);
        }

        if (live) {
            invalidate();
        }

        return live;
    }
}
