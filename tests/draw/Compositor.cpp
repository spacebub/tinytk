// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <chrono>
#include <utility>

#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wayland-server.h>

#include "draw/Compositor.h"

namespace tests {
namespace {

struct Surface {
    Compositor *owner = nullptr;
    wl_resource *current = nullptr;
    wl_resource *pending = nullptr;
    bool attached = false;
    std::vector<BLRectI> damage;
    bool in_buffer_pixels = false;
};

Surface &surface_of(wl_resource *resource) {
    return *static_cast<Surface *>(wl_resource_get_user_data(resource));
}

void surface_destroy(wl_client * /*unused*/, wl_resource *resource) {
    wl_resource_destroy(resource);
}

void surface_attach(wl_client * /*unused*/, wl_resource *resource, wl_resource *buffer, int32_t /*x*/, int32_t /*y*/) {
    Surface &surface = surface_of(resource);

    surface.pending = buffer;
    surface.attached = true;

    if (buffer != nullptr) {
        surface.owner->watch(buffer);
    }
}

void surface_damage(wl_client * /*unused*/, wl_resource *resource, int32_t x, int32_t y, int32_t w, int32_t h) {
    Surface &surface = surface_of(resource);

    surface.damage.emplace_back(x, y, w, h);
    surface.in_buffer_pixels = false;
}

void surface_frame(wl_client *client, wl_resource * /*unused*/, uint32_t callback) {
    wl_resource *made = wl_resource_create(client, &wl_callback_interface, 1, callback);

    wl_callback_send_done(made, 0);
    wl_resource_destroy(made);
}

void surface_region(wl_client * /*unused*/, wl_resource * /*unused*/, wl_resource * /*unused*/) {
}

void surface_commit(wl_client * /*unused*/, wl_resource *resource) {
    Surface &surface = surface_of(resource);

    if (surface.attached) {
        surface.current = surface.pending;
        surface.attached = false;
    }

    Commit commit;

    commit.buffer = surface.current;
    commit.damage = std::move(surface.damage);
    commit.in_buffer_pixels = surface.in_buffer_pixels;

    surface.damage.clear();

    if (wl_shm_buffer *shm = surface.current != nullptr ? wl_shm_buffer_get(surface.current) : nullptr) {
        commit.width = wl_shm_buffer_get_width(shm);
        commit.height = wl_shm_buffer_get_height(shm);
        commit.stride = wl_shm_buffer_get_stride(shm);

        wl_shm_buffer_begin_access(shm);
        commit.marker = *static_cast<const uint32_t *>(wl_shm_buffer_get_data(shm));
        wl_shm_buffer_end_access(shm);
    }

    surface.owner->record(std::move(commit));
}

void surface_int(wl_client * /*unused*/, wl_resource * /*unused*/, int32_t /*unused*/) {
}

void surface_damage_buffer(wl_client * /*unused*/, wl_resource *resource, int32_t x, int32_t y, int32_t w, int32_t h) {
    Surface &surface = surface_of(resource);

    surface.damage.emplace_back(x, y, w, h);
    surface.in_buffer_pixels = true;
}

void surface_offset(wl_client * /*unused*/, wl_resource * /*unused*/, int32_t /*unused*/, int32_t /*unused*/) {
}

// Filled in by name, so requests past version 4 stay null whatever the installed protocol adds.
const struct wl_surface_interface &surface_impl() {
    static const struct wl_surface_interface impl = [] {
        struct wl_surface_interface made{};

        made.destroy = surface_destroy;
        made.attach = surface_attach;
        made.damage = surface_damage;
        made.frame = surface_frame;
        made.set_opaque_region = surface_region;
        made.set_input_region = surface_region;
        made.commit = surface_commit;
        made.set_buffer_transform = surface_int;
        made.set_buffer_scale = surface_int;
        made.damage_buffer = surface_damage_buffer;
        made.offset = surface_offset;

        return made;
    }();

    return impl;
}

void region_destroy(wl_client * /*unused*/, wl_resource *resource) {
    wl_resource_destroy(resource);
}

void region_edit(
        wl_client * /*unused*/, wl_resource * /*unused*/, int32_t /*x*/, int32_t /*y*/, int32_t /*w*/, int32_t /*h*/) {
}

const struct wl_region_interface &region_impl() {
    static const struct wl_region_interface impl = [] {
        struct wl_region_interface made{};

        made.destroy = region_destroy;
        made.add = region_edit;
        made.subtract = region_edit;

        return made;
    }();

    return impl;
}

void compositor_create_surface(wl_client *client, wl_resource *resource, uint32_t id) {
    auto *surface = new Surface;
    wl_resource *made = wl_resource_create(client, &wl_surface_interface, wl_resource_get_version(resource), id);

    surface->owner = static_cast<Compositor *>(wl_resource_get_user_data(resource));

    wl_resource_set_implementation(made, &surface_impl(), surface, [](wl_resource *gone) {
        delete static_cast<Surface *>(wl_resource_get_user_data(gone));
    });
}

void compositor_create_region(wl_client *client, wl_resource * /*unused*/, uint32_t id) {
    wl_resource *made = wl_resource_create(client, &wl_region_interface, 1, id);

    wl_resource_set_implementation(made, &region_impl(), nullptr, nullptr);
}

const struct wl_compositor_interface &compositor_impl() {
    static const struct wl_compositor_interface impl = [] {
        struct wl_compositor_interface made{};

        made.create_surface = compositor_create_surface;
        made.create_region = compositor_create_region;

        return made;
    }();

    return impl;
}

void bind_compositor(wl_client *client, void *data, const uint32_t version, const uint32_t id) {
    wl_resource *made = wl_resource_create(client, &wl_compositor_interface, static_cast<int>(version), id);

    wl_resource_set_implementation(made, &compositor_impl(), data, nullptr);
}

// One per buffer the client attached, freed when the buffer goes.
struct Watch {
    wl_listener listener{};
    Compositor *owner = nullptr;
    wl_resource *buffer = nullptr;
};

static_assert(offsetof(Watch, listener) == 0);

// NOLINTNEXTLINE(misc-const-correctness): wl_notify_func_t takes the listener mutable.
void on_buffer_gone(wl_listener *listener, void * /*unused*/) {
    const auto *watch = reinterpret_cast<const Watch *>(listener);

    watch->owner->forget(watch->buffer);

    delete watch;
}

}

Compositor::Compositor(const uint32_t version)
    : _display(wl_display_create()),
      _global(wl_global_create(_display, &wl_compositor_interface, static_cast<int>(version), this, bind_compositor)),
      _wake(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)) {
    wl_display_init_shm(_display);

    int fds[2] = {-1, -1};

    socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds);

    _client = wl_client_create(_display, fds[0]);
    _client_fd = fds[1];
    _source = wl_event_loop_add_fd(wl_display_get_event_loop(_display), _wake, WL_EVENT_READABLE, on_wake, this);
    _thread = std::thread([this] { serve(); });
}

Compositor::~Compositor() {
    run([this] { _stopping = true; });

    _thread.join();

    wl_event_source_remove(_source);
    wl_display_destroy_clients(_display);
    wl_global_destroy(_global);
    wl_display_destroy(_display);

    ::close(_wake);
}

void Compositor::serve() {
    wl_event_loop *loop = wl_display_get_event_loop(_display);

    while (!_stopping) {
        wl_event_loop_dispatch(loop, -1);
        wl_display_flush_clients(_display);
    }
}

int Compositor::on_wake(const int fd, uint32_t /*unused*/, void *data) {
    auto *self = static_cast<Compositor *>(data);
    uint64_t count = 0;
    std::deque<std::function<void()>> work;

    static_cast<void>(read(fd, &count, sizeof count));

    {
        const std::scoped_lock lock(self->_mutex);

        work.swap(self->_work);
    }

    for (const std::function<void()> &one : work) {
        one();
    }

    wl_display_flush_clients(self->_display);

    {
        const std::scoped_lock lock(self->_mutex);

        self->_done += work.size();
    }

    self->_changed.notify_all();

    return 0;
}

void Compositor::run(const std::function<void()> &work) {
    size_t wanted = 0;

    {
        const std::scoped_lock lock(_mutex);

        _work.push_back(work);
        wanted = ++_queued;
    }

    const uint64_t one = 1;

    static_cast<void>(write(_wake, &one, sizeof one));

    std::unique_lock lock(_mutex);

    _changed.wait(lock, [&] { return _done >= wanted; });
}

void Compositor::release(wl_resource *buffer) {
    run([this, buffer] {
        const std::scoped_lock lock(_mutex);

        if (_live.contains(buffer)) {
            wl_buffer_send_release(buffer);
        }
    });
}

bool Compositor::await_commits(const size_t count) {
    std::unique_lock lock(_mutex);

    return _changed.wait_for(lock, std::chrono::seconds(1), [&] { return _commits.size() >= count; });
}

bool Compositor::await_destroyed(const size_t count) {
    std::unique_lock lock(_mutex);

    return _changed.wait_for(lock, std::chrono::seconds(1), [&] { return _destroyed >= count; });
}

std::vector<Commit> Compositor::commits() {
    const std::scoped_lock lock(_mutex);

    return _commits;
}

size_t Compositor::destroyed() {
    const std::scoped_lock lock(_mutex);

    return _destroyed;
}

void Compositor::record(Commit commit) {
    {
        const std::scoped_lock lock(_mutex);

        _commits.push_back(std::move(commit));
    }

    _changed.notify_all();
}

void Compositor::watch(wl_resource *buffer) {
    {
        const std::scoped_lock lock(_mutex);

        if (!_live.insert(buffer).second) {
            return;
        }
    }

    auto *watch = new Watch;

    watch->owner = this;
    watch->buffer = buffer;
    watch->listener.notify = on_buffer_gone;

    wl_resource_add_destroy_listener(buffer, &watch->listener);
}

void Compositor::forget(wl_resource *buffer) {
    {
        const std::scoped_lock lock(_mutex);

        _live.erase(buffer);
        ++_destroyed;
    }

    _changed.notify_all();
}
}
