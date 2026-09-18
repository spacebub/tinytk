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

namespace ttk {
    namespace Glyphs {

        enum class Glyph : std::uint8_t {
            Empty,
            Close,
            Plus,
            Search,
            Download,
            Save,
            Extract,
            Trash,
            Edit,
            Up,
            Down,
            System,
            Light,
            Dark,
            Check,
            Refresh,
            Cog,
            Terminal,
            Copy,
            Folder,
            Play,
            File,
            FileFolder,
            Dots,
            Grip,
            Minimize,
            Minus,
            Maximize,
            Restore,
            Count,
        };

        // The whole element is a `12 * weight` square. The drawing inside it is centred
        // and sized from its own viewbox.
        constexpr float element = 12.0F;

        // Draws `glyph` with its top-left at `origin`. `turn` is in degrees about the
        // square's centre.
        void draw(BLContext &context, Glyph glyph, BLPoint origin, float weight, BLRgba32 tone,
                  float turn = 0.0F);

        // The side of the square `draw` covers.
        float span(float weight);

        // Every glyph in the table, drawn onto one sheet and written to `path`. This is
        // the whole icon set of the application, rendered from the same strings it uses.
        bool sheet(const char *path);

    }
}


#endif //TTK_DRAW_GLYPHS_H
