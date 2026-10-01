// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_DIALOGS_DIALOGLAYER_H
#define TTK_DIALOGS_DIALOGLAYER_H


#include <functional>
#include <memory>

#include "ttk/toolkit/overlays/Dialog.h"

namespace ttk {
    //! Stack of dialogs of which only the top one is shown.
    //!
    //! Meant for the \ref Root::DIALOGS layer. A dialog shown over another hides it until the one on top is taken
    //! down, so a dialog can open a second one over itself and get back to the first.
    class DialogLayer : public Widget {
    public:
        //! Takes ownership of `dialog`, shows it on top of the stack and returns it.
        //!
        //! The dialog that was on top is hidden. \ref Dialog::dismissed of `dialog` is set to call \ref close(), a
        //! relayout is requested, and \ref Dialog::opened() is called on `dialog` before returning.
        Dialog *show(std::unique_ptr<Dialog> dialog);

        //! Takes the top dialog down as \ref dismiss() does, unless its \ref Dialog::closing() returns false. Does
        //! nothing when no dialog is shown.
        void close();

        //! Takes the top dialog down without asking it. Does nothing when no dialog is shown.
        //!
        //! Calls \ref closed with the dialog, destroys it, shows the dialog under it and calls \ref Dialog::opened()
        //! on that one.
        void dismiss();

        //! Called with the top dialog by \ref dismiss() just before the dialog is destroyed.
        std::function<void(Dialog *)> closed;

        //! Returns the dialog on top of the stack, or null when none is shown.
        [[nodiscard]] Dialog *top() const;

        //! Calls \ref Dialog::sync() on the top dialog. Does nothing when no dialog is shown.
        void sync() const;

        //! Tests whether any dialog is shown. An application uses it to keep the window from being dragged through a
        //! dialog.
        [[nodiscard]] bool covered() const { return !children().empty(); }

        //! Places every visible dialog over the layer's box below the title bar, which stays uncovered. Hidden dialogs
        //! are not laid out.
        void arrange(Typeface &type) override;
    };
}


#endif //TTK_DIALOGS_DIALOGLAYER_H
