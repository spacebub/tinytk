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

#include "ttk/draw/Damage.h"

namespace ttk {
    namespace {

        BLRectI enclose(const std::vector<BLRectI> &regions) {
            int left = regions.front().x;
            int top = regions.front().y;
            int right = left + regions.front().w;
            int bottom = top + regions.front().h;

            for (const BLRectI &region : regions) {
                left = std::min(left, region.x);
                top = std::min(top, region.y);
                right = std::max(right, region.x + region.w);
                bottom = std::max(bottom, region.y + region.h);
            }

            return {left, top, right - left, bottom - top};
        }

    }

    void Damage::resize(const int width, const int height) {
        _width = width;
        _height = height;
        _regions.clear();
    }

    BLRectI Damage::clamp_to(const BLRect &region) const {
        const int left = std::max(0, static_cast<int>(std::floor(region.x)));
        const int top = std::max(0, static_cast<int>(std::floor(region.y)));
        const int right = std::min(_width, static_cast<int>(std::ceil(region.x + region.w)));
        const int bottom = std::min(_height, static_cast<int>(std::ceil(region.y + region.h)));

        return right <= left || bottom <= top ? BLRectI{} : BLRectI{left, top, right - left,
                                                                    bottom - top};
    }

    void Damage::add(const BLRect &region) {
        const BLRectI inside = clamp_to(region);

        if (inside.w <= 0 || inside.h <= 0) {
            return;
        }

        // A pointer can report a hundred moves between two frames, and each of them
        // asks for the same rectangle. Without this the widget under it is painted
        // once per report rather than once per frame.
        for (const BLRectI &held : _regions) {
            if (inside.x >= held.x && inside.y >= held.y && inside.x + inside.w <= held.x + held.w
                && inside.y + inside.h <= held.y + held.h) {
                return;
            }
        }

        _regions.push_back(inside);

        if (_regions.size() > CROWDED) {
            _regions = {enclose(_regions)};
        }
    }

    void Damage::all() {
        _regions = {{0, 0, _width, _height}};
    }

    bool Damage::whole() const {
        return std::ranges::any_of(_regions, [&](const BLRectI &region) {
            return region.x <= 0 && region.y <= 0 && region.x + region.w >= _width
                && region.y + region.h >= _height;
        });
    }
}
