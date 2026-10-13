// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cstddef>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "ttk/notices/Notice.h"
#include "ttk/notices/Notifier.h"

namespace {
    constexpr int BASE = 3200;
    constexpr int PER_BYTE = 18;
    constexpr int COUNTED = 160;

    std::vector<int> ids(const ttk::Notifier &notifier) {
        std::vector<int> out;

        for (const ttk::Notice &each : notifier.messages()) {
            out.push_back(each.id);
        }

        return out;
    }

    TEST(Notice, DefaultsToAnEmptyInfoThatStays) {
        const ttk::Notice notice;

        EXPECT_EQ(notice.id, 0);
        EXPECT_EQ(notice.severity, ttk::Severity::Info);
        EXPECT_TRUE(notice.title.empty());
        EXPECT_TRUE(notice.body.empty());
        EXPECT_EQ(notice.duration, 0);
    }

    TEST(Notifier, StartsEmpty) {
        const ttk::Notifier notifier;

        EXPECT_TRUE(notifier.messages().empty());
    }

    TEST(Notifier, EachHelperPostsItsSeverity) {
        ttk::Notifier notifier;

        notifier.info("i");
        notifier.success("s");
        notifier.warning("w");
        notifier.error("e");

        const std::vector<ttk::Notice> &posted = notifier.messages();

        ASSERT_EQ(posted.size(), 4U);
        EXPECT_EQ(posted[0].severity, ttk::Severity::Info);
        EXPECT_EQ(posted[1].severity, ttk::Severity::Success);
        EXPECT_EQ(posted[2].severity, ttk::Severity::Warning);
        EXPECT_EQ(posted[3].severity, ttk::Severity::Error);
    }

    TEST(Notifier, PostKeepsTheTextAndTitle) {
        ttk::Notifier notifier;

        notifier.post(ttk::Severity::Warning, "Disk nearly full", "Storage");
        notifier.success("Saved");

        EXPECT_EQ(notifier.messages()[0].body, "Disk nearly full");
        EXPECT_EQ(notifier.messages()[0].title, "Storage");
        EXPECT_EQ(notifier.messages()[1].body, "Saved");
        EXPECT_TRUE(notifier.messages()[1].title.empty());
    }

    TEST(Notifier, IdsCountUpFromZeroOldestFirst) {
        ttk::Notifier notifier;

        notifier.info("a");
        notifier.info("b");
        notifier.info("c");

        EXPECT_EQ(ids(notifier), (std::vector<int>{0, 1, 2}));
    }

    TEST(Notifier, IdsAreNotReusedAfterADismissal) {
        ttk::Notifier notifier;

        notifier.info("a");
        notifier.info("b");
        notifier.dismiss(1);
        notifier.info("c");

        EXPECT_EQ(ids(notifier), (std::vector<int>{0, 2}));
    }

    TEST(Notifier, DurationGrowsWithTheText) {
        ttk::Notifier notifier;

        notifier.info("");
        notifier.success("twelve bytes");

        EXPECT_EQ(notifier.messages()[0].duration, BASE);
        EXPECT_EQ(notifier.messages()[1].duration, BASE + (12 * PER_BYTE));
    }

    TEST(Notifier, DurationCountsAtMostOneHundredSixtyBytes) {
        ttk::Notifier notifier;

        notifier.warning(std::string(COUNTED, 'x'));
        notifier.warning(std::string(static_cast<std::size_t>(COUNTED) * 5, 'x'));

        EXPECT_EQ(notifier.messages()[0].duration, BASE + (COUNTED * PER_BYTE));
        EXPECT_EQ(notifier.messages()[1].duration, BASE + (COUNTED * PER_BYTE));
    }

    TEST(Notifier, DurationCountsBytesNotCharacters) {
        ttk::Notifier notifier;

        notifier.info("é");

        EXPECT_EQ(notifier.messages()[0].duration, BASE + (2 * PER_BYTE));
    }

    TEST(Notifier, ErrorHasNoDuration) {
        ttk::Notifier notifier;

        notifier.error("Broken", "Failure");
        notifier.post(ttk::Severity::Error, std::string(40, 'x'));

        EXPECT_EQ(notifier.messages()[0].duration, 0);
        EXPECT_EQ(notifier.messages()[1].duration, 0);
    }

    TEST(Notifier, KeepsAtMostFourDroppingTheOldest) {
        ttk::Notifier notifier;

        for (int at = 0; at < 6; ++at) {
            notifier.info(std::to_string(at));
        }

        EXPECT_EQ(ids(notifier), (std::vector<int>{2, 3, 4, 5}));
        EXPECT_EQ(notifier.messages().front().body, "2");
    }

    TEST(Notifier, ChangedIsCalledAfterEveryPost) {
        ttk::Notifier notifier;
        std::vector<std::size_t> sizes;

        notifier.changed = [&] { sizes.push_back(notifier.messages().size()); };

        for (int at = 0; at < 5; ++at) {
            notifier.info("x");
        }

        EXPECT_EQ(sizes, (std::vector<std::size_t>{1, 2, 3, 4, 4})) << "changed ran before the list was updated";
    }

    TEST(Notifier, DismissRemovesTheNoticeAndCallsChanged) {
        ttk::Notifier notifier;
        int changes = 0;

        notifier.info("a");
        notifier.info("b");
        notifier.changed = [&] { ++changes; };
        notifier.dismiss(0);

        EXPECT_EQ(ids(notifier), (std::vector<int>{1}));
        EXPECT_EQ(changes, 1);
    }

    TEST(Notifier, DismissingAnUnknownIdChangesNothing) {
        ttk::Notifier notifier;
        int changes = 0;

        notifier.info("a");
        notifier.changed = [&] { ++changes; };
        notifier.dismiss(42);
        notifier.dismiss(-1);

        EXPECT_EQ(ids(notifier), (std::vector<int>{0}));
        EXPECT_EQ(changes, 0);
    }

    TEST(Notifier, DismissingTwiceChangesOnce) {
        ttk::Notifier notifier;
        int changes = 0;

        notifier.info("a");
        notifier.changed = [&] { ++changes; };
        notifier.dismiss(0);
        notifier.dismiss(0);

        EXPECT_TRUE(notifier.messages().empty());
        EXPECT_EQ(changes, 1);
    }

    TEST(Notifier, PostWithoutAChangedCallbackIsSafe) {
        ttk::Notifier notifier;

        notifier.info("a");
        notifier.dismiss(0);

        EXPECT_TRUE(notifier.messages().empty());
    }
}
