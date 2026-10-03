// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_DRAW_THEME_H
#define TTK_DRAW_THEME_H


#include <cstdint>

#include <blend2d/blend2d.h>

//! Colours and sizes shared by every widget, with a light and a dark palette.
namespace ttk::Theme {
    //! Choice of palette.
    enum class Mode : std::uint8_t {
        //! The light palette.
        Light,
        //! The dark palette.
        Dark,
        //! The palette matching the desktop, as last given to \ref set_system_dark(). Dark until told otherwise.
        System,
    };

    //! Every colour of one palette, and the font weight of headings. Colours are not premultiplied.
    struct Palette {
        //! Fill of the window behind everything else.
        BLRgba32 background;
        //! Fill of cards and panels laid on the background.
        BLRgba32 surface;
        //! Fill of controls that stand above a surface, such as buttons, chips and selects.
        BLRgba32 raised;
        //! Fill of recessed areas, such as tracks, wells and gutters.
        BLRgba32 sunken;
        //! Fill of text inputs.
        BLRgba32 field;

        //! Translucent wash laid over an item under the pointer.
        BLRgba32 hover;
        //! Hairline border of surfaces and controls.
        BLRgba32 border;
        //! Border with more contrast than \ref border, for controls that must stand out or are hovered.
        BLRgba32 borderStrong;

        //! Main text colour.
        BLRgba32 text;
        //! Secondary text colour.
        BLRgba32 muted;
        //! Text and glyph colour for the least important content, such as placeholders and icons at rest.
        BLRgba32 faint;

        //! Accent colour of primary buttons, selections and active controls.
        BLRgba32 accent;
        //! Accent colour of a primary button under the pointer.
        BLRgba32 accentHover;
        //! Text and glyph colour drawn on \ref accent.
        BLRgba32 accentText;
        //! Pale tint of \ref accent for backgrounds of selected or highlighted items.
        BLRgba32 accentSoft;

        //! Pale neutral background of neutral badges.
        BLRgba32 mutedSoft;
        //! Colour of success messages and states.
        BLRgba32 success;
        //! Pale tint of \ref success for backgrounds.
        BLRgba32 successSoft;
        //! Colour of warnings.
        BLRgba32 warning;
        //! Pale tint of \ref warning for backgrounds.
        BLRgba32 warningSoft;
        //! Colour of errors and destructive actions.
        BLRgba32 danger;
        //! Pale tint of \ref danger for backgrounds.
        BLRgba32 dangerSoft;

        //! Colour of drop shadows.
        BLRgba32 shadow;
        //! Translucent fill laid over the window behind a dialog.
        BLRgba32 scrim;

        //! Font weight of headings, such as 600 or 700.
        int headingWeight;
        //! True for a dark palette.
        bool dark;
    };

    //! State behind \ref palette() and \ref revision(). Not for direct use.
    namespace detail {
        //! Palette on screen.
        extern const Palette *shown;
        //! Counter behind \ref revision().
        extern std::uint32_t revision;
    }

    //! Returns the palette on screen, the one \ref mode() selects.
    inline const Palette &palette() { return *detail::shown; }

    //! Returns the palette `mode` selects. For \ref Mode::System it follows the last \ref set_system_dark().
    //!
    //! The reference stays valid for the life of the program, and \ref configure() changes what it holds.
    const Palette &palette(Mode mode);

    //! Returns the mode last set.
    Mode mode();

    //! Sets the mode to `mode` and shows its palette, advancing \ref revision() when the palette on screen changes.
    void set_mode(Mode mode);

    //! Moves the mode on from \ref Mode::System to \ref Mode::Light to \ref Mode::Dark and back to
    //! \ref Mode::System, and returns the new mode.
    Mode cycle_mode();

    //! Palettes and mode given to \ref configure().
    //!
    //! The palettes of a default `Setup` are all zero, not the built-in ones. Start from \ref palette(Mode) to
    //! change only some slots.
    struct Setup {
        //! Palette of \ref Mode::Dark.
        Palette dark{};
        //! Palette of \ref Mode::Light.
        Palette light{};
        //! Mode to show.
        Mode mode = Mode::System;
    };

    //! Replaces both palettes and the mode with those in `setup`, shows the selected palette and advances
    //! \ref revision().
    void configure(const Setup &setup);

    //! Sets whether the desktop prefers a dark appearance, which \ref Mode::System follows.
    //!
    //! The window calls it at start and whenever the desktop's preference changes. Advances \ref revision() when
    //! the palette on screen changes.
    void set_system_dark(bool dark);

    //! Font size of the smallest text, such as captions and badges, in pixels.
    constexpr float fontTiny = 12.0F;
    //! Font size of secondary text, in pixels.
    constexpr float fontSmall = 13.0F;
    //! Font size of body text, in pixels.
    constexpr float fontBody = 14.0F;
    //! Font size of card and section headings, in pixels.
    constexpr float fontMedium = 15.0F;
    //! Font size of dialog titles, in pixels.
    constexpr float fontLarge = 17.0F;
    //! Font size of page titles, in pixels.
    constexpr float fontTitle = 19.0F;
    //! Font size of large display text, in pixels.
    constexpr float fontDisplay = 23.0F;
    //! Font size of the largest display text, in pixels.
    constexpr float fontHero = 44.0F;

    //! Height of a control such as a button or a switch, in pixels.
    constexpr double control = 42.0;
    //! Height of a compact control, in pixels.
    constexpr double controlSmall = 32.0;
    //! Smallest width of a button that is not compact, in pixels.
    constexpr double buttonWidth = 140.0;

    //! Corner radius of dialogs, toasts and list panels, in pixels.
    constexpr double radius = 12.0;
    //! Corner radius of controls, in pixels.
    constexpr double radiusSmall = 8.0;
    //! Corner radius of large panels, in pixels.
    constexpr double radiusLarge = 18.0;
    //! Space between neighbouring widgets in a layout, in pixels.
    constexpr double gap = 12.0;
    //! Padding inside a page or panel, in pixels.
    constexpr double pad = 20.0;

    //! Height of the bar across the top of the window, in pixels. Toasts and dialogs keep below it.
    constexpr double barHeight = 54.0;

    //! Margin left and right of the cells of a \ref ReorderGrid, in pixels.
    constexpr double bleed = 16.0;
    //! Gap between cells of a \ref ReorderGrid, in pixels.
    constexpr double gutter = 20.0;

    //! Margin left and right of a page's content, in pixels.
    constexpr double pageMargin = 22.0;
    //! Margin above a page's content, in pixels.
    constexpr double pageTop = 22.0;

    //! Width of a scroll bar lane along the right edge of a scrolling view, in pixels.
    constexpr double lane = 10.0;

    //! Returns a counter that advances whenever the palette on screen changes.
    //!
    //! Widgets compare it with the value they last saw to know when to call \ref Widget::restyle().
    inline std::uint32_t revision() { return detail::revision; }

    //! Colour slots a program adds beside \ref Palette, one set for the dark palettes and one for the light.
    //!
    //! `Slots` is any struct of colours. Read it through \ref of() from a function given to a \ref Tone or a
    //! \ref Button::Kind, so it follows palette changes as the built-in slots do.
    //!
    //! \code
    //! struct Brand { BLRgba32 tint; };
    //!
    //! constexpr Theme::Extension<Brand> BRAND{.dark = {.tint = BLRgba32{0xff80c0ff}},
    //!                                         .light = {.tint = BLRgba32{0xff1a5fa0}}};
    //!
    //! Theme::Tone tint([](const Theme::Palette &palette) { return BRAND.of(palette).tint; });
    //! \endcode
    template <class Slots>
    struct Extension {
        //! Slots shown with a palette whose \ref Palette::dark is true.
        Slots dark;
        //! Slots shown with a palette whose \ref Palette::dark is false.
        Slots light;

        //! Returns the set of slots that goes with `palette`.
        [[nodiscard]] constexpr const Slots &of(const Palette &palette) const {
            return palette.dark ? dark : light;
        }
    };

    //! Colour that is a palette slot or derived from the palette, both of which follow palette changes, or a fixed
    //! colour, which does not.
    class Tone {
    public:
        //! Creates a tone that follows `slot` of the palette, taking its colour from the palette on screen.
        Tone(BLRgba32 Palette::*slot);

        //! Creates a tone that `derive` computes from the palette on screen, such as a slot of an \ref Extension.
        Tone(BLRgba32 (*derive)(const Palette &palette));

        //! Creates a tone fixed at `colour`.
        explicit Tone(const BLRgba32 colour) : _colour(colour) {}

        //! Reads the slot or derives the colour again from the palette on screen. Does nothing for a fixed colour.
        //!
        //! Call it from \ref Widget::restyle() of the widget holding the tone.
        void restyle();

        //! Returns the colour as of construction or the last \ref restyle().
        [[nodiscard]] BLRgba32 colour() const { return _colour; }

    private:
        BLRgba32 Palette::*_slot = nullptr;
        BLRgba32 (*_derive)(const Palette &) = nullptr;
        BLRgba32 _colour{};
    };

    //! Returns `tone` with its alpha multiplied by `fraction`, clamped to `[0, 1]`.
    BLRgba32 alpha(BLRgba32 tone, double fraction);

    //! Returns `tone` with red, green and blue divided by `1 + factor`. A negative `factor` counts as zero. Alpha is
    //! kept.
    BLRgba32 darker(BLRgba32 tone, double factor);

    //! Returns `tone` with red, green and blue multiplied by `1 + factor` and capped at 255. A negative `factor`
    //! counts as zero. Alpha is kept.
    BLRgba32 lighter(BLRgba32 tone, double factor);

    //! Returns `under` moved toward `over` by `amount`, clamped to `[0, 1]` and scaled by the alpha of `over`.
    //!
    //! Red, green and blue are interpolated. The alpha of `under` is kept.
    BLRgba32 mix(BLRgba32 under, BLRgba32 over, double amount);
}


#endif //TTK_DRAW_THEME_H
