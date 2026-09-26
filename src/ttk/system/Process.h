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

namespace ttk::Process {

    // Zero means no process.
    using Id = std::uint64_t;

    // Read end of a child's output: a descriptor, or a handle on Windows.
    using Stream = std::intptr_t;

    constexpr Stream NOTHING = -1;

    enum class State : std::uint8_t {
        // Not started, or already reaped.
        Unknown,
        Running,
        Finished,

        // Non-zero status, or a signal.
        Failed,
    };

    // Starts the program in its own session so it outlives the caller. Without an id it is not tracked.
    // Requested output must be drained or the child blocks on a full pipe. A pty is used where
    // available so the child's stdio stays line buffered.
    bool start(const std::filesystem::path &program,
               const std::vector<std::string> &arguments,
               const std::filesystem::path &workingDirectory,
               const std::map<std::string, std::string> &environment = {},
               Id *id = nullptr,
               Stream *output = nullptr,
               std::string *error = nullptr);

    // Non-blocking. False at end of stream.
    bool read(Stream output, std::string &into);

    // Graceful. The child may save on the way out.
    void stop(Id id);

    // Immediate, for a child that ignores stop().
    void force(Id id);

    void close_stream(Stream output);

    // A stream a waiting reader can be woken through: `wake` on the write end returns
    // `wait_for` at once. Both ends are closed with close_stream.
    bool make_waker(Stream *readEnd, Stream *writeEnd);

    void wake(Stream writeEnd);

    // Sleeps until `output` has something to read, `waker` is woken, or `millis` pass.
    // Windows has nothing to wait on for an anonymous pipe, so there it waits out the time.
    void wait_for(Stream output, Stream waker, int millis);

    // Non-blocking. A finished child is reaped and reported once, Unknown after that.
    // The code is the exit status, or the negated signal.
    State poll(Id id, int *code = nullptr);

}


#endif //TTK_SYSTEM_PROCESS_H
