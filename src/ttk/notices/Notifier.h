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
    //! List of the notices currently posted, oldest first, kept for a \ref Toasts stack to show.
    //!
    //! At most four notices are kept, and posting another drops the oldest. A notifier holds no lock, so use it
    //! from one thread or guard it.
    class Notifier {
    public:
        //! Posts `text` with an optional `title` as a \ref Severity::Info notice.
        void info(const std::string &text, const std::string &title = {});

        //! Posts `text` with an optional `title` as a \ref Severity::Success notice.
        void success(const std::string &text, const std::string &title = {});

        //! Posts `text` with an optional `title` as a \ref Severity::Warning notice.
        void warning(const std::string &text, const std::string &title = {});

        //! Posts `text` with an optional `title` as a \ref Severity::Error notice, which stays until it is
        //! dismissed.
        void error(const std::string &text, const std::string &title = {});

        //! Appends a notice of `severity` with the body `text` and an optional `title`, then calls \ref changed.
        //!
        //! The notice gets the next id. An error gets no duration and stays until it is dismissed. Any other
        //! severity counts down 3.2 seconds plus 18 milliseconds for each byte of `text`, counting at most 160.
        void post(Severity severity, const std::string &text, const std::string &title = {});

        //! Removes the notice `id` and calls \ref changed. Does nothing when no notice has that id.
        void dismiss(int id);

        //! Called after every change to \ref messages(), on the thread that made it, so the interface can pass the
        //! new list to \ref Toasts::set_messages().
        std::function<void()> changed;

        //! Returns the notices currently posted, oldest first.
        [[nodiscard]] const std::vector<Notice> &messages() const { return _messages; }

    private:
        static constexpr size_t LIMIT = 4;

        std::vector<Notice> _messages;
        int _next{0};
    };
}


#endif //TTK_NOTICES_NOTIFIER_H
