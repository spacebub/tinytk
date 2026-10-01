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
    //! Dialog that asks for one line of text, with a Cancel button and an accept button.
    class PromptDialog : public Dialog {
    public:
        //! Creates a dialog headed `title` with a field captioned `label` holding `value`, and an accept button
        //! labelled `accept`.
        //!
        //! The accept button is disabled while the text is empty or only whitespace. Accepting, by the button or by
        //! Return in the field, calls \ref Dialog::dismissed and then `accepted` with the text trimmed of surrounding
        //! whitespace, so `accepted` runs after the dialog may already have been destroyed. Cancel calls
        //! \ref Dialog::dismissed.
        PromptDialog(const std::string &title, const std::string &label, std::string value,
                    const std::string &accept, std::function<void(const std::string &)> accepted);

        //! Gives the field keyboard focus.
        void opened() override;

    private:
        void commit() const;

        std::function<void(const std::string &)> _accepted;

        Field *_field = nullptr;
        Button *_accept = nullptr;

        std::string _value;
    };
}


#endif //TTK_DIALOGS_PROMPTDIALOG_H
