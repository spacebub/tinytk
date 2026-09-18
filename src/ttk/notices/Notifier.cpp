// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>

#include "ttk/notices/Notifier.h"

namespace ttk {
    void Notifier::info(const std::string &text, const std::string &title) {
        post(Severity::Info, text, title);
    }

    void Notifier::success(const std::string &text, const std::string &title) {
        post(Severity::Success, text, title);
    }

    void Notifier::warning(const std::string &text, const std::string &title) {
        post(Severity::Warning, text, title);
    }

    void Notifier::error(const std::string &text, const std::string &title) {
        post(Severity::Error, text, title);
    }

    void Notifier::post(const Severity severity, const std::string &text, const std::string &title) {
        while (_messages.size() >= LIMIT) {
            _messages.erase(_messages.begin());
        }

        _messages.push_back(Notice{
            .id = _next++,
            .severity = severity,
            .title = title,
            .body = text,
            .duration = severity == Severity::Error
                ? 0
                : 3200 + (static_cast<int>(std::min<size_t>(text.length(), 160)) * 18),
        });

        if (changed) {
            changed();
        }
    }

    void Notifier::dismiss(const int id) {
        const auto found = std::ranges::find_if(_messages, [id](const Notice &each) {
            return each.id == id;
        });

        if (found != _messages.end()) {
            _messages.erase(found);

            if (changed) {
            changed();
        }
        }
    }
}
