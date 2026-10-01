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
    //! Pointer button.
    enum class Click : std::uint8_t {
        //! Primary button.
        Left,
        //! Middle button or wheel press.
        Middle,
        //! Secondary button.
        Right,
        //! Side button for going back. \ref Shell calls its `back` callback for it and never passes it to widgets.
        Back,
        //! Side button for going forward. \ref Shell calls its `forward` callback for it and never passes it to
        //! widgets.
        Forward,
    };

    //! Pointer event, in window coordinates.
    struct Pointer {
        //! Horizontal position of the pointer.
        double x = 0.0;
        //! Vertical position of the pointer.
        double y = 0.0;
        //! Button pressed or released. Left for motion and wheel events, which carry no button.
        Click button = Click::Left;

        //! Whether Ctrl was held. Set for presses only.
        bool ctrl = false;
        //! Whether Shift was held. Set for presses only.
        bool shift = false;
    };

    //! Key codes compared against \ref Key::code.
    //!
    //! Values are SDL keycodes. Only the keys the toolkit acts on are named here. Any other key arrives with its
    //! SDL keycode, which for a printable key is its unshifted character, such as `'a'`.
    namespace Code {

        //! Escape key.
        inline constexpr int Escape = 27;
        //! Space bar.
        inline constexpr int Space = 32;
        //! Return (Enter) key.
        inline constexpr int Return = 13;
        //! Tab key.
        inline constexpr int Tab = 9;
        //! Backspace key.
        inline constexpr int Backspace = 8;
        //! Delete key.
        inline constexpr int Delete = 0x4000004C;
        //! Left arrow key.
        inline constexpr int Left = 0x40000050;
        //! Right arrow key.
        inline constexpr int Right = 0x4000004F;
        //! Up arrow key.
        inline constexpr int Up = 0x40000052;
        //! Down arrow key.
        inline constexpr int Down = 0x40000051;
        //! Home key.
        inline constexpr int Home = 0x4000004A;
        //! End key.
        inline constexpr int End = 0x4000004D;
        //! Page Up key.
        inline constexpr int PageUp = 0x4000004B;
        //! Page Down key.
        inline constexpr int PageDown = 0x4000004E;
        //! F1 function key.
        inline constexpr int F1 = 0x4000003A;

    }

    //! Pointer cursor shape shown over a widget.
    enum class Cursor : std::uint8_t {
        //! System default arrow.
        Default,
        //! Pointing hand, for something that can be clicked.
        Pointer,
        //! Text I-beam, for editable or selectable text.
        Text,
        //! Vertical resize arrow.
        Resize,
        //! Shape for something that can be dragged. \ref Shell shows the system move cursor for it.
        Grab,
        //! Shape for something being dragged. \ref Shell shows the system move cursor for it.
        Grabbing,
    };

    //! Key press delivered to the focused widget.
    struct Key {
        //! SDL keycode of the key, see \ref Code.
        int code = 0;

        //! Whether Ctrl was held.
        bool ctrl = false;
        //! Whether Shift was held.
        bool shift = false;
        //! Whether Alt was held.
        bool alt = false;
    };
}


#endif //TTK_TOOLKIT_EVENT_H
