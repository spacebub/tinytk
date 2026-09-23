// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_TOOLKIT_WIDGET_H
#define TTK_TOOLKIT_WIDGET_H


#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <blend2d/blend2d.h>

#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Event.h"
#include "ttk/toolkit/Painter.h"

namespace ttk {
    class Root;

    // One thing on screen. Boxes are in window coordinates, so damage and hit testing
    // are both a rectangle test. Layout runs top down, a parent handing each child the
    // box it asked for.
    class Widget {
    public:
        using Ptr = std::unique_ptr<Widget>;

        Widget();
        virtual ~Widget() = default;

        Widget(const Widget &) = delete;
        Widget &operator=(const Widget &) = delete;
        Widget(Widget &&) = delete;
        Widget &operator=(Widget &&) = delete;

        // --- tree ---

        Widget *add(Ptr child);

        template <typename Kind>
        Kind *append(std::unique_ptr<Kind> child) {
            Kind *raw = child.get();

            add(std::move(child));

            return raw;
        }

        void clear();

        // Drops `child` and everything under it.
        void erase(const Widget *child);

        [[nodiscard]] const std::vector<Ptr> &children() const { return _children; }

        [[nodiscard]] Widget *parent() const { return _parent; }

        [[nodiscard]] Root *root() const { return _root; }

        // --- geometry ---

        [[nodiscard]] const BLRect &box() const { return _box; }

        void place(const BLRect &box, Typeface &type);

        // What it would like across, and how tall it is once that is settled.
        virtual double natural_width(Typeface &type);
        virtual double natural_height(Typeface &type, double width);

        // Places children inside the box already set.
        virtual void arrange(Typeface &type);

        // --- state ---

        [[nodiscard]] bool visible() const { return _visible; }
        void set_visible(bool value);

        [[nodiscard]] bool enabled() const { return _enabled; }
        void set_enabled(bool value);

        [[nodiscard]] bool hovered() const { return _hovered; }

        // True while the pointer is on it or on anything in it.
        [[nodiscard]] bool holds_pointer() const;
        [[nodiscard]] bool pressed() const { return _pressed; }
        [[nodiscard]] bool focused() const;

        // How a row shares its spare width. Zero never takes any.
        double stretch = 0.0;

        // Honoured by the layouts in Box.h. Negative is "ask the widget".
        double fixedWidth = -1.0;
        double fixedHeight = -1.0;

        // A hard floor: a row short of room overflows
        // and is clipped rather than squeezing what is in it past legibility.
        double minWidth = 0.0;
        double minHeight = 0.0;

        // What the widget wants across and down, with the floors applied.
        double wanted_width(Typeface &type);
        double wanted_height(Typeface &type, double width);

        // Shown while the pointer rests on it.
        std::string hint;

        // What the pointer turns into over it.
        Cursor cursor = Cursor::Default;

        // For a widget whose parts want different ones: the grip of a row, say.
        [[nodiscard]] virtual Cursor cursor_at(double x, double y) const;

        // --- painting ---

        virtual void paint(const Painter &painter);

        // Everything the widget puts on screen, which is its box unless it casts a
        // shadow or grows on hover. Damage and the cull test both go by this.
        [[nodiscard]] virtual BLRect drawn() const { return _box; }

        // What an arriving or leaving pointer changes. A container whose box is far
        // larger than what answers the pointer says only that part.
        [[nodiscard]] virtual BLRect lit_box() const { return drawn(); }

        // What a child may draw into. Empty for none. A scroller answers its viewport.
        virtual bool clips(BLRect &region) const;

        void invalidate() const;
        void invalidate(const BLRect &region) const;

        // --- events ---

        // True when the press was taken. The widget then receives drag and release.
        virtual bool press(const Pointer &at);
        virtual void drag(const Pointer &at);
        virtual void release(const Pointer &at);

        virtual void enter();
        virtual void leave();
        virtual void hover(const Pointer &at);

        // The pointer has come onto something in it, or left the last such thing.
        virtual void within(bool inside);

        virtual bool wheel(double steps, const Pointer &at);

        virtual bool key(const Key &pressed);
        virtual void wrote(const std::string &text);

        [[nodiscard]] virtual bool takes_focus() const { return false; }
        virtual void gained_focus();
        virtual void lost_focus();

        // The deepest widget under the point that wants the pointer.
        virtual Widget *at(double x, double y);

        [[nodiscard]] bool holds(double x, double y) const;

        // --- animation ---

        // Answers false once nothing is left in flight, which drops it from the
        // frame loop and lets the window go back to sleep.
        virtual bool advance(double now);

        void wake() const;

        // Leaves the live list until `when`. Answer it from advance(): the loop then
        // sleeps rather than turning at the frame cap for something that blinks.
        [[nodiscard]] bool sleep_until(double when) const;

        // The frame clock, for starting a tween from an event handler.
        [[nodiscard]] double now() const;

    protected:
        // Called after place() has set the box, before arrange().
        virtual void moved() {}

        // Called when the palette on screen has changed since the widget last saw it.
        virtual void restyle() {}

        // A leaf that answers the pointer says so once, in its constructor. A plain
        // container lets what is under it through.
        bool _takesPointer = false;

        void attach(Root *root);

        BLRect _box{};

    private:
        std::vector<Ptr> _children;
        Widget *_parent = nullptr;
        Root *_root = nullptr;
        std::uint32_t _revision;

        bool _visible = true;
        bool _enabled = true;

        bool _hovered = false;
        bool _pressed = false;

        friend class Root;
    };
}


#endif //TTK_TOOLKIT_WIDGET_H
