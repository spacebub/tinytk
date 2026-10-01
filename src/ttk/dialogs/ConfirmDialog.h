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
    //! Dialog that asks a yes or no question with a Cancel button and an accept button.
    class ConfirmDialog : public Dialog {
    public:
        //! Creates a dialog headed `title` with the message `said` and an accept button labelled `accept`.
        //!
        //! The accept button is styled as dangerous when `danger` is true, and as primary otherwise. Cancel calls
        //! \ref Dialog::dismissed. Accept calls \ref Dialog::dismissed and then `accepted`, so `accepted` runs after
        //! the dialog may already have been destroyed.
        ConfirmDialog(const std::string &title, const std::string &said, const std::string &accept,
                     bool danger, std::function<void()> accepted);

    private:
        std::function<void()> _accepted;
    };
}


#endif //TTK_DIALOGS_CONFIRMDIALOG_H
