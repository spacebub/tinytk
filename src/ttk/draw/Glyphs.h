// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_DRAW_GLYPHS_H
#define TTK_DRAW_GLYPHS_H


#include <cstdint>
#include <string>

#include <blend2d/blend2d.h>

//! Icon set drawn from vector paths: the built-in glyphs, and any an application adds.
namespace ttk::Glyphs {

//! Icon of the built-in set, or one returned by \ref add().
enum class Glyph : std::uint8_t {
    //! Draws nothing.
    Empty,
    //! Diagonal cross.
    Close,
    //! Plus sign.
    Plus,
    //! Magnifying glass.
    Search,
    //! Arrow pointing down onto a line.
    Download,
    //! Floppy disk.
    Save,
    //! Arrow leaving an open box to the right.
    Extract,
    //! Waste bin.
    Trash,
    //! Pencil.
    Edit,
    //! Arrow pointing up.
    Up,
    //! Arrow pointing down.
    Down,
    //! Circle with its left half filled, for following the system theme.
    System,
    //! Sun, for the light theme.
    Light,
    //! Crescent moon, for the dark theme.
    Dark,
    //! Check mark.
    Check,
    //! Circular arrow.
    Refresh,
    //! Gear of settings.
    Cog,
    //! Terminal window with a prompt.
    Terminal,
    //! Two overlapping squares.
    Copy,
    //! Filled folder.
    Folder,
    //! Filled triangle pointing right.
    Play,
    //! Page with a folded corner.
    File,
    //! Filled folder for file lists, drawn in a slightly larger viewbox than \ref Folder.
    FileFolder,
    //! Globe of a circle crossed by its equator and a meridian.
    Globe,
    //! Three dots in a row.
    Dots,
    //! Drag handle of two columns of three dots.
    Grip,
    //! Horizontal bar of a window's minimize button.
    Minimize,
    //! Horizontal bar, the same shape as \ref Minimize.
    Minus,
    //! Square of a window's maximize button.
    Maximize,
    //! Two overlapping squares of a window's restore button.
    Restore,
    //! Number of built-in glyphs. Not a glyph.
    Count,
};

//! Side of the square a glyph is laid out in at weight 1, in pixels. A glyph is centred on it.
constexpr float element = 12.0F;

//! Shape of a glyph an application adds with \ref add(), in the terms the built-in set is drawn in.
//!
//! Each path is SVG path data in a square viewbox of its own, drawn at one pixel per unit times the weight with
//! the viewbox centred on the glyph's square. A viewbox of \ref element matches the square. Most built-in glyphs
//! use one of 14, which reaches a unit past each side, and keep their ink near the middle of it.
struct Custom {
    //! Path stroked at \ref pen with round caps and joins. Empty for none.
    std::string outline{};
    //! Path filled under the outline. Empty for none.
    std::string solid{};
    //! Side of the viewbox `outline` is laid out in.
    float box = 14.0F;
    //! Side of the viewbox `solid` is laid out in.
    float solidBox = 14.0F;
    //! Stroke width of `outline`, in viewbox units.
    float pen = 1.5F;
};

//! Adds `shape` to the set and returns the glyph that draws it, which anything taking a \ref Glyph accepts.
//!
//! Added glyphs are numbered from one past \ref Glyph::Count up to 255, so the set takes 255 minus that many.
//! Returns \ref Glyph::Empty when both paths are empty, a path given does not parse, its viewbox or the pen is
//! not positive, or the set is full. Call it from the thread that draws.
Glyph add(Custom shape);

//! Draws `glyph` in `tone` centred on a square \ref span() of `weight` across, with its top-left corner at `origin`.
//!
//! Unturned, glyphs drawn in a larger viewbox, such as \ref Glyph::Light and \ref Glyph::File, ink up to 1.1 times
//! `weight` pixels past a side of the square, and their antialiasing a pixel beyond that. Pad the square by as
//! much before testing it against a clip.
//!
//! `turn` rotates the glyph clockwise about the centre of the square, in degrees. The state of `context` is
//! left as it was. When `turn` is zero and `context` only translates by whole pixels, a glyph drawn twice at
//! the same `weight` and sub-pixel offset is drawn from a cached mask.
void draw(BLContext &context, Glyph glyph, BLPoint origin, float weight, BLRgba32 tone, float turn = 0.0F);

//! Returns the side of the square \ref draw() lays a glyph out in at `weight`, which is \ref element times `weight`.
float span(float weight);

//! Draws every built-in glyph except \ref Glyph::Empty onto one sheet and writes it to the image file at `path`.
//!
//! The glyphs are drawn at weight 2.6 in 64 pixel cells, eight to a row, light on a dark background. The file
//! format follows the extension of `path`. Returns false when the sheet cannot be created or written.
bool sheet(const char *path);

}


#endif //TTK_DRAW_GLYPHS_H
