// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

#include <wayland-client.h>

#include <gtest/gtest.h>

#include "draw/Compositor.h"
#include "ttk/draw/WlShm.h"

namespace {
using Target = ttk::WlShm::Target;

constexpr int WIDE = 64;
constexpr int TALL = 48;

// What acquire() waits for a release, from WlShm.cpp, less the millisecond its clock can drop.
constexpr auto PATIENCE = std::chrono::milliseconds(15);

uint32_t *row(const Target &target, const int y) {
    return reinterpret_cast<uint32_t *>(static_cast<uint8_t *>(target.pixels)
                                        + (static_cast<size_t>(y) * static_cast<size_t>(target.stride)));
}

void paint(const Target &target, const BLRectI &region, const uint32_t value) {
    for (int y = region.y; y < region.y + region.h; ++y) {
        std::fill_n(row(target, y) + region.x, region.w, value);
    }
}

// The first pixel the two buffers disagree on, or -1.
long differ(const Target &one, const Target &two) {
    for (int y = 0; y < TALL; ++y) {
        for (int x = 0; x < WIDE; ++x) {
            if (row(one, y)[x] != row(two, y)[x]) {
                return (static_cast<long>(y) * WIDE) + x;
            }
        }
    }

    return -1;
}

bool blank(const Target &target) {
    for (int y = 0; y < TALL; ++y) {
        if (std::any_of(row(target, y), row(target, y) + WIDE, [](const uint32_t pixel) { return pixel != 0; })) {
            return false;
        }
    }

    return true;
}

bool same(const BLRectI &one, const BLRectI &two) {
    return one.x == two.x && one.y == two.y && one.w == two.w && one.h == two.h;
}

constexpr BLRectI WHOLE{0, 0, WIDE, TALL};

class WlShmRing : public ::testing::Test {
protected:
    [[nodiscard]] virtual uint32_t version() const { return 4; }

    void SetUp() override {
        compositor = std::make_unique<tests::Compositor>(version());
        display = wl_display_connect_to_fd(compositor->client_fd());

        ASSERT_NE(display, nullptr);

        registry = wl_display_get_registry(display);

        wl_registry_add_listener(registry, &REGISTRY_LISTENER, this);
        wl_display_roundtrip(display);

        ASSERT_NE(bound, nullptr) << "no wl_compositor was advertised";

        surface = wl_compositor_create_surface(bound);

        ASSERT_TRUE(shm.open(display, surface));
    }

    void TearDown() override {
        shm.close();

        if (surface != nullptr) {
            wl_surface_destroy(surface);
        }

        if (bound != nullptr) {
            wl_compositor_destroy(bound);
        }

        if (registry != nullptr) {
            wl_registry_destroy(registry);
        }

        if (display != nullptr) {
            wl_display_flush(display);
        }

        compositor.reset();

        if (display != nullptr) {
            wl_display_disconnect(display);
        }
    }

    // Acquires a buffer, paints `value` over `region` of it and presents that region, waiting for the
    // compositor to see the commit.
    bool frame(Target &target, const BLRectI &region, const uint32_t value, const bool whole = false) {
        if (!shm.acquire(WIDE, TALL, whole, target)) {
            ADD_FAILURE() << "acquire failed";

            return false;
        }

        paint(target, region, value);

        const std::vector<BLRectI> regions{region};

        if (!shm.present(regions)) {
            ADD_FAILURE() << "present failed";

            return false;
        }

        ++presented;

        if (!compositor->await_commits(presented)) {
            ADD_FAILURE() << "the compositor never saw commit " << presented;

            return false;
        }

        return true;
    }

    static void
    on_global(void *data, wl_registry *registry, const uint32_t name, const char *interface, const uint32_t version) {
        auto *self = static_cast<WlShmRing *>(data);

        if (std::strcmp(interface, wl_compositor_interface.name) == 0) {
            self->bound = static_cast<wl_compositor *>(
                    wl_registry_bind(registry, name, &wl_compositor_interface, std::min(version, 4U)));
        }
    }

    static void on_global_gone(void * /*unused*/, wl_registry * /*unused*/, uint32_t /*unused*/) {}

    static constexpr wl_registry_listener REGISTRY_LISTENER = {
            .global = on_global,
            .global_remove = on_global_gone,
    };

    std::unique_ptr<tests::Compositor> compositor;
    wl_display *display = nullptr;
    wl_registry *registry = nullptr;
    wl_compositor *bound = nullptr;
    wl_surface *surface = nullptr;
    ttk::WlShm shm;
    size_t presented = 0;
};

class WlShmOldRing : public WlShmRing {
protected:
    [[nodiscard]] uint32_t version() const override { return 3; }
};

TEST_F(WlShmRing, OpenNeedsBothADisplayAndASurface) {
    EXPECT_FALSE(shm.open(nullptr, surface));
    EXPECT_FALSE(shm.opened());
    EXPECT_FALSE(shm.open(display, nullptr));
    EXPECT_FALSE(shm.opened());
    EXPECT_TRUE(shm.open(display, surface));
    EXPECT_TRUE(shm.opened());
}

TEST_F(WlShmRing, FirstFrameIsFreshAndReachesTheCompositor) {
    Target target;

    ASSERT_TRUE(shm.acquire(WIDE, TALL, false, target));
    EXPECT_TRUE(target.fresh) << "nothing is on screen to bring the first buffer up to";
    ASSERT_NE(target.pixels, nullptr);
    EXPECT_EQ(target.stride % 64, 0);
    EXPECT_GE(target.stride, WIDE * 4);

    paint(target, WHOLE, 0x00112233);

    ASSERT_TRUE(shm.present(std::vector{WHOLE}));
    ASSERT_TRUE(compositor->await_commits(1));

    const std::vector<tests::Commit> seen = compositor->commits();

    ASSERT_EQ(seen.size(), 1U);
    EXPECT_NE(seen[0].buffer, nullptr);
    EXPECT_EQ(seen[0].width, WIDE);
    EXPECT_EQ(seen[0].height, TALL);
    EXPECT_EQ(seen[0].stride, target.stride);
    EXPECT_EQ(seen[0].marker, 0x00112233U) << "the compositor reads other memory than the client wrote";
}

TEST_F(WlShmRing, PresentDamagesTheRegionsGivenInBufferPixels) {
    Target target;

    ASSERT_TRUE(shm.acquire(WIDE, TALL, true, target));

    const std::vector<BLRectI> regions{{1, 2, 3, 4}, {10, 20, 30, 5}};

    ASSERT_TRUE(shm.present(regions));
    ASSERT_TRUE(compositor->await_commits(1));

    const std::vector<tests::Commit> seen = compositor->commits();

    ASSERT_EQ(seen.size(), 1U);
    EXPECT_TRUE(seen[0].in_buffer_pixels);
    ASSERT_EQ(seen[0].damage.size(), 2U);
    EXPECT_TRUE(same(seen[0].damage[0], regions[0]));
    EXPECT_TRUE(same(seen[0].damage[1], regions[1]));
}

TEST_F(WlShmOldRing, OldCompositorGetsTheWholeSurfaceDamaged) {
    Target target;

    ASSERT_TRUE(shm.acquire(WIDE, TALL, true, target));
    ASSERT_TRUE(shm.present(std::vector{BLRectI{1, 2, 3, 4}}));
    ASSERT_TRUE(compositor->await_commits(1));

    const std::vector<tests::Commit> seen = compositor->commits();

    ASSERT_EQ(seen.size(), 1U);
    EXPECT_FALSE(seen[0].in_buffer_pixels);
    ASSERT_EQ(seen[0].damage.size(), 1U);
    EXPECT_TRUE(
            same(seen[0].damage[0], {0, 0, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max()}));
}

TEST_F(WlShmRing, HeldBufferComesBackFreshUntilPresented) {
    Target first;
    Target again;

    ASSERT_TRUE(shm.acquire(WIDE, TALL, false, first));
    ASSERT_TRUE(shm.acquire(WIDE, TALL, true, again));
    EXPECT_EQ(again.pixels, first.pixels);
    EXPECT_TRUE(again.fresh) << "a second acquire forgot the buffer shows nothing of the screen";

    ASSERT_TRUE(shm.present(std::vector{WHOLE}));

    Target next;

    ASSERT_TRUE(shm.acquire(WIDE, TALL, false, next));
    EXPECT_NE(next.pixels, first.pixels) << "the buffer the compositor holds was handed out again";
    EXPECT_FALSE(next.fresh);
}

TEST_F(WlShmRing, ReleasedBufferIsTakenBeforeANewOne) {
    Target first;

    ASSERT_TRUE(frame(first, WHOLE, 0x01010101));

    compositor->release(compositor->commits()[0].buffer);

    Target next;

    ASSERT_TRUE(shm.acquire(WIDE, TALL, false, next));
    EXPECT_EQ(next.pixels, first.pixels);
    EXPECT_FALSE(next.fresh);
    EXPECT_EQ(row(next, 0)[0], 0x01010101U) << "the buffer on screen lost its frame";
}

TEST_F(WlShmRing, BusyBufferIsLeftWithTheCompositor) {
    Target first;

    ASSERT_TRUE(frame(first, WHOLE, 0x01010101));

    Target next;

    ASSERT_TRUE(shm.acquire(WIDE, TALL, false, next));
    EXPECT_NE(next.pixels, first.pixels);
    EXPECT_FALSE(next.fresh);
    EXPECT_EQ(differ(next, first), -1) << "the new buffer was not brought up to the frame on screen";

    paint(next, {4, 4, 8, 8}, 0x02020202);

    ASSERT_TRUE(shm.present(std::vector{BLRectI{4, 4, 8, 8}}));
    ASSERT_TRUE(compositor->await_commits(2));

    const std::vector<tests::Commit> seen = compositor->commits();

    ASSERT_EQ(seen.size(), 2U);
    EXPECT_NE(seen[1].buffer, seen[0].buffer);
}

TEST_F(WlShmRing, WholeRepaintCopiesNothing) {
    Target first;

    ASSERT_TRUE(frame(first, WHOLE, 0xAAAAAAAA));

    Target next;

    ASSERT_TRUE(shm.acquire(WIDE, TALL, true, next));
    EXPECT_NE(next.pixels, first.pixels);
    EXPECT_TRUE(blank(next)) << "a frame about to be repainted whole was copied anyway";
}

TEST_F(WlShmRing, OwedRegionsBringABufferUpToDate) {
    Target one;
    Target two;
    Target three;

    ASSERT_TRUE(frame(one, WHOLE, 0x01010101));
    ASSERT_TRUE(frame(two, {8, 8, 16, 8}, 0x02020202));
    ASSERT_TRUE(frame(three, {40, 20, 10, 10}, 0x03030303));
    ASSERT_NE(two.pixels, one.pixels);
    ASSERT_NE(three.pixels, two.pixels);
    ASSERT_NE(three.pixels, one.pixels);

    const std::vector<tests::Commit> seen = compositor->commits();

    ASSERT_EQ(seen.size(), 3U);

    compositor->release(seen[0].buffer);

    Target four;

    ASSERT_TRUE(shm.acquire(WIDE, TALL, false, four));
    ASSERT_EQ(four.pixels, one.pixels) << "the released buffer was not reused";
    EXPECT_FALSE(four.fresh);
    EXPECT_EQ(differ(four, three), -1) << "two frames of partial damage were not all copied over";

    // A full-width region takes the one-memcpy path.
    paint(four, {0, 40, WIDE, 8}, 0x04040404);

    ASSERT_TRUE(shm.present(std::vector{BLRectI{0, 40, WIDE, 8}}));
    ASSERT_TRUE(compositor->await_commits(4));

    compositor->release(seen[1].buffer);

    Target five;

    ASSERT_TRUE(shm.acquire(WIDE, TALL, false, five));
    ASSERT_EQ(five.pixels, two.pixels);
    EXPECT_EQ(differ(five, four), -1) << "damage owed from before the last frame was skipped";
}

TEST_F(WlShmRing, ResizeRetiresBuffersUntilReleased) {
    Target wide;

    ASSERT_TRUE(frame(wide, WHOLE, 0x01010101));

    Target small;

    ASSERT_TRUE(shm.acquire(32, 32, false, small));
    EXPECT_NE(small.pixels, wide.pixels);
    EXPECT_TRUE(small.fresh) << "nothing of the old size can be brought over";
    ASSERT_TRUE(shm.present(std::vector{BLRectI{0, 0, 32, 32}}));
    ASSERT_TRUE(compositor->await_commits(2));
    EXPECT_EQ(compositor->destroyed(), 0U) << "a buffer the compositor holds was destroyed";

    compositor->release(compositor->commits()[0].buffer);

    Target next;

    ASSERT_TRUE(shm.acquire(32, 32, false, next));
    ASSERT_TRUE(shm.present(std::vector{BLRectI{0, 0, 32, 32}}));
    ASSERT_TRUE(compositor->await_commits(3));
    EXPECT_TRUE(compositor->await_destroyed(1)) << "the released buffer of the old size was kept";
}

TEST_F(WlShmRing, AllBusyWaitsThenTakesBackTheOldest) {
    Target ring[4];

    for (uint32_t at = 0; at < 4; ++at) {
        ASSERT_TRUE(frame(ring[at], WHOLE, 0x10101010 * (at + 1)));
    }

    const auto started = std::chrono::steady_clock::now();
    Target taken;

    ASSERT_TRUE(shm.acquire(WIDE, TALL, false, taken));

    EXPECT_GE(std::chrono::steady_clock::now() - started, PATIENCE) << "no wait for a release";
    EXPECT_EQ(taken.pixels, ring[0].pixels) << "not the buffer presented longest ago";
    EXPECT_FALSE(taken.fresh);
    EXPECT_EQ(differ(taken, ring[3]), -1) << "the buffer taken back was not brought up to the frame on screen";

    ASSERT_TRUE(shm.present(std::vector{WHOLE}));
    ASSERT_TRUE(compositor->await_commits(5));

    const std::vector<tests::Commit> seen = compositor->commits();

    EXPECT_NE(seen[4].buffer, seen[0].buffer) << "the buffer went back under the wl_buffer the compositor holds";
    EXPECT_TRUE(compositor->await_destroyed(1)) << "the old wl_buffer was kept";
}

TEST_F(WlShmRing, ReleaseOfATakenBackBufferDoesNotFreeIt) {
    Target ring[4];

    for (uint32_t at = 0; at < 4; ++at) {
        ASSERT_TRUE(frame(ring[at], WHOLE, 0x10101010 * (at + 1)));
    }

    Target taken;

    ASSERT_TRUE(shm.acquire(WIDE, TALL, false, taken));
    ASSERT_EQ(taken.pixels, ring[0].pixels);

    // The compositor lets go of the first attach only now, after the client gave up on it.
    compositor->release(compositor->commits()[0].buffer);

    ASSERT_TRUE(shm.present(std::vector{WHOLE}));
    ASSERT_TRUE(compositor->await_commits(5));

    const auto started = std::chrono::steady_clock::now();
    Target next;

    ASSERT_TRUE(shm.acquire(WIDE, TALL, false, next));

    EXPECT_NE(next.pixels, ring[0].pixels) << "a late release freed the buffer the compositor holds again";
    EXPECT_EQ(next.pixels, ring[1].pixels);
    EXPECT_GE(std::chrono::steady_clock::now() - started, PATIENCE);
}

TEST_F(WlShmRing, FramesTooLargeForAPoolAreRefused) {
    Target target;

    // A width whose four bytes a pixel overflow int, then a stride and height whose product overflows int32.
    EXPECT_FALSE(shm.acquire((std::numeric_limits<int>::max() / 4) + 1, 1, true, target));
    EXPECT_FALSE(shm.acquire(1'048'576, 2'048, true, target));
    EXPECT_FALSE(shm.acquire(std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), true, target));
    EXPECT_EQ(target.pixels, nullptr);
    EXPECT_TRUE(shm.acquire(WIDE, TALL, true, target)) << "a refused size broke the ring";
}

TEST_F(WlShmRing, CloseDestroysEveryBuffer) {
    Target one;
    Target two;

    ASSERT_TRUE(frame(one, WHOLE, 0x01010101));
    ASSERT_TRUE(frame(two, WHOLE, 0x02020202));

    shm.close();
    wl_display_flush(display);

    EXPECT_FALSE(shm.opened());
    EXPECT_TRUE(compositor->await_destroyed(2)) << "a buffer outlived close()";
}
}
