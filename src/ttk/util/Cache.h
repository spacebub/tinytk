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

//! Eviction and keys for caches of text runs.
namespace ttk::Cache {

    //! Removes about a quarter of the entries of `held`, those used least recently, once it holds `keep` or more.
    //!
    //! Every value in `held` must have a `used` field that grows with recency of use. Entries whose `used` is at or
    //! below the quarter mark are removed, so ties can remove more. `dropped` is called with each value before it
    //! is removed. Does nothing while `held` holds fewer than `keep` entries.
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

    //! Owning key of a cache entry for a run of text under a font.
    //!
    //! A \ref RunMap is looked up with a \ref RunView of the same fields, so a hit allocates nothing.
    struct RunKey {
        //! Identity of the font, normally its address.
        std::uintptr_t font = 0;
        //! Every other input the cached answer depends on, packed by the cache that uses the key.
        std::int64_t at = 0;
        //! Text of the run, UTF-8.
        std::string run;
    };

    //! Non-owning form of \ref RunKey used to look up a \ref RunMap without copying the text.
    struct RunView {
        //! Identity of the font, normally its address.
        std::uintptr_t font = 0;
        //! Every other input the cached answer depends on, packed by the cache that uses the key.
        std::int64_t at = 0;
        //! Text of the run, UTF-8. Must stay alive for the lookup.
        std::string_view run;
    };

    //! Transparent hash of \ref RunKey and \ref RunView that gives equal values for equal fields.
    struct RunHash {
        using is_transparent = void;

        //! Number of bytes hashed from each end of the run. Only the ends and the length of a longer run are hashed,
        //! so hashing costs the same for any length.
        static constexpr std::size_t END = 64;

        //! Returns the hash of `view` from its font, its `at` and the ends and length of its run.
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

        //! Returns the hash of `key`, equal to the hash of a \ref RunView with the same fields.
        std::size_t operator()(const RunKey &key) const noexcept {
            return (*this)(RunView{.font = key.font, .at = key.at, .run = key.run});
        }
    };

    //! Transparent equality of \ref RunKey and \ref RunView that compares the font, `at` and the whole run.
    struct RunEqual {
        using is_transparent = void;

        //! Tests whether `one` and `two` have the same font, `at` and run.
        bool operator()(const RunKey &one, const RunKey &two) const noexcept {
            return one.font == two.font && one.at == two.at && one.run == two.run;
        }

        //! Tests whether key `one` and view `two` have the same font, `at` and run.
        bool operator()(const RunKey &one, const RunView &two) const noexcept {
            return one.font == two.font && one.at == two.at && one.run == two.run;
        }

        //! Tests whether view `one` and key `two` have the same font, `at` and run.
        bool operator()(const RunView &one, const RunKey &two) const noexcept {
            return one.font == two.font && one.at == two.at && one.run == two.run;
        }
    };

    //! Hash map from \ref RunKey to `Value` that can be looked up with a \ref RunView.
    template <typename Value>
    using RunMap = std::unordered_map<RunKey, Value, RunHash, RunEqual>;

    //! Returns the entry of `held` for `view` and whether it was just created.
    //!
    //! When there is no entry, one is added with a value-initialized `Value` and a copy of the run, and the second
    //! member is true. A reference stays valid until the entry is removed from `held`.
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
