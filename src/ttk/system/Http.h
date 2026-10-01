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

//! HTTP GET requests run on background threads, through libcurl or WinHTTP on Windows.
namespace ttk::Http {

    //! Initialises the HTTP backend. Call it once, before any other thread starts and before the first
    //! \ref Fetch, since libcurl's global initialisation is not thread safe. Does nothing on Windows.
    void start();

    //! Releases the HTTP backend after the last \ref Fetch is destroyed. Does nothing on Windows.
    void stop();

    //! Sets the `User-Agent` that requests send. Defaults to `tinytk`.
    //!
    //! Read by every \ref Fetch as it starts, so set it once, before the first one.
    void set_agent(const std::string &agent);

    //! A single HTTP GET request that runs on its own thread from construction. Poll \ref done() for the end.
    //!
    //! Redirects are followed. The request fails when it cannot connect within 30 seconds or receives nothing for
    //! 30 seconds. The results \ref status(), \ref error() and \ref body() are final only once \ref done()
    //! returns true.
    class Fetch {
    public:
        //! Starts fetching `url` on a new thread.
        //!
        //! The response body is written to the file `into`, which is created or truncated first, or kept in memory
        //! for \ref body() when `into` is empty. A non-empty `accept` is sent as the `Accept` header. A file left
        //! by a failed or cancelled request holds whatever had arrived.
        Fetch(std::string url, std::string accept, std::filesystem::path into);

        //! Cancels the request and waits for its thread to finish.
        ~Fetch();

        Fetch(const Fetch &) = delete;
        Fetch &operator=(const Fetch &) = delete;
        Fetch(Fetch &&) = delete;
        Fetch &operator=(Fetch &&) = delete;

        //! Tests whether the request has finished, whether it succeeded, failed or was cancelled.
        [[nodiscard]] bool done() const { return _done.load(); }

        //! Asks the request to stop. It ends soon after with \ref error() set to `Stopped`. Safe from any thread.
        void cancel() { _cancelled.store(true); }

        //! Tests whether \ref cancel() was called or the fetch is being destroyed.
        [[nodiscard]] bool cancelled() const { return _cancelled.load(); }

        //! Returns the share of the body received so far, from 0 to 1. Stays 0 when the response has no
        //! `Content-Length`.
        [[nodiscard]] double progress() const { return _progress.load(); }

        //! Returns the HTTP status code of the response, or 0 while none has arrived or when the request failed
        //! before one did.
        [[nodiscard]] int status() const { return _status.load(); }

        //! Returns a message describing why the request failed, or an empty string when it succeeded.
        //!
        //! A status of 400 or above counts as a failure. A cancelled request reports `Stopped`.
        [[nodiscard]] std::string error() const;

        //! Returns the response body received in memory. Empty while the request runs, and always empty when the
        //! body was written to a file.
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
