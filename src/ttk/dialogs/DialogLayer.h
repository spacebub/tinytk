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
    // One dialog shown over another hides it until it goes, so a dialog can open a
    // second over itself and get it back.
    class DialogLayer : public ttk::Widget {
    public:
        ttk::Dialog *show(std::unique_ptr<ttk::Dialog> dialog);

        // Takes the dialog down unless it says it dealt with the dismissal itself.
        void close();

        // Takes it down whatever it says, for a dialog whose reason for being up is gone.
        void dismiss();

        // Called with a dialog as it goes, for whoever was holding on to it.
        std::function<void(ttk::Dialog *)> closed;

        // The one that is up, or nothing.
        [[nodiscard]] ttk::Dialog *top() const;

        void sync() const;

        // True while one is up, which is what stops the window being dragged by it.
        [[nodiscard]] bool covered() const { return !children().empty(); }

        void arrange(Typeface &type) override;
    };
}


#endif //TTK_DIALOGS_DIALOGLAYER_H
