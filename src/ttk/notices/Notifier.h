// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_NOTICES_NOTIFIER_H
#define TTK_NOTICES_NOTIFIER_H


#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "ttk/notices/Notice.h"

namespace ttk {
    // The toasts, oldest first.
    class Notifier {
    public:
        void info(const std::string &text, const std::string &title = {});
        void success(const std::string &text, const std::string &title = {});
        void warning(const std::string &text, const std::string &title = {});
        void error(const std::string &text, const std::string &title = {});

        // Errors stay until dismissed.
        void post(Severity severity, const std::string &text, const std::string &title = {});

        void dismiss(int id);

        // Called whenever the list changes, so the interface can sync.
        std::function<void()> changed;

        [[nodiscard]] const std::vector<Notice> &messages() const { return _messages; }

    private:
        static constexpr size_t LIMIT = 4;

        std::vector<Notice> _messages;
        int _next{0};
    };
}


#endif //TTK_NOTICES_NOTIFIER_H
