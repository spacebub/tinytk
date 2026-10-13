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

// Width and height of where the two meet, at or below zero where they stand apart.
// One holds the other when the meeting is as large as it is.
BLSizeI meeting(const BLRectI &one, const BLRectI &two) {
    return {std::min(one.x + one.w, two.x + two.w) - std::max(one.x, two.x),
            std::min(one.y + one.h, two.y + two.h) - std::max(one.y, two.y)};
}

// Clean pixels the bounding rectangle of the two would repaint.
int64_t excess(const BLRectI &one, const BLRectI &two, const BLSizeI met) {
    const BLRectI bound = enclose(one, two);
    const int64_t shared = met.w > 0 && met.h > 0 ? int64_t{met.w} * met.h : 0;

    return (int64_t{bound.w} * bound.h) - (int64_t{one.w} * one.h) - (int64_t{two.w} * two.h) + shared;
}
}

void Damage::resize(const int width, const int height) {
    _width = width;
    _height = height;
    _regions.clear();
}

BLRectI Damage::clamp_to(const BLRect &region) const {
    const auto wide = static_cast<double>(_width);
    const auto tall = static_cast<double>(_height);
    const double right_edge = region.x + region.w;
    const double bottom_edge = region.y + region.h;

    // Written so that a NaN anywhere fails a comparison and is turned away with the rest, and an infinite
    // far edge, which no comparison catches, fails the last test.
    if (!(region.w > 0.0) || !(region.h > 0.0) || !(region.x < wide) || !(region.y < tall) || !(right_edge > 0.0)
        || !(bottom_edge > 0.0) || !std::isfinite(right_edge + bottom_edge)) {
        return {};
    }

    const int left = static_cast<int>(std::floor(std::max(region.x, 0.0)));
    const int top = static_cast<int>(std::floor(std::max(region.y, 0.0)));
    const int right = static_cast<int>(std::ceil(std::min(right_edge, wide)));
    const int bottom = static_cast<int>(std::ceil(std::min(bottom_edge, tall)));

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
        if (inside.x >= held.x && inside.y >= held.y && inside.x + inside.w <= held.x + held.w
            && inside.y + inside.h <= held.y + held.h) {
            if (kept != at) {
                const auto from = _regions.begin() + static_cast<ptrdiff_t>(at);

                _regions.erase(std::copy(from, _regions.end(), _regions.begin() + static_cast<ptrdiff_t>(kept)),
                               _regions.end());
            }

            return;
        }

        const BLSizeI met = meeting(inside, held);

        if (met.w >= held.w && met.h >= held.h) {
            continue;
        }

        if (least != 0) {
            const int64_t cost = excess(inside, held, met);

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

    // What the joined rectangle now covers or nearly covers goes in with it. A chain
    // past that waits for the next region to arrive in the area.
    size_t left = 0;

    for (const BLRectI held : _regions) {
        const BLSizeI met = meeting(joined, held);

        if (met.w >= held.w && met.h >= held.h) {
            continue;
        }

        if (excess(joined, held, met) <= WALK) {
            joined = enclose(joined, held);

            continue;
        }

        _regions[left] = held;
        ++left;
    }

    _regions.resize(left);

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
