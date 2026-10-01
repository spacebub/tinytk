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
    //! Row of tab names with the current one underlined.
    //!
    //! The strip does not change \ref current() when a tab is clicked. It reports the index through the
    //! `selected` callback, and the owner calls \ref set_current() to accept it. Tabs are laid out from the left
    //! edge of the box, each as wide as its name plus 30 pixels, with 2 pixels between them.
    class TabStrip : public Widget {
    public:
        //! One tab of a \ref TabStrip.
        struct Tab {
            //! Name shown on the tab.
            std::string label;
            //! Whether to draw a dot beside the name to draw attention to the page. Not drawn on the current tab.
            bool badge = false;
        };

        //! Creates an empty strip that reports clicked tabs to `selected`.
        //!
        //! `selected` is called with the index of the tab under the pointer when a press on a tab is released.
        explicit TabStrip(std::function<void(int)> selected);

        //! Replaces the tabs with `tabs` and asks the root for a new layout.
        //!
        //! \ref current() is kept when it is still in range and set to -1 otherwise. The underline moves to it
        //! without animating.
        void set_tabs(std::vector<Tab> tabs);

        //! Sets the current tab to `index` without calling the `selected` callback, and animates the underline to
        //! it. A negative or out of range `index` underlines no tab.
        void set_current(int index);

        //! Sets \ref Tab::badge of the tab at `index` to `badge` and repaints. Does nothing when `index` is out of
        //! range.
        void set_badge(int index, bool badge);

        //! Returns the index of the current tab, or -1 when there is none.
        [[nodiscard]] int current() const { return _current; }

        //! Returns the width of all tabs side by side, ignoring \ref Widget::fixedWidth.
        double natural_width(Typeface &type) override;

        //! Returns \ref Theme::control.
        double natural_height(Typeface &type, double width) override;

        //! Places each tab at its width from the left edge of the box, at the full height of the box.
        void arrange(Typeface &type) override;

        //! Paints the tab names, the underline of the current tab and the badge dots.
        void paint(const Painter &painter) override;

        //! Takes the press when `at` is on a tab.
        bool press(const Pointer &at) override;

        //! Calls the `selected` callback with the index of the tab under `at`, if any.
        void release(const Pointer &at) override;

        //! Highlights the name of the tab under `at`.
        void hover(const Pointer &at) override;

        //! Called when the pointer moves off the strip. Clears the highlighted name.
        void leave() override;

        //! Steps the underline animations and returns whether any is still running.
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
