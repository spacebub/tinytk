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

    //! Base class of everything drawn in a window.
    //!
    //! Widgets form a tree owned from the top by a \ref Root. Every widget has a box in window coordinates, set
    //! by its parent through \ref place(), and both hit testing and damage are tested against such boxes. Layout
    //! runs top down: a parent asks each child for \ref wanted_width() and \ref wanted_height(), then places it.
    //!
    //! A plain `Widget` is a container that stacks all of its children on its own box and lets the pointer
    //! through to them. Controls and layouts subclass it and override the virtual methods they need.
    class Widget {
    public:
        //! Owning pointer to a widget.
        using Ptr = std::unique_ptr<Widget>;

        Widget();
        virtual ~Widget() = default;

        Widget(const Widget &) = delete;
        Widget &operator=(const Widget &) = delete;
        Widget(Widget &&) = delete;
        Widget &operator=(Widget &&) = delete;

        //! \name Tree
        //! \{

        //! Appends `child` as the last child, attaches it to this widget's root and returns it.
        Widget *add(Ptr child);

        //! Appends `child` as the last child and returns it as its own type.
        template <typename Kind>
        Kind *append(std::unique_ptr<Kind> child) {
            Kind *raw = child.get();

            add(std::move(child));

            return raw;
        }

        //! Destroys all children.
        void clear();

        //! Destroys `child` and everything under it. Does nothing when `child` is not a direct child.
        void erase(const Widget *child);

        //! Returns the children in paint order, the last one painted on top.
        [[nodiscard]] const std::vector<Ptr> &children() const { return _children; }

        //! Returns the parent, or null for a widget that was not added to one.
        [[nodiscard]] Widget *parent() const { return _parent; }

        //! Returns the root the widget is attached to, or null while it is not in a window's tree.
        [[nodiscard]] Root *root() const { return _root; }

        //! \}

        //! \name Geometry
        //! \{

        //! Returns the box last given to \ref place(), in window coordinates.
        [[nodiscard]] const BLRect &box() const { return _box; }

        //! Sets the box to `box`, then calls \ref moved() and \ref arrange().
        void place(const BLRect &box, Typeface &type);

        //! Returns the width the widget would like when left to size itself.
        //!
        //! Returns \ref fixedWidth when it is set. The default implementation returns the widest of the visible
        //! children.
        virtual double natural_width(Typeface &type);

        //! Returns the height the widget needs when given `width` across.
        //!
        //! Returns \ref fixedHeight when it is set. The default implementation returns the tallest of the visible
        //! children at `width`.
        virtual double natural_height(Typeface &type, double width);

        //! Places the children inside the current box.
        //!
        //! The default implementation gives every child the widget's own box.
        virtual void arrange(Typeface &type);

        //! \}

        //! \name State
        //! \{

        //! Tests whether the widget is shown. A hidden widget is neither painted, hit tested nor laid out.
        [[nodiscard]] bool visible() const { return _visible; }

        //! Shows or hides the widget and asks the root for a new layout.
        void set_visible(bool value);

        //! Tests whether the widget accepts input. A disabled widget is skipped by \ref at().
        [[nodiscard]] bool enabled() const { return _enabled; }

        //! Enables or disables the widget and repaints it.
        void set_enabled(bool value);

        //! Tests whether the pointer is over this widget itself, as reported by \ref enter() and \ref leave().
        [[nodiscard]] bool hovered() const { return _hovered; }

        //! Tests whether the widget under the pointer is this widget or one of its descendants.
        [[nodiscard]] bool holds_pointer() const;

        //! Tests whether a press on this widget is being held.
        [[nodiscard]] bool pressed() const { return _pressed; }

        //! Tests whether this widget has keyboard focus.
        [[nodiscard]] bool focused() const;

        //! Share of a row's spare width given to this widget. Zero takes none.
        double stretch = 0.0;

        //! Width that overrides \ref natural_width() when non-negative. Negative lets the widget size itself.
        double fixedWidth = -1.0;

        //! Height that overrides \ref natural_height() when non-negative. Negative lets the widget size itself.
        double fixedHeight = -1.0;

        //! Smallest width \ref wanted_width() reports when \ref fixedWidth is not set.
        //!
        //! A row too narrow for the floors of its children overflows and is clipped rather than shrinking them.
        double minWidth = 0.0;

        //! Smallest height \ref wanted_height() reports when \ref fixedHeight is not set.
        double minHeight = 0.0;

        //! Returns \ref fixedWidth when set, otherwise \ref natural_width() raised to \ref minWidth.
        double wanted_width(Typeface &type);

        //! Returns \ref fixedHeight when set, otherwise \ref natural_height() at `width` raised to \ref minHeight.
        double wanted_height(Typeface &type, double width);

        //! Tooltip text shown while the pointer rests on the widget. Empty for none.
        //!
        //! A widget with a hint is hit by \ref at() even when it does not otherwise take the pointer.
        std::string hint;

        //! Pointer cursor shown over the widget.
        Cursor cursor = Cursor::Default;

        //! Returns the cursor to show at `[x, y]`. The default implementation returns \ref cursor.
        //!
        //! Override it for a widget whose parts want different cursors, such as a row with a drag grip.
        [[nodiscard]] virtual Cursor cursor_at(double x, double y) const;

        //! \}

        //! \name Painting
        //! \{

        //! Paints the widget. The default implementation paints each visible child that intersects the area being
        //! repainted, clipped to the region returned by its \ref clips().
        virtual void paint(const Painter &painter);

        //! Returns the area the widget paints into, which is \ref box() unless the widget draws outside it, for
        //! example a shadow or a hover growth. Damage and paint culling both use this area.
        [[nodiscard]] virtual BLRect drawn() const { return _box; }

        //! Returns the area to repaint when the pointer enters or leaves the widget. Defaults to \ref drawn().
        //!
        //! A container whose box is much larger than the part that reacts to the pointer returns only that part.
        [[nodiscard]] virtual BLRect lit_box() const { return drawn(); }

        //! Stores the region children are clipped to in `region` and returns true, or returns false when children
        //! are not clipped. The default implementation returns false. A scroller returns its viewport.
        virtual bool clips(BLRect &region) const;

        //! Marks \ref drawn() for repainting.
        void invalidate() const;

        //! Marks `region` for repainting, clipped by the \ref clips() region of every ancestor. Does nothing while
        //! the widget is not attached to a root.
        void invalidate(const BLRect &region) const;

        //! \}

        //! \name Events
        //! \{

        //! Handles a pointer press inside the widget and returns true when it was taken.
        //!
        //! A widget that takes a press receives \ref drag() and \ref release() for it. The default implementation
        //! returns false.
        virtual bool press(const Pointer &at);

        //! Handles pointer motion while a press taken by \ref press() is held.
        virtual void drag(const Pointer &at);

        //! Handles the end of a press taken by \ref press().
        virtual void release(const Pointer &at);

        //! Called when the pointer moves onto the widget. The default implementation sets \ref hovered() and
        //! repaints \ref lit_box().
        virtual void enter();

        //! Called when the pointer moves off the widget. The default implementation clears \ref hovered() and
        //! \ref pressed() and repaints \ref lit_box().
        virtual void leave();

        //! Handles pointer motion over the widget while no press is held.
        virtual void hover(const Pointer &at);

        //! Called with `inside` true when the pointer moves onto this widget or any descendant, and with false
        //! when it leaves the last of them.
        virtual void within(bool inside);

        //! Handles a wheel scroll of `steps` notches at `at` and returns true when it was consumed. Unconsumed
        //! scrolls are offered to the parent.
        virtual bool wheel(double steps, const Pointer &at);

        //! Handles a key press while the widget has focus and returns true when it was consumed.
        virtual bool key(const Key &pressed);

        //! Handles text input, in UTF-8, while the widget has focus.
        virtual void wrote(const std::string &text);

        //! Tests whether the widget can receive keyboard focus. The default implementation returns false.
        [[nodiscard]] virtual bool takes_focus() const { return false; }

        //! Called when the widget receives keyboard focus. The default implementation repaints it.
        virtual void gained_focus();

        //! Called when the widget loses keyboard focus. The default implementation repaints it.
        virtual void lost_focus();

        //! Returns the deepest visible and enabled widget at `[x, y]` that takes the pointer or has a \ref hint,
        //! or null when there is none. Later children are tested first, as they are painted on top.
        virtual Widget *at(double x, double y);

        //! Tests whether `[x, y]` lies inside \ref box().
        [[nodiscard]] bool holds(double x, double y) const;

        //! \}

        //! \name Animation
        //! \{

        //! Steps animations to the frame time `now` and returns whether any are still running.
        //!
        //! Called every frame while the widget is live, see \ref wake(). Returning false removes it from the frame
        //! loop, and the window sleeps once no widget is live. The default implementation returns false.
        virtual bool advance(double now);

        //! Makes the widget live, so \ref advance() is called from the next frame on.
        void wake() const;

        //! Schedules the widget to become live again at the frame time `when` and returns false.
        //!
        //! Meant to be returned from \ref advance() by a widget that changes at intervals, such as a blinking
        //! caret, so the window sleeps until then instead of running frames at the frame cap.
        [[nodiscard]] bool sleep_until(double when) const;

        //! Returns the current frame time in seconds, or 0 while the widget is not attached to a root. Use it to
        //! start an \ref Anim::Tween from an event handler.
        [[nodiscard]] double now() const;

        //! \}

    protected:
        //! Called by \ref place() after the box is set and before \ref arrange().
        virtual void moved() {}

        //! Called when the \ref Theme has changed since the widget was last attached or restyled. Override it to
        //! rebuild anything derived from theme colours.
        virtual void restyle() {}

        //! Whether \ref at() returns this widget for a point inside it. A control that answers the pointer sets
        //! it in its constructor. A plain container leaves it false so the pointer reaches what is under it.
        bool _takesPointer = false;

        //! Attaches this widget and all of its descendants to `root`.
        void attach(Root *root);

        //! Box in window coordinates, set by \ref place().
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
