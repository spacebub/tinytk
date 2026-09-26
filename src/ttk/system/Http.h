// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_SYSTEM_HTTP_H
#define TTK_SYSTEM_HTTP_H


#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>

namespace ttk::Http {

    // curl_global_init is not thread safe, so it runs before any fetch.
    void start();

    void stop();

    // What requests say they are. The application sets its own name and version.
    void set_agent(const std::string &agent);

    // Runs on its own thread. Poll done().
    class Fetch {
    public:
        // Into a file, or in memory when into is empty. A non empty accept becomes the
        // Accept header.
        Fetch(std::string url, std::string accept, std::filesystem::path into);

        ~Fetch();

        Fetch(const Fetch &) = delete;
        Fetch &operator=(const Fetch &) = delete;
        Fetch(Fetch &&) = delete;
        Fetch &operator=(Fetch &&) = delete;

        [[nodiscard]] bool done() const { return _done.load(); }

        void cancel() { _cancelled.store(true); }

        [[nodiscard]] bool cancelled() const { return _cancelled.load(); }

        // 0 to 1. Stays 0 without a Content-Length.
        [[nodiscard]] double progress() const { return _progress.load(); }

        [[nodiscard]] int status() const { return _status.load(); }

        // Empty when it worked.
        [[nodiscard]] std::string error() const;

        [[nodiscard]] std::string body() const;

    private:
        void work();

        static constexpr long STALL_SECONDS = 30;

        std::string _url;
        std::string _accept;
        std::filesystem::path _into;

        std::atomic<bool> _done{false};
        std::atomic<bool> _cancelled{false};
        std::atomic<double> _progress{0};
        std::atomic<int> _status{0};

        mutable std::mutex _guard;
        std::string _error;
        std::string _body;

        std::thread _thread;
    };

}


#endif //TTK_SYSTEM_HTTP_H
