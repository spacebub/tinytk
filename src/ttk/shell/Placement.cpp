// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <cstdint>

#include "ttk/shell/Placement.h"

namespace ttk::Placement {
namespace {
bool reaches(const SDL_Rect &window, const SDL_Rect &display) {
    const int width = std::min(GRAB_WIDTH, window.w);
    const int height = std::min(GRAB_HEIGHT, window.h);

    const SDL_Rect strip{window.x, window.y, window.w, height};

    SDL_Rect shared{};

    return SDL_GetRectIntersection(&strip, &display, &shared) && shared.w >= width && shared.h >= height;
}

int gap(const int from, const int length, const int low, const int high) {
    return std::max({0, low - (from + length), from - high});
}

std::int64_t distance(const SDL_Rect &window, const SDL_Rect &display) {
    const std::int64_t across = gap(window.x, window.w, display.x, display.x + display.w);
    const std::int64_t down = gap(window.y, window.h, display.y, display.y + display.h);

    return (across * across) + (down * down);
}

int inside(const int from, const int length, const int low, const int room) {
    return std::clamp(from, low, std::max(low, low + room - length));
}
}

SDL_Point fit(const SDL_Rect &window, const std::span<const SDL_Rect> displays) {
    if (displays.empty()) {
        return SDL_Point{window.x, window.y};
    }

    if (std::ranges::any_of(displays, [&](const SDL_Rect &display) { return reaches(window, display); })) {
        return SDL_Point{window.x, window.y};
    }

    const auto closest = std::ranges::min_element(displays, [&](const SDL_Rect &a, const SDL_Rect &b) {
        return distance(window, a) < distance(window, b);
    });

    return SDL_Point{
            inside(window.x, window.w, closest->x, closest->w),
            inside(window.y, window.h, closest->y, closest->h),
    };
}
}
