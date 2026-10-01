// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_OVERLAYS_MENU_H
#define TTK_OVERLAYS_MENU_H


#include <functional>
#include <string>
#include <vector>

#include "ttk/toolkit/Widget.h"
#include <type_traits>

#include "ttk/draw/Glyphs.h"

namespace ttk {
    //! Popup list of actions, each row reporting an action code when clicked.
    //!
    //! The menu neither places nor closes itself. The caller adds it to the \ref Root::POPUPS layer at the box it
    //! wants, \ref WIDTH wide and \ref height_of() tall, and removes it, normally from the trigger callback and
    //! from \ref Root::set_dismiss().
    class Menu : public Widget {
    public:
        //! One row of a menu: an action or a separating rule.
        struct Row {
            //! Action code passed to the trigger callback, the value of the caller's enum. -1 on a rule.
            int action = -1;
            //! Text of the row.
            std::string label;
            //! Icon drawn before the label. \ref Glyphs::Glyph::Empty draws none.
            Glyphs::Glyph glyph{};
            //! Whether the row is drawn in the danger colour, for a destructive action.
            bool danger = false;
            //! Whether the row is a horizontal rule. A rule cannot be triggered and its other fields are ignored.
            bool separator = false;
            //! Whether the row is dimmed and cannot be triggered.
            bool disabled = false;
        };

        //! Creates an action row that reports `action` as an integer, with `label`, `glyph` and its flags.
        template <typename Action>
            requires std::is_enum_v<Action>
        static Row item(const Action action, std::string label, Glyphs::Glyph glyph,
                        const bool danger = false, const bool disabled = false) {
            Row row;

            row.action = static_cast<int>(action);
            row.label = std::move(label);
            row.glyph = glyph;
            row.danger = danger;
            row.disabled = disabled;

            return row;
        }

        //! Creates a separator row, a horizontal rule between groups of actions.
        static Row rule() {
            Row row;

            row.separator = true;

            return row;
        }

        //! Width in pixels to place a menu at.
        static constexpr double WIDTH = 240.0;

        //! Creates a menu of `rows` that calls `triggered` with the \ref Row::action of a row clicked on.
        //!
        //! `triggered` may destroy the menu, as the menu keeps nothing it needs once the call starts.
        Menu(std::vector<Row> rows, std::function<void(int)> triggered);

        //! Returns the height in pixels a menu of `rows` needs, 32 per action, 9 per rule and 10 of padding.
        static double height_of(const std::vector<Row> &rows);

        //! Paints the menu's panel and rows, highlighting the row under the pointer.
        void paint(const Painter &painter) override;

        //! Takes a press inside the menu's box.
        bool press(const Pointer &at) override;
        //! Calls the trigger callback with the action of the row at the height of `at`, unless that row is a rule,
        //! is disabled, or there is no row there.
        void release(const Pointer &at) override;
        //! Highlights the row under the pointer.
        void hover(const Pointer &at) override;
        //! Clears the hover state and the row highlight.
        void leave() override;

    private:
        [[nodiscard]] int row_at(double y) const;

        static constexpr double ROW = 32.0;
        static constexpr double RULE = 9.0;

        std::vector<Row> _rows;
        std::function<void(int)> _triggered;

        int _over = -1;
    };
}


#endif //TTK_OVERLAYS_MENU_H
