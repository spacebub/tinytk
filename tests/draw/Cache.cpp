// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cstddef>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

#include "ttk/util/Cache.h"

namespace {
namespace Cache = ttk::Cache;

struct Entry {
    std::size_t used = 0;
    int payload = 0;
};

template <typename Map>
void fill(Map &held, const std::size_t count) {
    for (std::size_t at = 0; at < count; ++at) {
        held[static_cast<int>(at)] = Entry{.used = at + 1};
    }
}

TEST(Cache, MissMakesAValueInitialisedEntry) {
    Cache::RunMap<Entry> held;
    auto [entry, fresh] = Cache::run_entry(held, {.font = 1, .at = 2, .run = "text"});

    EXPECT_TRUE(fresh);
    EXPECT_EQ(entry.used, 0U);
    EXPECT_EQ(entry.payload, 0);
    EXPECT_EQ(held.size(), 1U);
}

TEST(Cache, HitReturnsTheSameEntry) {
    Cache::RunMap<Entry> held;
    Entry &made = Cache::run_entry(held, {.font = 1, .at = 2, .run = "text"}).first;

    made.payload = 42;

    auto [found, fresh] = Cache::run_entry(held, {.font = 1, .at = 2, .run = "text"});

    EXPECT_FALSE(fresh);
    EXPECT_EQ(&found, &made);
    EXPECT_EQ(found.payload, 42);
    EXPECT_EQ(held.size(), 1U);
}

TEST(Cache, EveryFieldTellsEntriesApart) {
    Cache::RunMap<Entry> held;

    EXPECT_TRUE(Cache::run_entry(held, {.font = 1, .at = 0, .run = "a"}).second);
    EXPECT_TRUE(Cache::run_entry(held, {.font = 2, .at = 0, .run = "a"}).second);
    EXPECT_TRUE(Cache::run_entry(held, {.font = 1, .at = 1, .run = "a"}).second);
    EXPECT_TRUE(Cache::run_entry(held, {.font = 1, .at = 0, .run = "b"}).second);
    EXPECT_TRUE(Cache::run_entry(held, {.font = 1, .at = 0, .run = ""}).second);
    EXPECT_FALSE(Cache::run_entry(held, {.font = 1, .at = 0, .run = ""}).second);
    EXPECT_EQ(held.size(), 5U);
}

TEST(Cache, KeyOwnsACopyOfTheRun) {
    Cache::RunMap<Entry> held;

    {
        std::string text = "transient";

        Cache::run_entry(held, {.font = 3, .run = text}).first.payload = 7;
        text.assign("overwritten");
    }

    const std::string again = "transient";
    auto [found, fresh] = Cache::run_entry(held, {.font = 3, .run = again});

    EXPECT_FALSE(fresh) << "the key did not keep its own text";
    EXPECT_EQ(found.payload, 7);
    EXPECT_EQ(held.begin()->first.run, "transient");
}

TEST(Cache, ZeroKeepNeverEvicts) {
    Cache::RunMap<Entry> held;

    for (int at = 0; at < 1000; ++at) {
        Cache::run_entry(held, {.at = at, .run = "run"}).first.used = static_cast<std::size_t>(at);
    }

    EXPECT_EQ(held.size(), 1000U);
}

TEST(Cache, MissesStayWithinTheKeepBound) {
    Cache::RunMap<Entry> held;
    constexpr std::size_t keep = 16;

    for (int at = 0; at < 500; ++at) {
        Cache::run_entry(held, {.at = at, .run = "run"}, keep).first.used = static_cast<std::size_t>(at) + 1;

        ASSERT_LE(held.size(), keep) << "after " << at + 1 << " misses";
    }
}

TEST(Cache, MissAtCapacityDropsTheColdestAndKeepsTheNewest) {
    Cache::RunMap<Entry> held;
    constexpr std::size_t keep = 8;

    for (int at = 0; at < 8; ++at) {
        Cache::run_entry(held, {.at = at, .run = "run"}).first.used = static_cast<std::size_t>(at) + 1;
    }

    Cache::run_entry(held, {.at = 0, .run = "run"}).first.used = 100;
    Cache::run_entry(held, {.at = 99, .run = "run"}, keep);

    EXPECT_TRUE(held.contains(Cache::RunView{.at = 0, .run = "run"})) << "a recently used entry was evicted";
    EXPECT_TRUE(held.contains(Cache::RunView{.at = 7, .run = "run"}));
    EXPECT_TRUE(held.contains(Cache::RunView{.at = 99, .run = "run"}));
    EXPECT_FALSE(held.contains(Cache::RunView{.at = 1, .run = "run"})) << "the coldest entry survived";
}

TEST(Cache, MissBelowCapacityEvictsNothing) {
    Cache::RunMap<Entry> held;

    for (int at = 0; at < 7; ++at) {
        Cache::run_entry(held, {.at = at, .run = "run"}, 8).first.used = static_cast<std::size_t>(at) + 1;
    }

    EXPECT_EQ(held.size(), 7U);

    Cache::run_entry(held, {.at = 7, .run = "run"}, 8);

    EXPECT_EQ(held.size(), 8U);
}

TEST(Cache, EntryReferencesSurviveGrowth) {
    Cache::RunMap<Entry> held;
    Entry &first = Cache::run_entry(held, {.font = 9, .run = "first"}).first;

    first.payload = 5;

    for (int at = 0; at < 4000; ++at) {
        Cache::run_entry(held, {.at = at, .run = "filler"});
    }

    EXPECT_EQ(&Cache::run_entry(held, {.font = 9, .run = "first"}).first, &first);
    EXPECT_EQ(first.payload, 5);
}

TEST(Cache, EvictOldestDoesNothingBelowKeep) {
    std::unordered_map<int, Entry> held;
    int dropped = 0;

    fill(held, 7);
    Cache::evict_oldest(held, 8, [&](const Entry &) { ++dropped; });

    EXPECT_EQ(held.size(), 7U);
    EXPECT_EQ(dropped, 0);
}

TEST(Cache, EvictOldestRemovesTheColdestQuarter) {
    std::unordered_map<int, Entry> held;

    fill(held, 100);
    Cache::evict_oldest(held, 100);

    EXPECT_GE(held.size(), 74U);
    EXPECT_LE(held.size(), 75U);

    for (const auto &[key, entry] : held) {
        EXPECT_GT(entry.used, 25U) << "a cold entry was kept";
    }

    EXPECT_TRUE(held.contains(99));
}

TEST(Cache, EvictOldestRemovesMoreOnTies) {
    std::unordered_map<int, Entry> held;

    for (int at = 0; at < 12; ++at) {
        held[at] = Entry{.used = at < 6 ? 1U : 2U};
    }

    Cache::evict_oldest(held, 12);

    EXPECT_EQ(held.size(), 6U) << "every entry tied at the quarter mark goes";

    for (int at = 0; at < 12; ++at) {
        held[at] = Entry{.used = 3};
    }

    Cache::evict_oldest(held, 12);

    EXPECT_TRUE(held.empty()) << "entries all tied did not all go";
}

TEST(Cache, EvictOldestTellsEachDroppedValue) {
    std::unordered_map<int, Entry> held;
    std::vector<std::size_t> told;

    fill(held, 40);
    Cache::evict_oldest(held, 40, [&](const Entry &gone) { told.push_back(gone.used); });

    EXPECT_EQ(told.size(), 40U - held.size());

    for (const std::size_t used : told) {
        EXPECT_LE(used, 11U);
    }
}

TEST(Cache, EvictOldestWorksOnOrderedMaps) {
    std::map<int, Entry> held;

    fill(held, 20);
    Cache::evict_oldest(held, 10);

    EXPECT_LT(held.size(), 20U);
    EXPECT_FALSE(held.contains(0));
    EXPECT_TRUE(held.contains(19));
}

TEST(Cache, HashOfAKeyEqualsHashOfItsView) {
    const Cache::RunHash hash;

    for (const std::string &run : {
                 std::string(),
                 std::string("short"),
                 std::string(64, 'x'),
                 std::string(65, 'y'),
                 std::string(10000, 'z') + "tail",
         }) {
        const Cache::RunKey key{.font = 0xdead, .at = -7, .run = run};
        const Cache::RunView view{.font = 0xdead, .at = -7, .run = run};

        EXPECT_EQ(hash(key), hash(view)) << "run of " << run.size() << " bytes";
    }
}

TEST(Cache, HashTellsCommonDifferencesApart) {
    const Cache::RunHash hash;
    const Cache::RunView base{.font = 1, .at = 2, .run = "label"};

    EXPECT_NE(hash(base), hash(Cache::RunView{.font = 2, .at = 2, .run = "label"}));
    EXPECT_NE(hash(base), hash(Cache::RunView{.font = 1, .at = 3, .run = "label"}));
    EXPECT_NE(hash(base), hash(Cache::RunView{.font = 1, .at = 2, .run = "labels"}));
}

TEST(Cache, LongRunsHashTheirEndsAndLength) {
    const Cache::RunHash hash;
    const std::string one(1000, 'a');
    std::string two = one;

    two[500] = 'b';

    EXPECT_EQ(hash(Cache::RunView{.run = one}), hash(Cache::RunView{.run = two}))
            << "the middle of a long run was hashed";

    std::string tail = one;

    tail.back() = 'c';

    EXPECT_NE(hash(Cache::RunView{.run = one}), hash(Cache::RunView{.run = tail})) << "the end was not hashed";
    EXPECT_NE(hash(Cache::RunView{.run = one}), hash(Cache::RunView{.run = one + "a"})) << "the length was not hashed";
}

TEST(Cache, RunsOfTheSameEndsStayApart) {
    Cache::RunMap<Entry> held;
    const std::string one(1000, 'a');
    std::string two = one;

    two[500] = 'b';

    Cache::run_entry(held, {.run = one}).first.payload = 1;
    Cache::run_entry(held, {.run = two}).first.payload = 2;

    EXPECT_EQ(held.size(), 2U);
    EXPECT_EQ(Cache::run_entry(held, {.run = one}).first.payload, 1);
    EXPECT_EQ(Cache::run_entry(held, {.run = two}).first.payload, 2);
}

TEST(Cache, EqualityComparesEveryField) {
    const Cache::RunEqual equal;
    const Cache::RunKey key{.font = 4, .at = 5, .run = "word"};

    EXPECT_TRUE(equal(key, Cache::RunKey{.font = 4, .at = 5, .run = "word"}));
    EXPECT_TRUE(equal(key, Cache::RunView{.font = 4, .at = 5, .run = "word"}));
    EXPECT_TRUE(equal(Cache::RunView{.font = 4, .at = 5, .run = "word"}, key));
    EXPECT_FALSE(equal(key, Cache::RunView{.font = 5, .at = 5, .run = "word"}));
    EXPECT_FALSE(equal(key, Cache::RunView{.font = 4, .at = 6, .run = "word"}));
    EXPECT_FALSE(equal(key, Cache::RunView{.font = 4, .at = 5, .run = "words"}));
    EXPECT_FALSE(equal(Cache::RunView{.font = 4, .at = 5, .run = "Word"}, key));
    EXPECT_FALSE(equal(key, Cache::RunKey{.font = 4, .at = 5, .run = "wor"}));
}

TEST(Cache, MapIsFoundByViewOrKey) {
    Cache::RunMap<int> held;

    held.emplace(Cache::RunKey{.font = 1, .at = 1, .run = "found"}, 11);

    const std::string text = "found";

    EXPECT_NE(held.find(Cache::RunView{.font = 1, .at = 1, .run = text}), held.end());
    EXPECT_NE(held.find(Cache::RunKey{.font = 1, .at = 1, .run = text}), held.end());
    EXPECT_EQ(held.find(Cache::RunView{.font = 1, .at = 1, .run = "missing"}), held.end());
}

TEST(Cache, ViewOfPartOfABufferFindsTheWholeKey) {
    Cache::RunMap<int> held;
    const std::string buffer = "prefix-needle-suffix";

    held.emplace(Cache::RunKey{.run = "needle"}, 3);

    const auto found = held.find(Cache::RunView{.run = std::string_view(buffer).substr(7, 6)});

    ASSERT_NE(found, held.end());
    EXPECT_EQ(found->second, 3);
}

TEST(Cache, Utf8RunsAreKeyedByteForByte) {
    Cache::RunMap<Entry> held;

    EXPECT_TRUE(Cache::run_entry(held, {.run = "\xc3\xa9"}).second);
    EXPECT_TRUE(Cache::run_entry(held, {.run = "e\xcc\x81"}).second) << "composed and decomposed are bytes apart";
    EXPECT_FALSE(Cache::run_entry(held, {.run = "\xc3\xa9"}).second);
}
}
