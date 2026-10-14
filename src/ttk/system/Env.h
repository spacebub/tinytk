// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_SYSTEM_ENV_H
#define TTK_SYSTEM_ENV_H


#include <string>

//! Reading and writing environment variables of the current process.
namespace ttk::Env {

//! Returns the value of the environment variable `name` as UTF-8, or an empty string when it is not set.
[[nodiscard]] std::string get(const char *name);

//! Sets the environment variable `name` to `value`, replacing any existing value. Both are UTF-8.
//!
//! On Windows both the Win32 environment and the C runtime's copy are set, since libraries read either. A
//! name or a value there holds at most 32766 UTF-16 code units, so a longer one leaves the variable as it was.
//! The C runtime cannot hold an empty value, so an empty `value` removes its copy while the Win32 environment,
//! which get() reads and children inherit, keeps the variable defined.
//!
//! \note Not thread safe. Call it before any other thread starts.
void set(const char *name, const char *value);

//! Removes the environment variable `name`. Does nothing when it is not set.
//!
//! On Windows it is removed from both the Win32 environment and the C runtime's copy.
//!
//! \note Not thread safe. Call it before any other thread starts.
void unset(const char *name);

}


#endif //TTK_SYSTEM_ENV_H
