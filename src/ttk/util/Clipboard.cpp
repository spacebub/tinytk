// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <SDL3/SDL.h>

#include "ttk/util/Clipboard.h"

namespace ttk {
    namespace Clipboard {

        std::string read() {
            char *held = SDL_GetClipboardText();

            if (held == nullptr) {
                return {};
            }

            std::string text = held;

            SDL_free(held);

            return text;
        }

        void write(const std::string &text) {
            SDL_SetClipboardText(text.c_str());
        }

    }
}
