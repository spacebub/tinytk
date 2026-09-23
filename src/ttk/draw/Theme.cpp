// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <cmath>

#include "ttk/draw/Theme.h"

namespace ttk {
    namespace {

        constexpr Theme::Palette DARK{
            .background = BLRgba32{0xff0a0d13},
            .surface = BLRgba32{0xff151b26},
            .raised = BLRgba32{0xff1d2431},
            .sunken = BLRgba32{0xff0e131c},
            .field = BLRgba32{0xff07090e},

            .hover = BLRgba32{0x16ffffff},
            .border = BLRgba32{0xff232c3a},
            .borderStrong = BLRgba32{0xff3b4759},

            .text = BLRgba32{0xffe4eaf5},
            .muted = BLRgba32{0xffa6b4cd},
            .faint = BLRgba32{0xff7a8cac},

            .accent = BLRgba32{0xffff7a1a},
            .accentHover = BLRgba32{0xffff9422},
            .accentText = BLRgba32{0xff1a0a02},
            .accentSoft = BLRgba32{0xff2b1607},

            .mutedSoft = BLRgba32{0xff212a37},
            .success = BLRgba32{0xff52d18b},
            .successSoft = BLRgba32{0xff0f2b1e},
            .warning = BLRgba32{0xffffc44d},
            .warningSoft = BLRgba32{0xff332609},
            .danger = BLRgba32{0xffff5470},
            .dangerSoft = BLRgba32{0xff35121c},

            .shadow = BLRgba32{0xa8000000},
            .scrim = BLRgba32{0xbe030509},

            .statusLaunching = BLRgba32{0xffff9422},
            .statusRunning = BLRgba32{0xff52d18b},
            .statusFailing = BLRgba32{0xffff5470},
            .statusIdle = BLRgba32{0xffa6b4cd},

            .headingWeight = 700,
            .dark = true,
        };

        constexpr Theme::Palette LIGHT{
            .background = BLRgba32{0xffe8ecf4},
            .surface = BLRgba32{0xffffffff},
            .raised = BLRgba32{0xffffffff},
            .sunken = BLRgba32{0xffdee4ef},
            .field = BLRgba32{0xffdae1ed},

            .hover = BLRgba32{0x140d1628},
            .border = BLRgba32{0xffccd5e4},
            .borderStrong = BLRgba32{0xffa9b6c9},

            .text = BLRgba32{0xff101620},
            .muted = BLRgba32{0xff46536a},
            .faint = BLRgba32{0xff64728a},

            .accent = BLRgba32{0xffc9450a},
            .accentHover = BLRgba32{0xffa83606},
            .accentText = BLRgba32{0xffffffff},
            .accentSoft = BLRgba32{0xffffe9dc},

            .mutedSoft = BLRgba32{0xffdde4ef},
            .success = BLRgba32{0xff0a7d4e},
            .successSoft = BLRgba32{0xffe0f6ec},
            .warning = BLRgba32{0xff8a5a08},
            .warningSoft = BLRgba32{0xfffcf0d8},
            .danger = BLRgba32{0xffc22a45},
            .dangerSoft = BLRgba32{0xfffde7ec},

            .shadow = BLRgba32{0x5212203a},
            .scrim = BLRgba32{0x780c1420},

            .statusLaunching = BLRgba32{0xffa83606},
            .statusRunning = BLRgba32{0xff0a7d4e},
            .statusFailing = BLRgba32{0xffc22a45},
            .statusIdle = BLRgba32{0xff46536a},

            .headingWeight = 600,
            .dark = false,
        };

        // Every slot, so a tone kept from one shade can be looked up in the other.
        struct Look {
            Theme::Palette dark = DARK;
            Theme::Palette light = LIGHT;
            Theme::Mode mode = Theme::Mode::System;
            bool systemDark = true;
        };

        constinit Look LOOK{};

        void show() {
            const Theme::Palette *was = Theme::detail::shown;

            Theme::detail::shown = &Theme::palette(LOOK.mode);

            if (Theme::detail::shown != was) {
                ++Theme::detail::revision;
            }
        }

    }

    namespace Theme {

        namespace detail {
            constinit const Palette *shown = &LOOK.dark;
            constinit std::uint32_t revision = 1;
        }

        const Palette &palette(const Mode mode) {
            switch (mode) {
                case Mode::Light:
                    return LOOK.light;
                case Mode::Dark:
                    return LOOK.dark;
                case Mode::System:
                    break;
            }

            return LOOK.systemDark ? LOOK.dark : LOOK.light;
        }

        Mode mode() {
            return LOOK.mode;
        }

        void set_mode(const Mode mode) {
            LOOK.mode = mode;

            show();
        }

        Mode cycle_mode() {
            switch (LOOK.mode) {
                case Mode::System:
                    set_mode(Mode::Light);
                    break;
                case Mode::Light:
                    set_mode(Mode::Dark);
                    break;
                case Mode::Dark:
                    set_mode(Mode::System);
                    break;
            }

            return LOOK.mode;
        }

        void configure(const Setup &setup) {
            LOOK.dark = setup.dark;
            LOOK.light = setup.light;
            LOOK.mode = setup.mode;
            detail::shown = &palette(setup.mode);
            ++detail::revision;
        }

        void set_system_dark(const bool dark) {
            LOOK.systemDark = dark;

            show();
        }

        Tone::Tone(BLRgba32 Palette::*const slot) : _slot(slot), _colour(palette().*slot) {}

        void Tone::restyle() {
            if (_slot != nullptr) {
                _colour = palette().*_slot;
            }
        }

        BLRgba32 alpha(const BLRgba32 tone, const double fraction) {
            const double clamped = std::clamp(fraction, 0.0, 1.0);

            return BLRgba32{tone.r(), tone.g(), tone.b(),
                            static_cast<uint32_t>(std::lround(tone.a() * clamped))};
        }

        BLRgba32 darker(const BLRgba32 tone, const double factor) {
            const double scale = 1.0 / (1.0 + std::max(0.0, factor));

            return BLRgba32{static_cast<uint32_t>(std::lround(tone.r() * scale)),
                            static_cast<uint32_t>(std::lround(tone.g() * scale)),
                            static_cast<uint32_t>(std::lround(tone.b() * scale)), tone.a()};
        }

        BLRgba32 lighter(const BLRgba32 tone, const double factor) {
            const double scale = 1.0 + std::max(0.0, factor);
            const auto lift = [scale](const uint32_t channel) {
                return static_cast<uint32_t>(std::clamp(std::lround(channel * scale), 0L, 255L));
            };

            return BLRgba32{lift(tone.r()), lift(tone.g()), lift(tone.b()), tone.a()};
        }

        BLRgba32 mix(const BLRgba32 under, const BLRgba32 over, const double amount) {
            const double weight = std::clamp(amount, 0.0, 1.0) * (over.a() / 255.0);
            // Both sides are widened first: a channel that falls wraps around if the
            // difference is taken between two unsigned ones.
            const auto blend = [weight](const uint32_t below, const uint32_t above) {
                const double from = below;
                const double to = above;

                return static_cast<uint32_t>(
                    std::clamp(std::lround(from + ((to - from) * weight)), 0L, 255L));
            };

            return BLRgba32{blend(under.r(), over.r()), blend(under.g(), over.g()),
                            blend(under.b(), over.b()), under.a()};
        }

    }
}
