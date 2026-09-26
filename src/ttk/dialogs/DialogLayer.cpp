// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>

#include "ttk/dialogs/DialogLayer.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Root.h"

namespace ttk {

    Dialog *DialogLayer::show(std::unique_ptr<Dialog> dialog) {
        if (Dialog *under = top(); under != nullptr) {
            under->set_visible(false);
        }

        Dialog *raw = dialog.get();

        raw->dismissed = [this] { close(); };

        add(std::move(dialog));

        if (root() != nullptr) {
            root()->relayout();
        }

        raw->opened();

        return raw;
    }

    Dialog *DialogLayer::top() const {
        return children().empty() ? nullptr : dynamic_cast<Dialog *>(children().back().get());
    }

    void DialogLayer::sync() const {
        if (Dialog *up = top(); up != nullptr) {
            up->sync();
        }
    }

    void DialogLayer::close() {
        Dialog *up = top();

        if (up == nullptr || !up->closing()) {
            return;
        }

        dismiss();
    }

    void DialogLayer::dismiss() {
        Dialog *up = top();

        if (up == nullptr) {
            return;
        }

        const BLRect was = up->box();

        if (closed) {
            closed(up);
        }

        erase(up);

        Dialog *under = top();

        if (under != nullptr) {
            under->set_visible(true);
        }

        if (root() != nullptr) {
            root()->damage(was);
        }

        if (under != nullptr) {
            under->opened();
        }
    }

    void DialogLayer::arrange(Typeface &type) {
        // Never over the title bar: the window stays draggable by it.
        const BLRect under{_box.x, _box.y + Theme::barHeight, _box.w,
                           std::max(0.0, _box.h - Theme::barHeight)};

        // A covered dialog is not measured: its card would shape all its text again.
        for (const Ptr &child : children()) {
            if (child->visible()) {
                child->place(under, type);
            }
        }
    }
}
