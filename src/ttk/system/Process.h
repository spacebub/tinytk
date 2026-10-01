// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_SYSTEM_PROCESS_H
#define TTK_SYSTEM_PROCESS_H


#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

//! Starting child processes, reading their output and tracking their state.
//!
//! Tracked children live in a process-wide table that is not locked, so call these functions from one thread.
namespace ttk::Process {

    //! Identifier of a tracked child process. Zero means no process.
    using Id = std::uint64_t;

    //! Read end of a stream, a file descriptor on POSIX and a `HANDLE` on Windows.
    using Stream = std::intptr_t;

    //! Value of a \ref Stream that refers to nothing. Functions given it do nothing or return false.
    constexpr Stream NOTHING = -1;

    //! State of a tracked child process as reported by \ref poll().
    enum class State : std::uint8_t {
        //! The id is not tracked: it was never returned by \ref start(), or its end was already reported.
        Unknown,
        //! The child is still running. A child stopped by a signal counts as running.
        Running,
        //! The child exited with status 0.
        Finished,
        //! The child exited with a non-zero status, or was ended by a signal.
        Failed,
    };

    //! Starts `program` with `arguments` in `workingDirectory` and returns true when it was started.
    //!
    //! A `program` without a directory part is searched for on `PATH`. An empty `workingDirectory` keeps the
    //! current one. Entries of `environment` are added to the current environment, replacing variables of the
    //! same name. On POSIX the child runs in its own session, so it outlives the caller.
    //!
    //! When `id` is not null it receives an id for \ref poll(), \ref stop() and \ref force(). Without one the
    //! child is not tracked and nothing needs to reap it. When `output` is not null it receives the non-blocking
    //! read end of the child's stdout and stderr, a pseudo-terminal on POSIX so the child's stdio stays line
    //! buffered, and a pipe on Windows. That output must be drained with \ref read() and closed with
    //! \ref close_stream(), or the child blocks on a full buffer. On failure, including when the program cannot
    //! be executed, returns false and stores a message in `error` when it is not null.
    bool start(const std::filesystem::path &program,
               const std::vector<std::string> &arguments,
               const std::filesystem::path &workingDirectory,
               const std::map<std::string, std::string> &environment = {},
               Id *id = nullptr,
               Stream *output = nullptr,
               std::string *error = nullptr);

    //! Replaces `into` with whatever can be read from `output` without blocking, and returns true.
    //!
    //! Returns true with `into` empty when nothing is waiting yet. Returns false with `into` empty at the end of
    //! the stream, on an error, or when `output` is \ref NOTHING.
    bool read(Stream output, std::string &into);

    //! Asks the tracked child `id` to end, sending `SIGTERM` on POSIX so it may save on the way out.
    //!
    //! On Windows the child is terminated at once, as by \ref force(). Does nothing when `id` is not tracked.
    void stop(Id id);

    //! Ends the tracked child `id` at once, sending `SIGKILL` on POSIX. Use it for a child that ignores
    //! \ref stop(). Does nothing when `id` is not tracked.
    void force(Id id);

    //! Closes `output`, either end of a waker, or any other \ref Stream. Does nothing for \ref NOTHING.
    void close_stream(Stream output);

    //! Creates a pipe whose read end, given to \ref wait_for(), ends the wait when \ref wake() is called on its
    //! write end. Stores the ends in `readEnd` and `writeEnd` and returns true, or returns false on failure.
    //!
    //! Both ends are closed with \ref close_stream().
    bool make_waker(Stream *readEnd, Stream *writeEnd);

    //! Writes one byte to the waker end `writeEnd`. Does nothing for \ref NOTHING.
    //!
    //! The byte stays in the pipe until it is consumed with \ref read() on the read end, and \ref wait_for()
    //! returns at once until then.
    void wake(Stream writeEnd);

    //! Sleeps until `output` has something to read, `waker` has been woken through \ref wake(), or `millis`
    //! milliseconds pass. Either stream may be \ref NOTHING.
    //!
    //! On POSIX returns at once when both are \ref NOTHING. On Windows `output` is ignored, since an anonymous
    //! pipe cannot be waited on, so the call waits out `millis` unless `waker` is woken.
    void wait_for(Stream output, Stream waker, int millis);

    //! Returns the state of the tracked child `id` without blocking.
    //!
    //! A child that has ended is reaped and reported once, after which `id` is forgotten and returns
    //! \ref State::Unknown. When the end is reported and `code` is not null it receives the exit status, or on
    //! POSIX the negated signal number for a child ended by a signal.
    State poll(Id id, int *code = nullptr);
}


#endif //TTK_SYSTEM_PROCESS_H
