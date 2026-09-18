// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_TABSTRIP_H
#define TTK_CONTROLS_TABSTRIP_H


#include <functional>
#include <string>
#include <vector>

#include "ttk/draw/Anim.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
    // A row of names, one of them underlined. It reports a press and leaves the
    // choice to whoever owns it.
    class TabStrip : public Widget {
    public:
        struct Tab {
            std::string label;
            // A dot beside the name, for a page that wants a look.
            bool badge = false;
        };

        explicit TabStrip(std::function<void(int)> selected);

        void set_tabs(std::vector<Tab> tabs);
        void set_current(int index);
        void set_badge(int index, bool badge);

        [[nodiscard]] int current() const { return _current; }

        double natural_width(Typeface &type) override;
        double natural_height(Typeface &type, double width) override;
        void arrange(Typeface &type) override;
        void paint(const Painter &painter) override;
        bool press(const Pointer &at) override;
        void release(const Pointer &at) override;
        void hover(const Pointer &at) override;
        void leave() override;
        bool advance(double now) override;

    private:
        struct Held {
            Tab tab;
            BLRect box{};
            double width = 0.0;
            Anim::Tween on{};
        };

        static constexpr double SIDES = 30.0;
        static constexpr double GAP = 2.0;

        [[nodiscard]] int at_point(double x, double y) const;

        std::function<void(int)> _selected;
        std::vector<Held> _tabs;
        int _current = -1;
        int _over = -1;
    };
}


#endif //TTK_CONTROLS_TABSTRIP_H
