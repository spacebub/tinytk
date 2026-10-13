// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <cstdlib>
#include <string>

#include "ttk/system/Env.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace ttk {
#ifdef _WIN32
namespace {
std::wstring widen(const char *text) {
    const int wide = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    std::wstring held(static_cast<size_t>(wide), L'\0');

    MultiByteToWideChar(CP_UTF8, 0, text, -1, held.data(), wide);
    held.resize(static_cast<size_t>(std::max(wide - 1, 0)));

    return held;
}

std::string narrow(const wchar_t *text) {
    const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    std::string held(static_cast<size_t>(size), '\0');

    WideCharToMultiByte(CP_UTF8, 0, text, -1, held.data(), size, nullptr, nullptr);
    held.resize(static_cast<size_t>(std::max(size - 1, 0)));

    return held;
}
}
#endif

std::string Env::get(const char *name) {
#ifdef _WIN32
    // The wide copy, since the narrow one is in the ANSI code page rather than UTF-8.
    wchar_t *value = nullptr;
    size_t length = 0;

    if (_wdupenv_s(&value, &length, widen(name).c_str()) != 0 || value == nullptr) {
        return {};
    }

    std::string held = narrow(value);

    std::free(value);

    return held;
#else
    // NOLINTNEXTLINE(concurrency-mt-unsafe): set() runs before any thread starts.
    const char *value = std::getenv(name);

    return value != nullptr ? std::string(value) : std::string();
#endif
}

void Env::set(const char *name, const char *value) {
#ifdef _WIN32
    const std::wstring held = widen(value);
    const std::wstring called = widen(name);

    // The C library ends the process over a variable longer than it holds.
    if (called.size() + 1 + held.size() >= _MAX_ENV) {
        return;
    }

    SetEnvironmentVariableW(called.c_str(), held.c_str());

    // Windows keeps the C library's environment separate, and get() reads that one.
    // NOLINTNEXTLINE(concurrency-mt-unsafe): runs before any thread starts.
    _wputenv_s(called.c_str(), held.c_str());
#else
    // NOLINTNEXTLINE(concurrency-mt-unsafe): runs before any thread starts.
    setenv(name, value, 1);
#endif
}

void Env::unset(const char *name) {
#ifdef _WIN32
    const std::wstring called = widen(name);

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
