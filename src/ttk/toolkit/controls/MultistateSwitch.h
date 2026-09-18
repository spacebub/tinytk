// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_MULTISTATESWITCH_H
#define TTK_CONTROLS_MULTISTATESWITCH_H


#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "ttk/draw/Anim.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
    // One of a short list, as a row of words in a trough.
    class MultistateSwitch : public Widget {
    public:
        // `value` is the page's own enum, cast to an int. A word spelled three times
        // over, here, in the handler and in the table that reads it back, compiles
        // just as well when one of the three is wrong.
        struct Choice {
            int value = 0;
            std::string label;
            bool badge = false;

            bool operator==(const Choice &other) const = default;
        };

        explicit MultistateSwitch(std::function<void(int)> selected);

        void set_options(std::vector<Choice> options);
        void set_current(int value);

        [[nodiscard]] std::optional<int> current() const { return _current; }

        double natural_width(Typeface &type) override;
        double natural_height(Typeface & /*type*/, double /*width*/) override { return Theme::control; }

        void arrange(Typeface &type) override;

        void paint(const Painter &painter) override;

        bool press(const Pointer &at) override;
        void release(const Pointer &at) override;
        void hover(const Pointer &at) override;
        void leave() override;

        bool advance(double now) override;

    private:
        // The box each word sits in, worked out at paint and layout time alike.
        std::vector<BLRect> lanes(Typeface &type) const;

        std::vector<Choice> _options;
        std::optional<int> _current;

        std::function<void(int)> _selected;

        int _over = -1;

        Anim::Tween _markX;
        Anim::Tween _markWidth;
        bool _marked = false;
    };
}


#endif //TTK_CONTROLS_MULTISTATESWITCH_H
