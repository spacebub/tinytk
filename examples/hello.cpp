// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <memory>

#include "ttk/draw/Theme.h"
#include "ttk/notices/Notifier.h"
#include "ttk/notices/Toasts.h"
#include "ttk/shell/Shell.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/controls/Button.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/layout/Box.h"

int main() {
    ttk::Shell shell;

    if (!shell.start("tinytk", 720, 520)) {
        return 1;
    }

    ttk::Notifier notifier;

    ttk::Box *page = shell.ui().content()->append(ttk::Box::column());
    page->pad(ttk::Theme::pad)->spacing(ttk::Theme::gap);

    page->append(std::make_unique<ttk::Label>("Hello from tinytk"))
        ->font(ttk::Theme::palette().headingWeight, ttk::Theme::fontTitle);

    page->append(std::make_unique<ttk::Button>("Say hello", [&notifier] {
        notifier.success("Hello", "tinytk");
    }));

    ttk::Toasts *toasts = shell.ui().layer(ttk::Root::NOTICES)->append(
        std::make_unique<ttk::Toasts>([&notifier](const int id) { notifier.dismiss(id); }));

    // Posted rather than done in place: the news can arrive from inside a toast's
    // own frame, and posting also wakes a loop that had nothing else to do.
    notifier.changed = [&] {
        shell.post([&] { toasts->set_messages(notifier.messages()); });
    };

    shell.closing = [&shell] { shell.stop(); };
    shell.run();

    return 0;
}
