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
#include <cstdint>
#include <limits>

#include "ttk/draw/Damage.h"

namespace ttk {
namespace {

BLRectI enclose(const BLRectI &one, const BLRectI &two) {
    const int left = std::min(one.x, two.x);
    const int top = std::min(one.y, two.y);
    const int right = std::max(one.x + one.w, two.x + two.w);
    const int bottom = std::max(one.y + one.h, two.y + two.h);

    return {left, top, right - left, bottom - top};
}

bool covers(const BLRectI &outer, const BLRectI &inner) {
    return inner.x >= outer.x && inner.y >= outer.y && inner.x + inner.w <= outer.x + outer.w
           && inner.y + inner.h <= outer.y + outer.h;
}

// Clean pixels the bounding rectangle of the two would repaint.
int64_t excess(const BLRectI &one, const BLRectI &two) {
    const BLRectI bound = enclose(one, two);
    const int w = std::max(0, std::min(one.x + one.w, two.x + two.w) - std::max(one.x, two.x));
    const int h = std::max(0, std::min(one.y + one.h, two.y + two.h) - std::max(one.y, two.y));

    return (int64_t{bound.w} * bound.h) - (int64_t{one.w} * one.h) - (int64_t{two.w} * two.h) + (int64_t{w} * h);
}
}

void Damage::resize(const int width, const int height) {
    _width = width;
    _height = height;
    _regions.clear();
}

BLRectI Damage::clamp_to(const BLRect &region) const {
    const double right_edge = region.x + region.w;
    const double bottom_edge = region.y + region.h;

    // A sum of the four is finite only when each of them is.
    if (_width <= 0 || _height <= 0 || !(region.w > 0.0) || !(region.h > 0.0)
        || !std::isfinite(region.x + region.y + right_edge + bottom_edge)) {
        return {};
    }

    const auto wide = static_cast<double>(_width);
    const auto tall = static_cast<double>(_height);
    const int left = static_cast<int>(std::floor(std::clamp(region.x, 0.0, wide)));
    const int top = static_cast<int>(std::floor(std::clamp(region.y, 0.0, tall)));
    const int right = static_cast<int>(std::ceil(std::clamp(right_edge, 0.0, wide)));
    const int bottom = static_cast<int>(std::ceil(std::clamp(bottom_edge, 0.0, tall)));

    return right <= left || bottom <= top ? BLRectI{} : BLRectI{left, top, right - left, bottom - top};
}

void Damage::add(const BLRect &region) {
    const BLRectI inside = clamp_to(region);

    if (inside.w <= 0 || inside.h <= 0) {
        return;
    }

    const size_t count = _regions.size();
    size_t kept = 0;
    size_t partner = count;
    int64_t least = std::numeric_limits<int64_t>::max();

    for (size_t at = 0; at < count; ++at) {
        const BLRectI held = _regions[at];

        // A pointer can report a hundred moves between two frames, and each of them
        // asks for the same rectangle. Without this the widget under it is painted
        // once per report rather than once per frame.
        if (covers(held, inside)) {
            if (kept != at) {
                const auto from = _regions.begin() + static_cast<ptrdiff_t>(at);

                _regions.erase(std::copy(from, _regions.end(), _regions.begin() + static_cast<ptrdiff_t>(kept)),
                               _regions.end());
            }

            return;
        }

        if (covers(inside, held)) {
            continue;
        }

        if (least != 0) {
            const int64_t cost = excess(inside, held);

            if (cost < least) {
                least = cost;
                partner = kept;
            }
        }

        _regions[kept] = held;
        ++kept;
    }

    _regions.resize(kept);

    if (partner == count || (least > WALK && kept < CROWDED)) {
        _regions.push_back(inside);

        return;
    }

    BLRectI joined = enclose(inside, _regions[partner]);

    _regions.erase(_regions.begin() + static_cast<ptrdiff_t>(partner));

    // A join can bring the result within reach of another rectangle, so the pass
    // repeats while it still finds one.
    for (bool merged = true; merged;) {
        size_t left = 0;

        merged = false;

        for (const BLRectI held : _regions) {
            if (covers(joined, held)) {
                continue;
            }

            if (excess(joined, held) <= WALK) {
                joined = enclose(joined, held);
                merged = true;

                continue;
            }

            _regions[left] = held;
            ++left;
        }

        _regions.resize(left);
    }

    _regions.push_back(joined);
}

void Damage::all() {
    _regions.clear();

    if (_width > 0 && _height > 0) {
        _regions.emplace_back(0, 0, _width, _height);
    }
}

void Damage::shift(const BLRectI &region, const int dy) {
    if (dy == 0 || whole()) {
        return;
    }

    // add() rewrites the set, so the rectangles to move are read from a copy.
    _moving.assign(_regions.begin(), _regions.end());

    for (const BLRectI &held : _moving) {
        if (held.y >= region.y + region.h || held.y + held.h <= region.y) {
            continue;
        }

        const int x0 = std::max(held.x, region.x);
        const int x1 = std::min(held.x + held.w, region.x + region.w);
        const int y0 = std::max(region.y, std::max(held.y, region.y) + dy);
        const int y1 = std::min(region.y + region.h, std::min(held.y + held.h, region.y + region.h) + dy);

        if (x1 > x0 && y1 > y0) {
            add(BLRect{static_cast<double>(x0),
                       static_cast<double>(y0),
                       static_cast<double>(x1 - x0),
                       static_cast<double>(y1 - y0)});
        }
    }
}

bool Damage::whole() const {
    return std::ranges::any_of(_regions, [&](const BLRectI &region) {
        return region.x <= 0 && region.y <= 0 && region.x + region.w >= _width && region.y + region.h >= _height;
    });
}
}
