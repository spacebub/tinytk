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
#include <vector>

namespace ttk {
    namespace Cache {

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

    }
}


#endif //TTK_UTIL_CACHE_H
