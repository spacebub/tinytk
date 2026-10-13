// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <string>

#include <gtest/gtest.h>

#include "ttk/system/Notify.h"

namespace {
namespace Notify = ttk::Notify;
}

TEST(NotifyStub, ShowFailsWithAReason) {
    std::string why;

    EXPECT_EQ(Notify::show(Notify::Message{.title = "title", .body = "body"}, 0, &why), 0U);
    EXPECT_FALSE(why.empty());
}

TEST(NotifyStub, ShowFailsWithoutAReasonPointer) {
    EXPECT_EQ(Notify::show(Notify::Message{}), 0U);
}

TEST(NotifyStub, CloseAndStopDoNothing) {
    Notify::close(0);
    Notify::close(42);
    Notify::stop();
    Notify::stop();

    SUCCEED();
}
