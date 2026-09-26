// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_UTIL_CACHE_H
#define TTK_UTIL_CACHE_H


#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ttk::Cache {

    // Only the quarter that has gone longest unasked for goes. Clearing the lot would
    // make a page holding more than the cap rebuild everything on it every frame.
    template <typename Map, typename Dropped = decltype([](const auto &) {})>
    void evict_oldest(Map &held, const size_t keep, Dropped dropped = {}) {
        if (held.size() < keep) {
            return;
        }

        std::vector<size_t> stamps;

        stamps.reserve(held.size());

        for (const auto &entry : held) {
            stamps.push_back(entry.second.used);
        }

        const size_t quarter = stamps.size() / 4;

        std::ranges::nth_element(stamps, stamps.begin() + static_cast<ptrdiff_t>(quarter));

        const size_t oldest = stamps[quarter];

        std::erase_if(held, [oldest, &dropped](const auto &entry) {
            if (entry.second.used > oldest) {
                return false;
            }

            dropped(entry.second);

            return true;
        });
    }

    // A run of text under a font, with whatever else the answer depends on packed
    // into `at`. A cache that answers for a run keys on this and is asked through
    // the view, so a hit allocates nothing.
    struct RunKey {
        std::uintptr_t font = 0;
        std::int64_t at = 0;
        std::string run;
    };

    struct RunView {
        std::uintptr_t font = 0;
        std::int64_t at = 0;
        std::string_view run;
    };

    struct RunHash {
        using is_transparent = void;

        // A paragraph is a key too, so only its ends go into the hash: its length and
        // both ends tell paragraphs apart as well as the whole would, at a cost that
        // does not grow with it. The equality still reads all of it.
        static constexpr std::size_t END = 64;

        // The fields are mixed by Knuth's multiplicative hashing: 2^64 over the
        // golden ratio spreads small and adjacent values as evenly as one multiplier can.
        std::size_t operator()(const RunView &view) const noexcept {
            constexpr auto golden = static_cast<std::size_t>(0x9e3779b97f4a7c15ULL);
            constexpr std::hash<std::string_view> bytes;

            std::size_t text = bytes(view.run.substr(0, END)) + view.run.size();

            if (view.run.size() > END) {
                const std::size_t tail = bytes(view.run.substr(view.run.size() - END));

                text ^= tail + golden + (text << 6U) + (text >> 2U);
            }

            const std::size_t rest = static_cast<std::size_t>(view.font)
                ^ (static_cast<std::size_t>(view.at) * golden);

            return text ^ (rest + golden + (text << 6U) + (text >> 2U));
        }

        std::size_t operator()(const RunKey &key) const noexcept {
            return (*this)(RunView{.font = key.font, .at = key.at, .run = key.run});
        }
    };

    struct RunEqual {
        using is_transparent = void;

        bool operator()(const RunKey &one, const RunKey &two) const noexcept {
            return one.font == two.font && one.at == two.at && one.run == two.run;
        }

        bool operator()(const RunKey &one, const RunView &two) const noexcept {
            return one.font == two.font && one.at == two.at && one.run == two.run;
        }

        bool operator()(const RunView &one, const RunKey &two) const noexcept {
            return one.font == two.font && one.at == two.at && one.run == two.run;
        }
    };

    template <typename Value>
    using RunMap = std::unordered_map<RunKey, Value, RunHash, RunEqual>;

    // The entry for `view`, made fresh when there was none, which the second answer says.
    template <typename Value>
    std::pair<Value &, bool> run_entry(RunMap<Value> &held, const RunView &view) {
        if (const auto found = held.find(view); found != held.end()) {
            return {found->second, false};
        }

        const auto made = held.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(RunKey{.font = view.font, .at = view.at, .run = std::string(view.run)}),
            std::forward_as_tuple());

        return {made.first->second, true};
    }

}


#endif //TTK_UTIL_CACHE_H
