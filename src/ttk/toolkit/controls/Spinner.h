// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_SPINNER_H
#define TTK_CONTROLS_SPINNER_H


#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
//! Arc turning round a faint ring, for something under way.
//!
//! Sized like a glyph and drawn in the accent colour unless given a \ref tone(). It turns once every \ref PERIOD
//! seconds in \ref STEPS steps a second, and only while it is being painted and enabled: once it is hidden, or
//! scrolled or clipped out of every repaint, it stops asking for frames, and the next paint starts it again.
//! Every spinner follows the frame clock, so all of them on screen turn together.
class Spinner : public Widget {
public:
    //! Seconds a spinner takes to turn once.
    static constexpr double PERIOD = 0.9;

    //! Times a second a turning spinner is drawn again.
    static constexpr double STEPS = 60.0;

    //! Returns how far round a spinner is at the frame time `now`, from 0 up to 1, held for a step at a time.
    [[nodiscard]] static double phase_at(double now);

    //! Returns the frame time after `now` at which \ref phase_at() next changes. Meant for a widget that paints
    //! spinners of its own to return from \ref Widget::advance() through \ref Widget::sleep_until().
    [[nodiscard]] static double step_after(double now);

    //! Paints a spinner at `phase`, from \ref phase_at(), in the largest square centred in `box`.
    //!
    //! For painting a spinner as part of another widget, without a `Spinner` of its own. Draws the ring in
    //! `tone` faded and the arc in `tone`, and nothing where \ref Painter::needed() rejects the square. Allocates
    //! nothing once the first one is drawn.
    static void render(const Painter &painter, const BLRect &box, double phase, BLRgba32 tone);

    //! Creates a spinner the size of a glyph drawn at `weight`, see \ref Glyphs::span().
    explicit Spinner(float weight = 1.2F);

    //! Sets the colour to `tone` and returns this. Defaults to the palette's accent.
    Spinner *tone(Theme::Tone tone);

    //! Returns \ref Widget::fixedWidth when it is set, otherwise the side of the glyph square.
    double natural_width(Typeface &type) override;

    //! Returns \ref Widget::fixedHeight when it is set, otherwise the side of the glyph square.
    double natural_height(Typeface &type, double width) override;

    //! Paints the spinner centred in the box, faded and still while disabled, and starts it turning when it was
    //! not.
    void paint(const Painter &painter) override;

    //! Repaints at each step while the spinner is enabled and was painted since the last one, sleeping in
    //! between. Returns false otherwise, which stops it until it is painted again.
    bool advance(double now) override;

protected:
    //! Called when the \ref Theme has changed. Rereads a palette slot given to \ref tone().
    void restyle() override;

private:
    double _side;
    Theme::Tone _tone{&Theme::Palette::accent};

    double _step = -1.0;
    bool _turning = false;
    bool _shown = false;
};
}


#endif //TTK_CONTROLS_SPINNER_H
