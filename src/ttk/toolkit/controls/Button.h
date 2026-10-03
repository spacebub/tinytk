// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_BUTTON_H
#define TTK_CONTROLS_BUTTON_H


#include <functional>
#include <string>

#include "ttk/draw/Anim.h"
#include "ttk/draw/Glyphs.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
    namespace Theme {
        struct Palette;
    }
    //! Push button with a text label and an optional glyph before it.
    //!
    //! A click is a press released inside the button, or Return or Space while it has focus. The methods that
    //! return `this` are meant to be chained at construction: \ref kind(), \ref glyph() and \ref compact() neither
    //! repaint nor ask for a new layout.
    class Button : public Widget {
    public:
        //! Colours a button is painted with.
        struct Look {
            //! Fill at rest.
            BLRgba32 ground;
            //! Fill under the pointer, blended from \ref ground while the hover fades.
            BLRgba32 lit;
            //! Fill while pressed.
            BLRgba32 down;
            //! Border at rest. No border is drawn when both edges are fully transparent.
            BLRgba32 edge;
            //! Border under the pointer, blended from \ref edge while the hover fades.
            BLRgba32 edgeLit;
            //! Colour of the label and glyph.
            BLRgba32 ink;
            //! Colour of the busy dots.
            BLRgba32 dots;
        };

        //! Visual style of a button.
        //!
        //! A program adds its own kinds beside the built-in ones with a function that derives a \ref Look from a
        //! palette. The look is derived again whenever the palette on screen changes.
        struct Kind {
            //! Creates a kind whose look `derive` computes from a palette. A null `derive` paints as \ref Default.
            constexpr explicit Kind(Look (*const derive)(const Theme::Palette &palette)) : look(derive) {}

            //! Returns the look of the kind under `palette`.
            Look (*look)(const Theme::Palette &palette);

            //! True when both kinds derive their look with the same function.
            constexpr bool operator==(const Kind &) const = default;

            //! Raised surface with a border that turns to the accent colour on hover.
            static const Kind Default;
            //! Filled with the accent colour, for the main action of a view.
            static const Kind Primary;
            //! Danger coloured text and border, for a destructive action.
            static const Kind Danger;
            //! No border and accent coloured text, with a soft accent wash on hover.
            static const Kind Ghost;
        };

        //! Creates a button showing `text` that calls `clicked` on each click. `clicked` may be empty.
        Button(std::string text, std::function<void()> clicked);

        //! Sets the label to `text` and repaints. The layout is not redone, so the width stays as it was.
        void set_text(std::string text);

        //! Sets the visual style to `value`, derives its look from the palette on screen and returns this button.
        Button *kind(Kind value);

        //! Sets the glyph drawn before the label and returns this button. \ref Glyphs::Glyph::Empty shows none.
        Button *glyph(Glyphs::Glyph glyph);

        //! Selects the small variant, with a lower height, a smaller font and no minimum width, and returns this
        //! button.
        Button *compact(bool value = true);

        //! Sets whether the button is busy and returns this button.
        //!
        //! A busy button replaces its label with three animated dots and ignores clicks, both from the pointer and
        //! from the keyboard.
        Button *busy(bool value);

        //! Sets the tooltip, \ref Widget::hint, to `text` and returns this button.
        Button *tooltip(std::string text);

        //! Returns the width of the glyph and label plus padding, never less than \ref Theme::buttonWidth unless
        //! the button is compact. Returns \ref fixedWidth when it is set.
        double natural_width(Typeface &type) override;

        //! Returns \ref Theme::control, or \ref Theme::controlSmall when compact. Returns \ref fixedHeight when it
        //! is set.
        double natural_height(Typeface &type, double width) override;

        //! Paints the button body in its \ref Kind style, then either the glyph and label centred in the box or the
        //! busy dots. A disabled button draws its label faded.
        void paint(const Painter &painter) override;

        //! Takes the press unless the button is disabled or busy, and shrinks the body slightly while it is held.
        bool press(const Pointer &at) override;

        //! Ends the press and calls the click callback when `at` is still inside the button and it is enabled
        //! and not busy.
        void release(const Pointer &at) override;

        //! Called when the pointer moves onto the button. Starts the hover highlight.
        void enter() override;

        //! Called when the pointer moves off the button. Fades out the hover highlight.
        void leave() override;

        //! Tests whether the button can receive keyboard focus, which is whenever it is enabled.
        [[nodiscard]] bool takes_focus() const override { return enabled(); }

        //! Consumes Return and Space and clicks the button on them when it is enabled and not busy. Returns false
        //! for every other key.
        bool key(const Key &pressed) override;

        //! Steps the hover and press animations and, while busy, the dots. Returns true while either animation
        //! runs, otherwise sleeps until the next dot step when busy.
        bool advance(double now) override;

    protected:
        //! Called when the \ref Theme has changed. Derives the look of the kind again.
        void restyle() override;

    private:
        void derive_look();

        std::string _text;
        Glyphs::Glyph _glyph{};
        std::function<void()> _clicked;

        Kind _kind = Kind::Default;
        Look _look{};
        bool _compact = false;
        bool _busy = false;

        Anim::Tween _lit;
        Anim::Tween _give;

        int _tick = 0;
        double _ticked = 0.0;
    };
}


#endif //TTK_CONTROLS_BUTTON_H
