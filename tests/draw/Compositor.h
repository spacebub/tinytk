// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_TESTS_DRAW_COMPOSITOR_H
#define TTK_TESTS_DRAW_COMPOSITOR_H


#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include <blend2d/blend2d.h>

struct wl_client;
struct wl_display;
struct wl_event_source;
struct wl_global;
struct wl_resource;

namespace tests {
// What one wl_surface.commit carried.
struct Commit {
    wl_resource *buffer = nullptr;

    int width = 0;
    int height = 0;
    int stride = 0;

    // The first pixel of the buffer as the client left it.
    uint32_t marker = 0;

    std::vector<BLRectI> damage;

    // Whether the damage came through wl_surface.damage_buffer rather than wl_surface.damage.
    bool in_buffer_pixels = false;
};

// A compositor on a thread of its own: libwayland-server's wl_shm and a wl_compositor whose surfaces record
// every commit. A buffer is released only when a test says so.
class Compositor {
public:
    explicit Compositor(uint32_t version);
    ~Compositor();

    Compositor(const Compositor &) = delete;
    Compositor &operator=(const Compositor &) = delete;
    Compositor(Compositor &&) = delete;
    Compositor &operator=(Compositor &&) = delete;

    // The client's end of the socket, which the caller owns.
    [[nodiscard]] int client_fd() const { return _client_fd; }

    // Runs `work` on the compositor's thread and returns once everything it sent has been flushed.
    void run(const std::function<void()> &work);

    // Sends wl_buffer.release for `buffer`, unless the client has destroyed it.
    void release(wl_resource *buffer);

    // Waits up to a second for the client to have committed `count` times.
    [[nodiscard]] bool await_commits(size_t count);

    // Waits up to a second for the client to have destroyed `count` buffers.
    [[nodiscard]] bool await_destroyed(size_t count);

    [[nodiscard]] std::vector<Commit> commits();
    [[nodiscard]] size_t destroyed();

    // On the compositor's thread.
    void record(Commit commit);
    void watch(wl_resource *buffer);
    void forget(wl_resource *buffer);

private:
    static int on_wake(int fd, uint32_t mask, void *data);

    void serve();

    wl_display *_display = nullptr;
    wl_global *_global = nullptr;
    wl_client *_client = nullptr;
    wl_event_source *_source = nullptr;
    int _client_fd = -1;
    int _wake = -1;

    std::thread _thread;
    std::atomic<bool> _stopping = false;

    std::mutex _mutex;
    std::condition_variable _changed;
    std::deque<std::function<void()>> _work;
    size_t _queued = 0;
    size_t _done = 0;
    std::vector<Commit> _commits;
    std::unordered_set<wl_resource *> _live;
    size_t _destroyed = 0;
};
}


#endif //TTK_TESTS_DRAW_COMPOSITOR_H
