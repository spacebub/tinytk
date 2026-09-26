// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

#include "ttk/draw/WlShm.h"

namespace ttk {
    namespace {

        // Rows start on a cache line.
        constexpr int ALIGN = 64;

        // Past this many rectangles owed to one buffer, bringing it up whole is cheaper
        // than the list.
        constexpr size_t CROWDED = 16;

        // The ring never grows past this. With every one still with the compositor, the
        // one it has held longest is taken back rather than waiting on it.
        constexpr size_t MOST = 4;

        int stride_for(const int width) {
            return ((width * 4) + ALIGN - 1) / ALIGN * ALIGN;
        }

        // Rows of `region` from one buffer to another of the same layout. Measured
        // against AVX2 streaming stores, memcpy is twice as fast warm, no slower cold,
        // and a narrow column through streaming stores is a hundred times slower.
        void copy_rows(uint8_t *to, const uint8_t *from, const size_t stride, const int width,
                       const BLRectI &region) {
            const size_t top = static_cast<size_t>(region.y) * stride;

            if (region.x == 0 && region.w == width) {
                std::memcpy(to + top, from + top, stride * static_cast<size_t>(region.h));

                return;
            }

            const size_t wide = static_cast<size_t>(region.w) * 4;
            const size_t at = top + (static_cast<size_t>(region.x) * 4);

            for (int row = 0; row < region.h; ++row) {
                const size_t offset = at + (static_cast<size_t>(row) * stride);

                std::memcpy(to + offset, from + offset, wide);
            }
        }

        struct Buffer {
            wl_buffer *buffer = nullptr;
            uint8_t *pixels = nullptr;
            size_t bytes = 0;

            int width = 0;
            int height = 0;
            int stride = 0;

            // With the compositor, until it says otherwise.
            bool busy = false;

            // The wrong size now. Freed once the compositor lets go of it.
            bool retired = false;

            // What of the frames presented since this one was last drawn into it has
            // not seen, or all of it.
            std::vector<BLRectI> owed;
            bool owedAll = true;

            // The frame it was last presented in, to pick the least behind.
            uint64_t shown = 0;
        };

    }

    struct WlShm::Impl {
        static void on_global(void *held, wl_registry *registry, uint32_t name,
                              const char *interface, uint32_t version);
        static void on_global_gone(void *held, wl_registry *registry, uint32_t name);
        static void on_release(void *held, wl_buffer *buffer);

        // Picks up what the compositor has sent our queue without waiting for more.
        void poll() const;

        // Frees the retired buffers the compositor has let go of.
        void sweep();

        [[nodiscard]] std::unique_ptr<Buffer> make(int width, int height) const;

        void destroy(Buffer &buffer);

        wl_display *display = nullptr;
        wl_surface *surface = nullptr;
        wl_event_queue *queue = nullptr;
        wl_registry *registry = nullptr;
        wl_shm *shm = nullptr;

        std::vector<std::unique_ptr<Buffer>> buffers;

        // Drawn into, not yet presented.
        Buffer *held = nullptr;

        // Presented last, so the one holding the frame on screen.
        Buffer *last = nullptr;

        uint64_t frame = 0;
    };

    namespace {
        constexpr wl_registry_listener REGISTRY_LISTENER = {
            .global = WlShm::Impl::on_global,
            .global_remove = WlShm::Impl::on_global_gone,
        };

        constexpr wl_buffer_listener BUFFER_LISTENER = {
            WlShm::Impl::on_release,
        };

    }

    void WlShm::Impl::on_global(void *held, wl_registry *registry, const uint32_t name,
                                const char *interface, uint32_t /*unused*/) {
        auto *impl = static_cast<Impl *>(held);

        if (impl->shm == nullptr && std::strcmp(interface, wl_shm_interface.name) == 0) {
            impl->shm = static_cast<wl_shm *>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
        }
    }

    void WlShm::Impl::on_global_gone(void * /*unused*/, wl_registry * /*unused*/,
                                     uint32_t /*unused*/) {}

    void WlShm::Impl::on_release(void *held, wl_buffer * /*unused*/) {
        static_cast<Buffer *>(held)->busy = false;
    }

    void WlShm::Impl::poll() const {
        while (wl_display_prepare_read_queue(display, queue) != 0) {
            wl_display_dispatch_queue_pending(display, queue);
        }

        pollfd waiting{.fd = wl_display_get_fd(display), .events = POLLIN, .revents = 0};

        if (::poll(&waiting, 1, 0) > 0 && (waiting.revents & POLLIN) != 0) {
            wl_display_read_events(display);
        } else {
            wl_display_cancel_read(display);
        }

        wl_display_dispatch_queue_pending(display, queue);
    }

    std::unique_ptr<Buffer> WlShm::Impl::make(const int width, const int height) const {
        auto made = std::make_unique<Buffer>();

        made->width = width;
        made->height = height;
        made->stride = stride_for(width);
        made->bytes = static_cast<size_t>(made->stride) * static_cast<size_t>(height);

        if (made->bytes > static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
            return nullptr;
        }

        const int fd = memfd_create("ttk-frame", MFD_CLOEXEC | MFD_ALLOW_SEALING);

        if (fd < 0) {
            return nullptr;
        }

        if (ftruncate(fd, static_cast<off_t>(made->bytes)) != 0) {
            ::close(fd);

            return nullptr;
        }

        // The compositor maps it too, and this is what lets it do so without guarding
        // against the file shrinking under it.
        fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_SEAL);

        // Populated up front: the first frame touches every page anyway, and a fault
        // per page inside the rasteriser is the slowest way to do that.
        void *mapped = mmap(nullptr, made->bytes, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
                            fd, 0);

        if (mapped == MAP_FAILED) {
            ::close(fd);

            return nullptr;
        }

        made->pixels = static_cast<uint8_t *>(mapped);

        wl_shm_pool *pool = wl_shm_create_pool(shm, fd, static_cast<int32_t>(made->bytes));

        // The request carries a duplicate.
        ::close(fd);

        if (pool == nullptr) {
            munmap(made->pixels, made->bytes);

            return nullptr;
        }

        made->buffer = wl_shm_pool_create_buffer(pool, 0, width, height, made->stride,
                                                 WL_SHM_FORMAT_XRGB8888);

        wl_shm_pool_destroy(pool);

        if (made->buffer == nullptr) {
            munmap(made->pixels, made->bytes);

            return nullptr;
        }

        wl_buffer_add_listener(made->buffer, &BUFFER_LISTENER, made.get());

        return made;
    }

    void WlShm::Impl::destroy(Buffer &buffer) {
        if (&buffer == held) {
            held = nullptr;
        }

        if (&buffer == last) {
            last = nullptr;
        }

        if (buffer.buffer != nullptr) {
            wl_buffer_destroy(buffer.buffer);
        }

        if (buffer.pixels != nullptr) {
            munmap(buffer.pixels, buffer.bytes);
        }

        buffer.buffer = nullptr;
        buffer.pixels = nullptr;
    }

    void WlShm::Impl::sweep() {
        std::erase_if(buffers, [this](const std::unique_ptr<Buffer> &buffer) {
            if (!buffer->retired || buffer->busy) {
                return false;
            }

            destroy(*buffer);

            return true;
        });
    }

    WlShm::WlShm() : _impl(std::make_unique<Impl>()) {}

    WlShm::~WlShm() {
        close();
    }

    bool WlShm::available(SDL_Window *window) {
        const char *driver = SDL_GetCurrentVideoDriver();

        if (window == nullptr || driver == nullptr || SDL_strcmp(driver, "wayland") != 0) {
            return false;
        }

        const SDL_PropertiesID props = SDL_GetWindowProperties(window);

        return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr) != nullptr
            && SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr) != nullptr;
    }

    bool WlShm::open(SDL_Window *window) {
        close();

        Impl &impl = *_impl;
        const SDL_PropertiesID props = SDL_GetWindowProperties(window);

        impl.display = static_cast<wl_display *>(
            SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr));
        impl.surface = static_cast<wl_surface *>(
            SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr));

        if (impl.display == nullptr || impl.surface == nullptr) {
            return false;
        }

        // A queue of our own: SDL dispatches the default one, and nothing of ours
        // should turn up inside its handlers or wait on its loop.
        impl.queue = wl_display_create_queue(impl.display);

        if (impl.queue == nullptr) {
            close();

            return false;
        }

        auto *wrapped = static_cast<wl_display *>(wl_proxy_create_wrapper(impl.display));

        wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(wrapped), impl.queue);

        impl.registry = wl_display_get_registry(wrapped);

        wl_proxy_wrapper_destroy(wrapped);

        wl_registry_add_listener(impl.registry, &REGISTRY_LISTENER, &impl);
        wl_display_roundtrip_queue(impl.display, impl.queue);

        if (impl.shm == nullptr) {
            close();

            return false;
        }

        return true;
    }

    void WlShm::close() {
        Impl &impl = *_impl;

        for (const std::unique_ptr<Buffer> &buffer : impl.buffers) {
            impl.destroy(*buffer);
        }

        impl.buffers.clear();
        impl.held = nullptr;
        impl.last = nullptr;

        if (impl.shm != nullptr) {
            wl_shm_destroy(impl.shm);
            impl.shm = nullptr;
        }

        if (impl.registry != nullptr) {
            wl_registry_destroy(impl.registry);
            impl.registry = nullptr;
        }

        if (impl.queue != nullptr) {
            wl_event_queue_destroy(impl.queue);
            impl.queue = nullptr;
        }

        impl.display = nullptr;
        impl.surface = nullptr;
    }

    bool WlShm::opened() {
        return _impl->shm != nullptr;
    }

    bool WlShm::acquire(const int width, const int height, const bool whole, Target &target) {
        Impl &impl = *_impl;

        if (impl.shm == nullptr || width <= 0 || height <= 0) {
            return false;
        }

        impl.poll();

        if (impl.held != nullptr && impl.held->width == width && impl.held->height == height) {
            target = {.pixels = impl.held->pixels, .stride = impl.held->stride, .fresh = false};

            return true;
        }

        impl.held = nullptr;

        for (const std::unique_ptr<Buffer> &buffer : impl.buffers) {
            if (buffer->width != width || buffer->height != height) {
                buffer->retired = true;
            }
        }

        impl.sweep();

        // The free buffer least behind, so the least to bring up.
        Buffer *chosen = nullptr;
        Buffer *oldest = nullptr;
        size_t active = 0;

        for (const std::unique_ptr<Buffer> &buffer : impl.buffers) {
            if (buffer->retired) {
                continue;
            }

            ++active;

            if (!buffer->busy && (chosen == nullptr || buffer->shown > chosen->shown)) {
                chosen = buffer.get();
            }

            if (oldest == nullptr || buffer->shown < oldest->shown) {
                oldest = buffer.get();
            }
        }

        if (chosen == nullptr) {
            if (active < MOST) {
                std::unique_ptr<Buffer> made = impl.make(width, height);

                if (made == nullptr) {
                    return false;
                }

                chosen = made.get();

                impl.buffers.push_back(std::move(made));
            } else {
                chosen = oldest;
            }
        }

        bool fresh = false;

        if (!whole && chosen != impl.last) {
            const Buffer *last = impl.last;

            if (last == nullptr || last->retired) {
                fresh = true;
            } else if (chosen->owedAll) {
                std::memcpy(chosen->pixels, last->pixels, chosen->bytes);
            } else {
                for (const BLRectI &region : chosen->owed) {
                    copy_rows(chosen->pixels, last->pixels, static_cast<size_t>(chosen->stride), width,
                              region);
                }
            }
        }

        chosen->owed.clear();
        chosen->owedAll = false;

        impl.held = chosen;

        target = {.pixels = chosen->pixels, .stride = chosen->stride, .fresh = fresh};

        return true;
    }

    bool WlShm::present(const std::span<const BLRectI> regions) {
        Impl &impl = *_impl;
        Buffer *held = impl.held;

        if (held == nullptr) {
            return false;
        }

        wl_surface_attach(impl.surface, held->buffer, 0, 0);

        // Damage in buffer pixels needs a compositor of version 4. Before that it is
        // in surface units, which a scaled surface makes a different thing, so the lot.
        if (wl_proxy_get_version(reinterpret_cast<wl_proxy *>(impl.surface))
            >= WL_SURFACE_DAMAGE_BUFFER_SINCE_VERSION) {
            for (const BLRectI &region : regions) {
                wl_surface_damage_buffer(impl.surface, region.x, region.y, region.w, region.h);
            }
        } else {
            wl_surface_damage(impl.surface, 0, 0, std::numeric_limits<int32_t>::max(),
                              std::numeric_limits<int32_t>::max());
        }

        wl_surface_commit(impl.surface);
        wl_display_flush(impl.display);

        held->busy = true;
        held->shown = ++impl.frame;

        const bool all = std::ranges::any_of(regions, [&](const BLRectI &region) {
            return region.x <= 0 && region.y <= 0 && region.x + region.w >= held->width
                && region.y + region.h >= held->height;
        });

        // Every other buffer is now that much further behind the screen.
        for (const std::unique_ptr<Buffer> &buffer : impl.buffers) {
            if (buffer.get() == held || buffer->retired || buffer->owedAll) {
                continue;
            }

            if (all || buffer->owed.size() + regions.size() > CROWDED) {
                buffer->owed.clear();
                buffer->owedAll = true;

                continue;
            }

            buffer->owed.insert(buffer->owed.end(), regions.begin(), regions.end());
        }

        impl.last = held;
        impl.held = nullptr;

        return true;
    }
}
