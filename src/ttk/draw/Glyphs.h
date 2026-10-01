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

#include <blend2d/blend2d.h>

//! Built-in icon set drawn from vector paths.
namespace ttk::Glyphs {

    //! Icon of the built-in set.
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
        //! Number of glyphs. Not a glyph.
        Count,
    };

    //! Side of the square a glyph covers at weight 1, in pixels. A glyph is centred in it.
    constexpr float element = 12.0F;

    //! Draws `glyph` in `tone` inside a square \ref span() of `weight` across, with its top-left corner at `origin`.
    //!
    //! `turn` rotates the glyph clockwise about the centre of the square, in degrees. The state of `context` is
    //! left as it was. When `turn` is zero and `context` only translates by whole pixels, a glyph drawn twice at
    //! the same `weight` and sub-pixel offset is drawn from a cached mask.
    void draw(BLContext &context, Glyph glyph, BLPoint origin, float weight, BLRgba32 tone,
              float turn = 0.0F);

    //! Returns the side of the square \ref draw() covers at `weight`, which is \ref element times `weight`.
    float span(float weight);

    //! Draws every glyph except \ref Glyph::Empty onto one sheet and writes it to the image file at `path`.
    //!
    //! The glyphs are drawn at weight 2.6 in 64 pixel cells, eight to a row, light on a dark background. The file
    //! format follows the extension of `path`. Returns false when the sheet cannot be created or written.
    bool sheet(const char *path);

}


#endif //TTK_DRAW_GLYPHS_H
