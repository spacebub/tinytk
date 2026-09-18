// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_DIALOGS_CONFIRMDIALOG_H
#define TTK_DIALOGS_CONFIRMDIALOG_H


#include <functional>
#include <string>

#include "ttk/toolkit/overlays/Dialog.h"

namespace ttk {
    // A question with two answers, one of which may be the dangerous one.
    class ConfirmDialog : public ttk::Dialog {
    public:
        ConfirmDialog(const std::string &title, const std::string &said, const std::string &accept,
                     bool danger, std::function<void()> accepted);

    private:
        std::function<void()> _accepted;
    };
}


#endif //TTK_DIALOGS_CONFIRMDIALOG_H
