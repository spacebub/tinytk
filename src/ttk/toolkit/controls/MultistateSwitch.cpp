// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <utility>

#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/controls/MultistateSwitch.h"

namespace ttk {
MultistateSwitch::MultistateSwitch(std::function<void(int)> selected) : _selected(std::move(selected)) {
    _takesPointer = true;
    cursor = Cursor::Pointer;
}

void MultistateSwitch::set_options(const std::vector<Choice> &options) {
    if (options == _options) {
        return;
    }

    _options = options;

    options_changed();
}

void MultistateSwitch::set_options(std::vector<Choice> &&options) {
    if (options == _options) {
        return;
    }

    _options = std::move(options);

    options_changed();
}

void MultistateSwitch::options_changed() {
    _marked = false;

    // Hit testing reads the lanes before the relayout comes round, so they follow the options now.
    if (root() != nullptr) {
        measure_lanes(root()->type());
        root()->relayout();
    } else {
        _lanes.clear();
    }

    invalidate();
}

void MultistateSwitch::set_current(const int value) {
    if (_current == value) {
        return;
    }

    _current = value;

    invalidate();
    wake();
}

MultistateSwitch *MultistateSwitch::fill(const bool value) {
    if (_fill == value) {
        return this;
    }

    _fill = value;
    _marked = false;

    if (root() != nullptr) {
        root()->relayout();
    }

    invalidate();

    return this;
}

void MultistateSwitch::measure_lanes(Typeface &type) {
    const BLFont &face = type.at(400, Theme::fontBody);
    const double share = _options.empty() ? 0.0 : std::max(0.0, _box.w - 8.0) / static_cast<double>(_options.size());

    double x = _box.x + 4.0;

    _lanes.clear();

    for (const Choice &option : _options) {
        const double wide = _fill ? share : type.width(face, option.label) + 34.0;

        _lanes.emplace_back(x, _box.y + 4.0, wide, _box.h - 8.0);

        x += wide;
    }
}

double MultistateSwitch::natural_width(Typeface &type) {
    if (fixedWidth >= 0.0) {
        return fixedWidth;
    }

    const BLFont &face = type.at(400, Theme::fontBody);
    double total = 8.0;

    for (const Choice &option : _options) {
        total += type.width(face, option.label) + 34.0;
    }

    return total;
}

// The marker is placed against the lanes every time they move, so a resize does
// not leave it where the control used to be.
void MultistateSwitch::arrange(Typeface &type) {
    measure_lanes(type);

    for (size_t at = 0; at < _lanes.size(); ++at) {
        if (_options[at].value != _current) {
            continue;
        }

        if (!_marked || !_markX.live()) {
            _marked = true;

            _markX.set(static_cast<float>(_lanes[at].x));
            _markWidth.set(static_cast<float>(_lanes[at].w));
        }

        return;
    }

    // Nothing is current, so nothing is marked.
    _markWidth.set(0.0F);
}

void MultistateSwitch::paint(const Painter &painter) {
    const Theme::Palette &palette = Theme::palette();
    const double radius = _box.h / 2.0;

    painter.round(_box, radius, palette.sunken);
    painter.outline(_box, radius, 1.0, palette.border);

    if (_markWidth.value() > 0.0) {
        const BLRect mark{_markX.value(), _box.y + 4.0, _markWidth.value(), _box.h - 8.0};

        painter.round(mark, mark.h / 2.0, palette.accentSoft);
        painter.outline(mark, mark.h / 2.0, 1.0, Theme::alpha(palette.accent, 0.45));
    }

    for (size_t at = 0; at < _lanes.size(); ++at) {
        const bool active = _options[at].value == _current;
        const BLRgba32 ink = active ? palette.accent : std::cmp_equal(at, _over) ? palette.text : palette.muted;

        const BLRect &lane = _lanes[at];

        painter.label(painter.font(active ? 600 : 400, Theme::fontBody), lane, Align::Centre, _options[at].label, ink);

        if (_options[at].badge) {
            painter.circle(BLPoint{lane.x + lane.w - 11.0, lane.y + 10.0}, 3.0, palette.accent);
        }
    }
}

bool MultistateSwitch::press(const Pointer & /*at*/) {
    return enabled();
}

void MultistateSwitch::release(const Pointer &at) {
    if (!holds(at.x, at.y)) {
        return;
    }

    for (size_t index = 0; index < _lanes.size(); ++index) {
        if (at.x >= _lanes[index].x && at.x < _lanes[index].x + _lanes[index].w && _selected) {
            _selected(_options[index].value);

            return;
        }
    }
}

bool MultistateSwitch::key(const Key &pressed) {
    const int last = static_cast<int>(_options.size()) - 1;
    const auto found = _current ? std::ranges::find(_options, *_current, &Choice::value) : _options.end();
    const int at = found == _options.end() ? -1 : static_cast<int>(found - _options.begin());
    const int to = [&] {
        switch (pressed.code) {
            case Code::Left:
                return std::max(at - 1, 0);
            case Code::Right:
                return std::min(at + 1, last);
            case Code::Home:
                return 0;
            case Code::End:
                return last;
            default:
                return -1;
        }
    }();

    if (to < 0) {
        return pressed.code == Code::Return || pressed.code == Code::Space;
    }

    if (to != at && to <= last && _selected) {
        _selected(_options[to].value);
    }

    return true;
}

void MultistateSwitch::hover(const Pointer &at) {
    int over = -1;

    for (size_t index = 0; index < _lanes.size(); ++index) {
        if (at.x >= _lanes[index].x && at.x < _lanes[index].x + _lanes[index].w) {
            over = static_cast<int>(index);
        }
    }

    if (over != _over) {
        _over = over;

        invalidate();
    }
}

void MultistateSwitch::leave() {
    Widget::leave();

    _over = -1;
}

bool MultistateSwitch::advance(const double now) {
    for (size_t at = 0; at < _lanes.size(); ++at) {
        if (_options[at].value != _current) {
            continue;
        }

        if (!_marked) {
            _marked = true;
            _markX.set(static_cast<float>(_lanes[at].x));
            _markWidth.set(static_cast<float>(_lanes[at].w));
        } else {
            _markX.toward(static_cast<float>(_lanes[at].x), now, 0.18, Anim::Curve::CubicOut);
            _markWidth.toward(static_cast<float>(_lanes[at].w), now, 0.18, Anim::Curve::CubicOut);
        }
    }

    _markX.advance(now);
    _markWidth.advance(now);

    invalidate();

    return _markX.live() || _markWidth.live();
}
}
