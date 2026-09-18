// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <utility>

#include "ttk/dialogs/ConfirmDialog.h"
#include "ttk/toolkit/controls/Button.h"
#include "ttk/toolkit/layout/Spacer.h"

namespace ttk {

    ConfirmDialog::ConfirmDialog(const std::string &title, const std::string &said,
                               const std::string &accept, const bool danger,
                               std::function<void()> accepted)
        : _accepted(std::move(accepted)) {
        wanted = 460.0;

        Box *column = card()->append(Box::column());

        column->pad(22.0)->spacing(10.0);

        heading(column, title);
        body(column, said);

        column->append(std::make_unique<Spacer>(0.0))->fixedHeight = 8.0;

        Box *row = column->append(Box::row());

        row->spacing(8.0)->align(Box::Place::End);
        row->fixedHeight = Theme::control;

        row->append(std::make_unique<Button>("Cancel", [this] {
            if (dismissed) {
                dismissed();
            }
        }));

        // Copied out: answering takes the dialog down, and the callable with it.
        row->append(std::make_unique<Button>(accept, [this] {
            const std::function<void()> fire = _accepted;
            const std::function<void()> shut = dismissed;

            if (shut) {
                shut();
            }

            if (fire) {
                fire();
            }
        }))->kind(danger ? Button::Kind::Danger : Button::Kind::Primary);
    }
}
