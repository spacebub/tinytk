// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_SHELL_PLACEMENT_H
#define TTK_SHELL_PLACEMENT_H


#include <span>

#include <SDL3/SDL_rect.h>

//! Where a window can be put on the connected displays.
namespace ttk::Placement {
//! Height of the strip along the top of a window that has to stay on a display, in logical pixels.
inline constexpr int GRAB_HEIGHT = 32;

//! Width of the strip along the top of a window that has to stay on a display, in logical pixels.
inline constexpr int GRAB_WIDTH = 96;

//! Returns the position to give `window` so that its title bar can be reached on one of `displays`.
//!
//! A `window` can be reached when one of the `displays` holds the whole height of the strip along its top edge,
//! \ref GRAB_HEIGHT, and at least \ref GRAB_WIDTH of its width, or all of it when the window is narrower. The
//! position of such a `window` is returned unchanged. Any other `window` is moved the least distance that puts
//! it inside the display nearest to it, or against the top left corner of that display when it is larger. A tie
//! goes to the earlier display, so list the primary display first. Without `displays` the position of `window`
//! is returned unchanged.
//!
//! The `displays` are usable areas, which leave out taskbars and panels, in the same coordinates as `window`.
[[nodiscard]] SDL_Point fit(const SDL_Rect &window, std::span<const SDL_Rect> displays);
}


#endif //TTK_SHELL_PLACEMENT_H
