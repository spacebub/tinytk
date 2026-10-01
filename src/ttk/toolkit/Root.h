// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_TOOLKIT_ROOT_H
#define TTK_TOOLKIT_ROOT_H


#include <cstdint>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

#include <blend2d/blend2d.h>

#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
    //! Top of a window's widget tree, which routes input to widgets and collects what needs repainting.
    //!
    //! A root holds the page, \ref content(), and \ref LAYERS layers painted over it in index order. It tracks the
    //! widget under the pointer, the one holding a press, the one with keyboard focus and the widgets that are
    //! animating, and it gathers the damaged rectangles and pixel moves of each frame for \ref compose().
    class Root {
    public:
        //! Creates an empty root whose widgets lay text out with `type`, which must outlive the root.
        explicit Root(Typeface &type) : _type(type) { _layers[POPUPS].floats(); }

        //! \name Window
        //! \{

        //! Returns the page, the widget under every layer that holds the window's content. It is given the whole
        //! window as its box.
        Widget *content() { return &_page; }

        //! Returns the layer at `index`, clamped to the last layer.
        //!
        //! Layers cover the window and let the pointer through to what is under them. Each child of a layer is given
        //! the whole window, except on \ref POPUPS, where a child keeps the box it was placed at.
        Widget *layer(size_t index);

        //! Number of layers over the page.
        static constexpr size_t LAYERS = 4;

        //! Index of the layer for dialogs, painted first over the page.
        static constexpr size_t DIALOGS = 0;
        //! Index of the layer for popups such as menus and dropdown lists, painted over dialogs.
        static constexpr size_t POPUPS = 1;
        //! Index of the layer for toasts, painted over popups.
        static constexpr size_t NOTICES = 2;
        //! Index of the layer for tooltips, painted over everything else.
        static constexpr size_t TIPS = 3;

        //! Sets the window size to `width` by `height` pixels.
        //!
        //! When the size changes, schedules a relayout and dismisses the open popup, see \ref dismiss().
        void resize(double width, double height);

        //! Returns the window width given to \ref resize().
        double width() const { return _width; }
        //! Returns the window height given to \ref resize().
        double height() const { return _height; }

        //! Schedules a layout of the whole tree, run by the next \ref settle().
        void relayout() { _relayout = true; }

        //! Runs a scheduled layout and returns true, or returns false when none is due.
        //!
        //! A change of the \ref Theme since the last layout also schedules one. The layout places the page and every
        //! layer on the whole window, damages all of it, and moves the hover off a widget that is no longer under
        //! the pointer.
        bool settle();

        //! \}

        //! \name Damage
        //! \{

        //! Marks `region` for repainting. An empty `region` is ignored.
        //!
        //! Once more than 32 regions are pending they are merged into their bounding rectangle, which every further
        //! region joins until the next \ref take().
        void damage(const BLRect &region);
        //! Replaces every pending region with the whole window.
        void damage_all();

        //! Returns the pending regions and clears them.
        //!
        //! Regions are merged where repainting their bounding rectangle costs less than repainting them apart.
        std::vector<BLRect> take();

        //! Tests whether any region is pending.
        bool dirty() const { return !_dirty.empty(); }

        //! Pixel move requested through \ref shift().
        struct Shift {
            //! Rectangle of pixels to move, rounded inward to whole pixels.
            BLRectI region;
            //! Vertical distance in pixels, positive downward.
            int dy;
        };

        //! Asks for the pixels of `region` to be moved by `dy` pixels vertically instead of being repainted, and
        //! returns whether the move was accepted.
        //!
        //! Refused when any ancestor of `who` clips its children, or when a visible child of a layer above the one
        //! holding `who` draws over `region`. The caller still damages what the move leaves uncovered, and repaints
        //! everything itself when refused.
        bool shift(const Widget *who, const BLRect &region, int dy);

        //! Returns the moves accepted by \ref shift() since the last call and clears them.
        std::vector<Shift> take_shifts();

        //! Paints the page and then every layer, clipped to `clip`.
        void paint(BLContext &context, const BLRectI &clip);

        //! \}

        //! \name Pointer
        //! \{

        //! Handles pointer motion to `[x, y]`.
        //!
        //! While a widget holds the press, it receives \ref Widget::drag(). Otherwise the hover moves to the widget
        //! under the pointer, calling \ref Widget::leave(), \ref Widget::enter(), \ref Widget::hover() and
        //! \ref Widget::within() as needed.
        void motion(double x, double y);
        //! Handles a button press.
        //!
        //! A press outside the \ref POPUPS layer while a popup is open dismisses it first, and goes no further when
        //! it lands on nothing or on the popup's owner, see \ref set_dismiss(). The press is then offered to the
        //! widget under the pointer and its ancestors until one takes it, which then holds the press. A press that
        //! nothing takes clears keyboard focus.
        void press(const Pointer &at);
        //! Handles a button release. The widget holding the press receives \ref Widget::release() and the hover
        //! moves to whatever is under `at`. Does nothing when no widget holds the press.
        void release(const Pointer &at);
        //! Handles a wheel scroll of `steps` notches at `[x, y]`, offered to the widget there and then its ancestors
        //! until one consumes it.
        void wheel(double steps, double x, double y);
        //! Handles the pointer leaving the window by clearing the hover. Ignored while a widget holds the press.
        void leave();

        //! Returns the widget under the pointer, or null when there is none.
        Widget *hovered() const { return _hovered; }
        //! Returns the widget holding the press, or null when no press is held.
        Widget *grabbed() const { return _grabbed; }

        //! Returns the horizontal pointer position of the last motion, press or release.
        double pointer_x() const { return _pointer.x; }
        //! Returns the vertical pointer position of the last motion, press or release.
        double pointer_y() const { return _pointer.y; }

        //! Returns the cursor to show, taken from \ref Widget::cursor_at() of the widget holding the press, else of
        //! the hovered widget, else \ref Cursor::Default.
        Cursor cursor() const;

        //! Makes `who` hold the press, so it receives all motion as \ref Widget::drag() and the next release,
        //! whatever the pointer passes over. Null releases the hold without calling anything.
        void grab(Widget *who);

        //! \}

        //! \name Keyboard
        //! \{

        //! Offers `pressed` to the focused widget and then its ancestors, and returns true when one consumed it.
        //! Returns false when nothing has focus.
        bool key(const Key &pressed) const;
        //! Passes `text`, in UTF-8, to the focused widget. Ignored when nothing has focus.
        void wrote(const std::string &text) const;

        //! Gives keyboard focus to `who`, or clears it when `who` is null. Does nothing when `who` already has it.
        //!
        //! Calls \ref Widget::lost_focus() on the old widget and \ref Widget::gained_focus() on the new one, then
        //! \ref composing.
        void focus(Widget *who);
        //! Returns the widget with keyboard focus, or null.
        Widget *focused() const { return _focused; }

        //! Moves focus to the next widget that takes focus, or the previous one when `backwards` is true.
        //!
        //! Only visible and enabled widgets count, in tree order, and the order wraps around. The candidates are
        //! taken from the topmost layer that has any, otherwise from the page, so an open dialog keeps focus to
        //! itself. Without a focused candidate, focus goes to the first one, or the last when `backwards` is true.
        void focus_next(bool backwards);

        //! Called after every focus change with the \ref Widget::takes_focus() of the newly focused widget, or false
        //! when focus was cleared. The shell turns the platform's text input on or off with it.
        std::function<void(bool)> composing;

        //! \}

        //! \name Animation
        //! \{

        //! Adds `who` to the live widgets, whose \ref Widget::advance() runs every frame, and cancels its wake-up
        //! from \ref wake_at().
        void live(Widget *who);

        //! Schedules `who` to become live at the frame time `when`, so the loop can sleep until then.
        //!
        //! When `who` is already scheduled, the earlier of the two times is kept. It does not remove `who` from the
        //! live widgets, which returning false from \ref Widget::advance() does.
        void wake_at(Widget *who, double when);

        //! Drops every reference the root holds to `who` and its descendants: live, scheduled, hovered, holding the
        //! press, focused and popup owner. \ref Widget::erase() and \ref Widget::clear() call it on what they destroy.
        void forget(const Widget *who);

        //! Makes live the scheduled widgets due at or before `now`, then calls \ref Widget::advance() on every live
        //! widget and drops those that return false.
        void advance(double now);

        //! Tests whether any widget is live.
        bool busy() const { return !_live.empty(); }

        //! Returns the earliest frame time a scheduled widget is due, or -1 when none is scheduled.
        [[nodiscard]] double waking() const;

        //! Sets the frame time to `now`, in seconds. The shell sets it before each frame and each event.
        void set_now(const double now) { _now = now; }

        //! Returns the frame time in seconds, the clock every animation is started against.
        double now() const { return _now; }

        //! \}

        //! Returns the typeface widgets lay text out with.
        Typeface &type() const { return _type; }

        //! \name Popups
        //! \{

        //! Registers `dismiss` as the way to close the open popup, which a press outside the \ref POPUPS layer calls.
        //!
        //! A press on `owner` or one of its descendants closes the popup and goes no further, so the control the
        //! popup hangs off does not reopen it. When a popup is already registered and `dismiss` is not null, the
        //! old one is dismissed first. A null `dismiss` clears the registration without calling it.
        void set_dismiss(std::function<void()> dismiss, const Widget *owner = nullptr) {
            // One at a time: what is already up closes rather than being left on the
            // layer with nothing able to reach it.
            if (_dismiss && dismiss) {
                this->dismiss();
            }

            _dismiss = std::move(dismiss);
            _owner = owner;
        }

        //! Tests whether a popup is registered through \ref set_dismiss().
        bool has_dismiss() const { return static_cast<bool>(_dismiss); }

        //! Tests whether the press being handled closed a popup. A dialog under the popup uses it so the press does
        //! not count as a click on its scrim.
        bool just_dismissed() const { return _justDismissed; }

        //! Clears the registered popup and calls its callback once. Does nothing when none is registered.
        void dismiss();

        //! \}

    private:
        Widget *pick(double x, double y);

        void hover_to(Widget *who, const Pointer &at);

        static void gather(const Widget *from, std::vector<Widget *> &out);

        // A layer over the page. Its children are stretched to the window unless it
        // floats them: a popup is anchored to the control it dropped from and keeps
        // the box it was placed at.
        class Page : public Widget {
        public:
            void floats() { _floats = true; }

            void arrange(Typeface &type) override;

        private:
            bool _floats = false;
        };

        Typeface &_type;

        Page _page;
        Page _layers[LAYERS];

        std::vector<BLRect> _dirty;
        std::vector<Shift> _shifts;
        bool _crowded = false;

        std::unordered_set<Widget *> _live;

        struct Sleeper {
            Widget *who;
            double due;
        };

        std::vector<Sleeper> _sleeping;

        Pointer _pointer{};

        Widget *_hovered = nullptr;
        Widget *_grabbed = nullptr;
        Widget *_focused = nullptr;

        std::function<void()> _dismiss;
        const Widget *_owner = nullptr;
        bool _justDismissed = false;

        double _width = 0.0;
        double _height = 0.0;
        double _now = 0.0;

        bool _relayout = true;
        std::uint32_t _revision = 0;
    };
}


#endif //TTK_TOOLKIT_ROOT_H
