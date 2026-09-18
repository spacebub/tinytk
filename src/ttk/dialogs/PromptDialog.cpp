// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <utility>

#include "ttk/system/Text.h"
#include "ttk/dialogs/PromptDialog.h"
#include "ttk/toolkit/controls/Button.h"

namespace ttk {

    PromptDialog::PromptDialog(const std::string &title, const std::string &label,
                             std::string value, const std::string &accept,
                             std::function<void(const std::string &)> accepted)
        : _accepted(std::move(accepted)), _value(std::move(value)) {
        wanted = 460.0;

        Box *column = card()->append(Box::column());

        column->pad(22.0)->spacing(16.0);

        heading(column, title);

        _field = column->append(std::make_unique<Field>(label, [this](const std::string &typed) {
            _value = typed;

            _accept->set_enabled(!Text::trim(_value).empty());
        }));

        _field->accepted = [this] { commit(); };
        _field->set_text(_value);

        Box *row = column->append(Box::row());

        row->spacing(8.0)->align(Box::Place::End);
        row->fixedHeight = Theme::control;

        row->append(std::make_unique<Button>("Cancel", [this] {
            if (dismissed) {
                dismissed();
            }
        }));

        _accept = row->append(std::make_unique<Button>(accept, [this] { commit(); }));
        _accept->kind(Button::Kind::Primary);
        _accept->set_enabled(!Text::trim(_value).empty());
    }

    void PromptDialog::opened() {
        _field->take_focus();
    }

    void PromptDialog::commit() const {
        const std::string tidy = Text::trim(_value);

        if (tidy.empty()) {
            return;
        }

        // Copied out: accepting takes the dialog down, and the callable with it.
        const std::function<void(const std::string &)> fire = _accepted;
        const std::function<void()> shut = dismissed;

        if (shut) {
            shut();
        }

        if (fire) {
            fire(tidy);
        }
    }
}
