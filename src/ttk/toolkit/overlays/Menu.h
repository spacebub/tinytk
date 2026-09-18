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
    // A short list of things to do, at the pointer.
    class Menu : public Widget {
    public:
        struct Row {
            // Whatever enum the page that built this menu uses. -1 on a rule.
            int action = -1;
            std::string label;
            Glyphs::Glyph glyph{};
            bool danger = false;
            bool separator = false;
            bool disabled = false;
        };

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

        static Row rule() {
            Row row;

            row.separator = true;

            return row;
        }

        static constexpr double WIDTH = 240.0;

        Menu(std::vector<Row> rows, std::function<void(int)> triggered);

        // How tall the rows come to, so the caller can place it.
        static double height_of(const std::vector<Row> &rows);

        void paint(const Painter &painter) override;

        bool press(const Pointer &at) override;
        void release(const Pointer &at) override;
        void hover(const Pointer &at) override;
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
