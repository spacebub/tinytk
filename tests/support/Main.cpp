// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <gtest/gtest.h>

#include "support/Sandbox.h"

namespace {
    class Sandboxed final : public ::testing::Environment {
    public:
        void SetUp() override { tests::Sandbox::enter(); }

        void TearDown() override { tests::Sandbox::leave(); }
    };
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Sandboxed);

    return RUN_ALL_TESTS();
}
