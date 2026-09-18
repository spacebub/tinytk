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
    // A pill saying what a launched process is doing: a word in its own tone on a
    // tile, with a dot that beats while the run is still settling either way.
    class StatusIndicator : public Widget {
    public:
        enum class Status : std::uint8_t {
            Empty,
            Launching,
            Running,
            Stopping,
            Closed,
            Failed,
        };

        static constexpr double HEIGHT = 24.0;

        // How long the dot rests on each half of its beat.
        static constexpr double BEAT = 0.62;

        // For a face painted whole, with no room for a widget in it: the same pill,
        // drawn from its top-left corner and answering where it landed. `dim` is the
        // far half of the beat.
        static BLRect render(const Painter &painter, BLPoint at, Status status, bool dim);

        static double width_of(Typeface &type, Status status);

        [[nodiscard]] static bool beats(Status status);

        // The sentence behind the word. A failure says `reason` instead, when it has one.
        [[nodiscard]] static std::string say_of(Status status, const std::string &reason);

        StatusIndicator() = default;

        void set(Status status, std::string reason = {});

        // Gives the pill the pointer, and the sentence a line about what a click does.
        StatusIndicator *on_click(std::function<void()> clicked, std::string about);

        [[nodiscard]] Status status() const { return _status; }

        double natural_width(Typeface &type) override;
        double natural_height(Typeface & /*type*/, double /*width*/) override { return HEIGHT; }

        void paint(const Painter &painter) override;

        bool press(const Pointer &at) override;
        void release(const Pointer &at) override;

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
