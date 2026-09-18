// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_DIALOGS_PROMPTDIALOG_H
#define TTK_DIALOGS_PROMPTDIALOG_H


#include <functional>
#include <string>

#include "ttk/toolkit/controls/Field.h"
#include "ttk/toolkit/overlays/Dialog.h"

namespace ttk {
    // One line asked for, with a name for what it is.
    class PromptDialog : public ttk::Dialog {
    public:
        PromptDialog(const std::string &title, const std::string &label, std::string value,
                    const std::string &accept, std::function<void(const std::string &)> accepted);

        void opened() override;

    private:
        void commit() const;

        std::function<void(const std::string &)> _accepted;

        ttk::Field *_field = nullptr;
        ttk::Button *_accept = nullptr;

        std::string _value;
    };
}


#endif //TTK_DIALOGS_PROMPTDIALOG_H
