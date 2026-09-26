// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "ttk/draw/WlShm.h"

namespace ttk {
    // Built without TTK_WLSHM: SDL keeps the window's pixels everywhere.
    struct WlShm::Impl {};

    WlShm::WlShm() = default;

    WlShm::~WlShm() = default;

    bool WlShm::available(SDL_Window * /*unused*/) {
        return false;
    }

    bool WlShm::open(SDL_Window * /*unused*/) {
        return false;
    }

    void WlShm::close() {}

    bool WlShm::opened() {
        return false;
    }

    bool WlShm::acquire(const int /*unused*/, const int /*unused*/, const bool /*unused*/,
                        Target & /*unused*/) {
        return false;
    }

    bool WlShm::present(const std::span<const BLRectI> /*unused*/) {
        return false;
    }
}
