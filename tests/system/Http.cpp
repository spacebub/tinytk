// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <thread>

#include <gtest/gtest.h>

#include "support/Sandbox.h"
#include "ttk/system/Http.h"

namespace {
    namespace fs = std::filesystem;

    using Clock = std::chrono::steady_clock;

    constexpr std::chrono::seconds PATIENCE{2};

    std::string slurp(const fs::path &path) {
        std::ifstream file(path, std::ios::binary);

        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }

    void spill(const fs::path &path, const std::string_view text) {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);

        file.write(text.data(), static_cast<std::streamsize>(text.size()));
    }

    std::string url_of(const fs::path &path) {
        return "file://" + path.string();
    }

    bool settle(const ttk::Http::Fetch &fetch) {
        const Clock::time_point deadline = Clock::now() + PATIENCE;

        while (!fetch.done() && Clock::now() < deadline) {
            std::this_thread::yield();
        }

        return fetch.done();
    }

    // Only file:// and malformed addresses are fetched, so nothing leaves the machine.
    class HttpFetch : public ::testing::Test {
    protected:
        static void SetUpTestSuite() { ttk::Http::start(); }

        static void TearDownTestSuite() { ttk::Http::stop(); }
    };
}

TEST_F(HttpFetch, KeepsTheBodyInMemory) {
    const fs::path source = tests::Sandbox::scratch("http-memory") / "page.txt";

    spill(source, "hello from a file");

    const ttk::Http::Fetch fetch(url_of(source), "", {});

    ASSERT_TRUE(settle(fetch));
    EXPECT_EQ(fetch.error(), "");
    EXPECT_EQ(fetch.body(), "hello from a file");
}

TEST_F(HttpFetch, KeepsBinaryBodiesIntact) {
    const fs::path source = tests::Sandbox::scratch("http-binary") / "blob.bin";
    std::string blob;

    for (int i = 0; i < 256; ++i) {
        blob.push_back(static_cast<char>(i));
    }

    spill(source, blob);

    const ttk::Http::Fetch fetch(url_of(source), "application/octet-stream", {});

    ASSERT_TRUE(settle(fetch));
    EXPECT_EQ(fetch.error(), "");
    EXPECT_EQ(fetch.body(), blob);
}

TEST_F(HttpFetch, KeepsALargeBody) {
    constexpr std::size_t SIZE = std::size_t{1} << 20U;
    const fs::path source = tests::Sandbox::scratch("http-large") / "large.txt";
    std::string text(SIZE, 'x');

    text.back() = 'y';
    spill(source, text);

    const ttk::Http::Fetch fetch(url_of(source), "", {});

    ASSERT_TRUE(settle(fetch));
    EXPECT_EQ(fetch.error(), "");
    EXPECT_EQ(fetch.body().size(), SIZE);
    EXPECT_EQ(fetch.body().back(), 'y');
}

TEST_F(HttpFetch, EmptyBody) {
    const fs::path source = tests::Sandbox::scratch("http-empty") / "empty.txt";

    spill(source, "");

    const ttk::Http::Fetch fetch(url_of(source), "", {});

    ASSERT_TRUE(settle(fetch));
    EXPECT_EQ(fetch.error(), "");
    EXPECT_EQ(fetch.body(), "");
}

TEST_F(HttpFetch, WritesTheBodyToAFile) {
    const fs::path root = tests::Sandbox::scratch("http-file");

    spill(root / "source.txt", "saved to disk");

    const ttk::Http::Fetch fetch(url_of(root / "source.txt"), "", root / "saved.txt");

    ASSERT_TRUE(settle(fetch));
    EXPECT_EQ(fetch.error(), "");
    EXPECT_EQ(fetch.body(), "") << "a body written to a file is not kept in memory";
    EXPECT_EQ(slurp(root / "saved.txt"), "saved to disk");
}

TEST_F(HttpFetch, TruncatesTheTargetFileFirst) {
    const fs::path root = tests::Sandbox::scratch("http-truncate");

    spill(root / "source.txt", "short");
    spill(root / "saved.txt", "a much longer previous content");

    const ttk::Http::Fetch fetch(url_of(root / "source.txt"), "", root / "saved.txt");

    ASSERT_TRUE(settle(fetch));
    EXPECT_EQ(slurp(root / "saved.txt"), "short");
}

TEST_F(HttpFetch, UnwritableTargetFails) {
    const fs::path root = tests::Sandbox::scratch("http-unwritable");

    spill(root / "source.txt", "x");

    const ttk::Http::Fetch fetch(url_of(root / "source.txt"), "", root / "missing" / "saved.txt");

    ASSERT_TRUE(settle(fetch));
    EXPECT_FALSE(fetch.error().empty());
    EXPECT_EQ(fetch.status(), 0);
    EXPECT_FALSE(fs::exists(root / "missing"));
}

TEST_F(HttpFetch, MissingFileFails) {
    const ttk::Http::Fetch fetch(url_of(tests::Sandbox::scratch("http-missing") / "absent.txt"), "", {});

    ASSERT_TRUE(settle(fetch));
    EXPECT_FALSE(fetch.error().empty());
    EXPECT_EQ(fetch.body(), "");
    EXPECT_EQ(fetch.status(), 0) << "no response arrived";
}

TEST_F(HttpFetch, UnsupportedSchemeFails) {
    const ttk::Http::Fetch fetch("ttk-no-such-scheme://example/x", "", {});

    ASSERT_TRUE(settle(fetch));
    EXPECT_FALSE(fetch.error().empty());
    EXPECT_EQ(fetch.status(), 0);
}

TEST_F(HttpFetch, EmptyAddressFails) {
    const ttk::Http::Fetch fetch("", "", {});

    ASSERT_TRUE(settle(fetch));
    EXPECT_FALSE(fetch.error().empty());
}

TEST_F(HttpFetch, MalformedAddressFails) {
    const ttk::Http::Fetch fetch("http://[not a host/", "", {});

    ASSERT_TRUE(settle(fetch));
    EXPECT_FALSE(fetch.error().empty());
    EXPECT_EQ(fetch.status(), 0);
}

TEST_F(HttpFetch, IsNotCancelledUntilAsked) {
    const fs::path source = tests::Sandbox::scratch("http-cancel") / "page.txt";

    spill(source, "x");

    ttk::Http::Fetch fetch(url_of(source), "", {});

    EXPECT_FALSE(fetch.cancelled());

    fetch.cancel();

    EXPECT_TRUE(fetch.cancelled());
    EXPECT_TRUE(settle(fetch)) << "a cancelled request still ends";
}

TEST_F(HttpFetch, CancelledRequestReportsStoppedOrFinished) {
    const fs::path source = tests::Sandbox::scratch("http-cancel-early") / "page.txt";

    spill(source, std::string(std::size_t{1} << 20U, 'z'));

    ttk::Http::Fetch fetch(url_of(source), "", {});

    fetch.cancel();

    ASSERT_TRUE(settle(fetch));

    const std::string error = fetch.error();

    EXPECT_TRUE(error.empty() || error == "Stopped") << "a race with completion may finish first: " << error;
}

TEST_F(HttpFetch, ProgressStaysWithinRange) {
    const fs::path source = tests::Sandbox::scratch("http-progress") / "page.txt";

    spill(source, std::string(4096, 'p'));

    const ttk::Http::Fetch fetch(url_of(source), "", {});

    ASSERT_TRUE(settle(fetch));
    EXPECT_GE(fetch.progress(), 0.0);
    EXPECT_LE(fetch.progress(), 1.0);
}

TEST_F(HttpFetch, DestroyingARunningFetchWaitsForIt) {
    const fs::path source = tests::Sandbox::scratch("http-destroy") / "page.txt";

    spill(source, "x");

    {
        const ttk::Http::Fetch fetch(url_of(source), "", {});
    }

    SUCCEED() << "the destructor cancelled and joined the thread";
}

TEST_F(HttpFetch, SetAgentBeforeFetching) {
    const fs::path source = tests::Sandbox::scratch("http-agent") / "page.txt";

    spill(source, "agent");
    ttk::Http::set_agent("tinytk-tests/1.0");

    const ttk::Http::Fetch fetch(url_of(source), "", {});

    ASSERT_TRUE(settle(fetch));
    EXPECT_EQ(fetch.body(), "agent");

    ttk::Http::set_agent("tinytk");
}
