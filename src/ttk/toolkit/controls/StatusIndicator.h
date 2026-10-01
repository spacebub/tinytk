// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_STATUSINDICATOR_H
#define TTK_CONTROLS_STATUSINDICATOR_H


#include <functional>
#include <string>

#include "ttk/toolkit/Widget.h"

namespace ttk {
    //! Pill that shows the state of a launched process as a coloured word and a dot on a dark tile.
    //!
    //! While the status is \ref Status::Launching or \ref Status::Stopping the dot blinks, switching between full
    //! and faint every \ref BEAT seconds. The tooltip is set to \ref say_of() for the current status.
    class StatusIndicator : public Widget {
    public:
        //! State of the process shown by the pill.
        enum class Status : std::uint8_t {
            //! No process. Nothing is drawn, the width is 0 and there is no tooltip.
            Empty,
            //! Started and still loading. Drawn as "Launching" with a blinking dot.
            Launching,
            //! Up and running. Drawn as "Running".
            Running,
            //! Asked to quit and not yet gone. Drawn as "Stopping" with a blinking dot.
            Stopping,
            //! Closed. Drawn as "Closed".
            Closed,
            //! Did not start. Drawn as "Failed".
            Failed,
        };

        //! Height of the pill in pixels.
        static constexpr double HEIGHT = 24.0;

        //! Time in seconds the blinking dot stays on each of its full and faint phases.
        static constexpr double BEAT = 0.62;

        //! Paints the pill for `status` with its top-left corner at `at` and returns the box it covers.
        //!
        //! For painting the pill as part of another widget, without a `StatusIndicator` of its own. `dim` paints
        //! the faint phase of the dot, and is ignored for a status that does not blink. Paints nothing and returns
        //! an empty rectangle for \ref Status::Empty.
        static BLRect render(const Painter &painter, BLPoint at, Status status, bool dim);

        //! Returns the width in pixels of the pill for `status`, or 0 for \ref Status::Empty.
        static double width_of(Typeface &type, Status status);

        //! Tests whether the dot blinks for `status`, which is true for \ref Status::Launching and
        //! \ref Status::Stopping.
        [[nodiscard]] static bool beats(Status status);

        //! Returns a sentence that explains `status`.
        //!
        //! For \ref Status::Failed it returns `reason` when that is not empty. `reason` is ignored for every other
        //! status. \ref Status::Empty gets the same sentence as \ref Status::Closed.
        [[nodiscard]] static std::string say_of(Status status, const std::string &reason);

        //! Creates an indicator in the \ref Status::Empty state.
        StatusIndicator() = default;

        //! Sets the status to `status` and the failure reason to `reason`, updates the tooltip and repaints.
        //!
        //! Does nothing when both are unchanged. Restarts the blink on its full phase and wakes the widget when
        //! `status` blinks. The widget is not laid out again, so a change of width shows after the next layout.
        void set(Status status, std::string reason = {});

        //! Makes the pill clickable, calling `clicked` on a click, and returns this.
        //!
        //! `about` is appended to the tooltip to say what a click does. A null `clicked` makes the pill ignore the
        //! pointer again.
        StatusIndicator *on_click(std::function<void()> clicked, std::string about);

        //! Returns the current status.
        [[nodiscard]] Status status() const { return _status; }

        //! Returns \ref Widget::fixedWidth when it is set, otherwise \ref width_of() for the current status.
        double natural_width(Typeface &type) override;

        //! Returns \ref HEIGHT.
        double natural_height(Typeface & /*type*/, double /*width*/) override { return HEIGHT; }

        //! Paints the pill at the top-left corner of the box.
        void paint(const Painter &painter) override;

        //! Takes the press when a click handler is set through \ref on_click().
        bool press(const Pointer &at) override;

        //! Calls the click handler when `at` is still inside the box.
        void release(const Pointer &at) override;

        //! Flips the dot between its phases every \ref BEAT seconds while the status blinks, sleeping in between.
        //! Returns false once the status no longer blinks.
        bool advance(double now) override;

    private:
        void retell();

        Status _status = Status::Empty;
        std::string _reason;

        std::function<void()> _clicked;
        std::string _about;

        double _blinked = 0.0;
        bool _dim = false;
    };
}


#endif //TTK_CONTROLS_STATUSINDICATOR_H
