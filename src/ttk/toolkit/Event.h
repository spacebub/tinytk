// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_TOOLKIT_EVENT_H
#define TTK_TOOLKIT_EVENT_H


#include <cstdint>

namespace ttk {
    enum class Click : std::uint8_t {
        Left,
        Middle,
        Right,
        Back,
        Forward,
    };

    struct Pointer {
        double x = 0.0;
        double y = 0.0;
        Click button = Click::Left;

        bool ctrl = false;
        bool shift = false;
    };

    // SDL keycodes are passed through. Only the ones the interface acts on are named.
    namespace Code {

        inline constexpr int Escape = 27;
        inline constexpr int Space = 32;
        inline constexpr int Return = 13;
        inline constexpr int Tab = 9;
        inline constexpr int Backspace = 8;
        inline constexpr int Delete = 0x4000004C;
        inline constexpr int Left = 0x40000050;
        inline constexpr int Right = 0x4000004F;
        inline constexpr int Up = 0x40000052;
        inline constexpr int Down = 0x40000051;
        inline constexpr int Home = 0x4000004A;
        inline constexpr int End = 0x4000004D;
        inline constexpr int PageUp = 0x4000004B;
        inline constexpr int PageDown = 0x4000004E;
        inline constexpr int F1 = 0x4000003A;

    }

    // What the pointer turns into over a widget.
    enum class Cursor : std::uint8_t {
        Default,
        Pointer,
        Text,
        Resize,
        Grab,
        Grabbing,
    };

    struct Key {
        int code = 0;

        bool ctrl = false;
        bool shift = false;
        bool alt = false;
    };
}


#endif //TTK_TOOLKIT_EVENT_H
