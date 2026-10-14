// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cstdlib>
#include <string>

#include "ttk/system/Env.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "ttk/system/Text.h"
#endif

namespace ttk {
std::string Env::get(const char *name) {
#ifdef _WIN32
    // The Win32 copy, since children inherit it and it can hold an empty value.
    const std::wstring called = Text::widen(name);
    const DWORD room = GetEnvironmentVariableW(called.c_str(), nullptr, 0);

    if (room == 0) {
        return {};
    }

    std::wstring value(room, L'\0');
    const DWORD written = GetEnvironmentVariableW(called.c_str(), value.data(), room);

    value.resize(written);

    return Text::narrow(value);
#else
    // NOLINTNEXTLINE(concurrency-mt-unsafe): set() runs before any thread starts.
    const char *value = std::getenv(name);

    return value != nullptr ? std::string(value) : std::string();
#endif
}

void Env::set(const char *name, const char *value) {
#ifdef _WIN32
    const std::wstring held = Text::widen(value);
    const std::wstring called = Text::widen(name);

    // The C library ends the process over a name or a value this long, and Win32 refuses one too.
    if (called.size() >= _MAX_ENV || held.size() >= _MAX_ENV) {
        return;
    }

    // The C library may write the Win32 copy too, removing it for an empty value, so the Win32 write goes last.
    // NOLINTNEXTLINE(concurrency-mt-unsafe): runs before any thread starts.
    _wputenv_s(called.c_str(), held.c_str());

    SetEnvironmentVariableW(called.c_str(), held.c_str());
#else
    // NOLINTNEXTLINE(concurrency-mt-unsafe): runs before any thread starts.
    setenv(name, value, 1);
#endif
}

void Env::unset(const char *name) {
#ifdef _WIN32
    const std::wstring called = Text::widen(name);

    // No variable has a name this long, and the C library ends the process over one.
    if (called.size() >= _MAX_ENV) {
        return;
    }

    SetEnvironmentVariableW(called.c_str(), nullptr);

    // An empty value removes the C library's copy.
    // NOLINTNEXTLINE(concurrency-mt-unsafe): runs before any thread starts.
    _wputenv_s(called.c_str(), L"");
#else
    // NOLINTNEXTLINE(concurrency-mt-unsafe): runs before any thread starts.
    unsetenv(name);
#endif
}
}
