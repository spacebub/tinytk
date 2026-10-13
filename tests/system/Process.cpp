// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "support/Sandbox.h"
#include "ttk/system/Process.h"
#include "ttk/system/Text.h"

namespace {
    namespace fs = std::filesystem;
    namespace Process = ttk::Process;

    using Clock = std::chrono::steady_clock;

    constexpr std::chrono::seconds PATIENCE{2};
    constexpr int SLICE = 50;
    constexpr int LONG_WAIT = 5000;

    struct Ended {
        Process::State state{Process::State::Unknown};
        int code{-1000};
    };

    Ended finish(const Process::Id id, const Process::Stream output = Process::NOTHING) {
        const Clock::time_point deadline = Clock::now() + PATIENCE;
        Ended ended;

        while (Clock::now() < deadline) {
            ended.state = Process::poll(id, &ended.code);

            if (ended.state != Process::State::Running) {
                return ended;
            }

            if (output != Process::NOTHING) {
                Process::wait_for(output, Process::NOTHING, SLICE);
            } else {
                std::this_thread::yield();
            }
        }

        return ended;
    }

    // Reads until the end of the stream, which a pseudo-terminal reports once the child has gone.
    std::string drain(const Process::Stream output) {
        const Clock::time_point deadline = Clock::now() + PATIENCE;
        std::string all;
        std::string piece;

        while (Clock::now() < deadline) {
            if (!Process::read(output, piece)) {
                break;
            }

            if (piece.empty()) {
                Process::wait_for(output, Process::NOTHING, SLICE);
            }

            all += piece;
        }

        return all;
    }

    std::string first_line(const Process::Stream output) {
        const Clock::time_point deadline = Clock::now() + PATIENCE;
        std::string got;
        std::string piece;

        while (!got.contains('\n') && Clock::now() < deadline && Process::read(output, piece)) {
            got += piece;
            Process::wait_for(output, Process::NOTHING, SLICE);
        }

        return got;
    }

    struct Outcome {
        bool started{false};
        std::string error;
        std::string output;
        Ended ended;
    };

    Outcome launch(const fs::path &program, const std::vector<std::string> &arguments, const fs::path &directory = {},
                   const std::map<std::string, std::string> &environment = {}) {
        Outcome result;
        Process::Id id = 0;
        Process::Stream output = Process::NOTHING;

        result.started = Process::start(program, arguments, directory, environment, &id, &output, &result.error);

        if (!result.started) {
            return result;
        }

        result.output = drain(output);
        result.ended = finish(id, output);
        Process::close_stream(output);

        return result;
    }

    double elapsed_seconds(const Clock::time_point since) {
        return std::chrono::duration<double>(Clock::now() - since).count();
    }
}

TEST(ProcessStart, TrueFinishesWithZero) {
    Process::Id id = 0;

    ASSERT_TRUE(Process::start("true", {}, {}, {}, &id));
    EXPECT_NE(id, 0U);

    const Ended ended = finish(id);

    EXPECT_EQ(ended.state, Process::State::Finished);
    EXPECT_EQ(ended.code, 0);
}

TEST(ProcessStart, FalseFails) {
    Process::Id id = 0;

    ASSERT_TRUE(Process::start("false", {}, {}, {}, &id));

    const Ended ended = finish(id);

    EXPECT_EQ(ended.state, Process::State::Failed);
    EXPECT_EQ(ended.code, 1);
}

TEST(ProcessStart, ReportsTheExitStatus) {
    const Outcome result = launch("sh", {"-c", "exit 7"});

    ASSERT_TRUE(result.started) << result.error;
    EXPECT_EQ(result.ended.state, Process::State::Failed);
    EXPECT_EQ(result.ended.code, 7);
}

TEST(ProcessStart, IdsAreDistinct) {
    Process::Id first = 0;
    Process::Id second = 0;

    ASSERT_TRUE(Process::start("true", {}, {}, {}, &first));
    ASSERT_TRUE(Process::start("true", {}, {}, {}, &second));

    EXPECT_NE(first, second);
    EXPECT_EQ(finish(first).state, Process::State::Finished);
    EXPECT_EQ(finish(second).state, Process::State::Finished);
}

TEST(ProcessStart, CapturesOutput) {
    const Outcome result = launch("echo", {"hello"});

    ASSERT_TRUE(result.started) << result.error;
    EXPECT_EQ(ttk::Text::trim(result.output), "hello");
    EXPECT_EQ(result.ended.state, Process::State::Finished);
}

TEST(ProcessStart, OutputComesThroughATerminal) {
    const Outcome result = launch("echo", {"line"});

    EXPECT_EQ(result.output, "line\r\n") << "a pseudo-terminal turns a newline into CR LF";
}

TEST(ProcessStart, PassesArgumentsUntouched) {
    const Outcome result = launch("echo", {"two words", "it's", R"("quoted")", "$HOME", "*", R"(back\slash)"});

    ASSERT_TRUE(result.started) << result.error;
    EXPECT_EQ(ttk::Text::trim(result.output), R"(two words it's "quoted" $HOME * back\slash)")
            << "no shell sits between start and the program";
}

TEST(ProcessStart, PassesEachArgumentSeparately) {
    const Outcome result = launch("sh", {"-c", R"(printf '[%s]' "$@")", "sh", "a b", "", "c"});

    EXPECT_EQ(result.output, "[a b][][c]");
}

TEST(ProcessStart, PassesUtf8Arguments) {
    const Outcome result = launch("echo", {"\xE6\x97\xA5\xE6\x9C\xAC \xF0\x9F\x98\x80"});

    EXPECT_EQ(ttk::Text::trim(result.output), "\xE6\x97\xA5\xE6\x9C\xAC \xF0\x9F\x98\x80");
}

TEST(ProcessStart, CapturesStandardError) {
    const Outcome result = launch("sh", {"-c", "echo oops >&2"});

    EXPECT_EQ(ttk::Text::trim(result.output), "oops") << "stderr shares the output stream";
}

TEST(ProcessStart, CapturesOutputLargerThanOneRead) {
    const Outcome result = launch("sh", {"-c", "i=0; while [ $i -lt 2000 ]; do echo 0123456789; i=$((i+1)); done"});

    ASSERT_TRUE(result.started) << result.error;
    EXPECT_EQ(result.output.size(), 2000U * 12U) << "every line arrives, each ending in CR LF";
    EXPECT_EQ(result.ended.state, Process::State::Finished);
}

TEST(ProcessStart, SilentProgramGivesNoOutput) {
    const Outcome result = launch("true", {});

    EXPECT_EQ(result.output, "");
    EXPECT_EQ(result.ended.state, Process::State::Finished);
}

TEST(ProcessStart, RunsInTheWorkingDirectory) {
    const fs::path directory = tests::Sandbox::scratch("process-cwd");
    const Outcome result = launch("pwd", {"-P"}, directory);

    ASSERT_TRUE(result.started) << result.error;
    EXPECT_EQ(fs::path(ttk::Text::trim(result.output)), fs::canonical(directory));
}

TEST(ProcessStart, EmptyWorkingDirectoryKeepsTheCurrentOne) {
    const Outcome result = launch("pwd", {"-P"});

    EXPECT_EQ(fs::path(ttk::Text::trim(result.output)), fs::canonical(fs::current_path()));
}

TEST(ProcessStart, AddsEnvironmentVariables) {
    const Outcome result =
            launch("sh", {"-c", R"(printf %s "$TTK_TEST_ADDED")"}, {}, {{"TTK_TEST_ADDED", "added value"}});

    EXPECT_EQ(ttk::Text::trim(result.output), "added value");
}

TEST(ProcessStart, ReplacesEnvironmentVariables) {
    const Outcome result = launch("sh", {"-c", R"(printf %s "$HOME")"}, {}, {{"HOME", "/replaced"}});

    EXPECT_EQ(ttk::Text::trim(result.output), "/replaced");
}

TEST(ProcessStart, KeepsTheRestOfTheEnvironment) {
    const Outcome result = launch("sh", {"-c", R"(printf %s "$XDG_CONFIG_HOME")"}, {}, {{"TTK_TEST_OTHER", "x"}});

    EXPECT_EQ(fs::path(ttk::Text::trim(result.output)), tests::Sandbox::root() / "config");
}

TEST(ProcessStart, RunsAProgramGivenByPath) {
    const fs::path directory = tests::Sandbox::scratch("process-script");
    const fs::path script = directory / "say";

    std::ofstream(script) << "#!/bin/sh\necho from script \"$1\"\n";
    fs::permissions(script, fs::perms::owner_all);

    const Outcome result = launch(script, {"arg"});

    ASSERT_TRUE(result.started) << result.error;
    EXPECT_EQ(ttk::Text::trim(result.output), "from script arg");
}

TEST(ProcessStart, MissingProgramFails) {
    Process::Id id = 0;
    std::string error;

    EXPECT_FALSE(Process::start("ttk-no-such-program-anywhere", {}, {}, {}, &id, nullptr, &error));
    EXPECT_FALSE(error.empty());
}

TEST(ProcessStart, MissingPathFails) {
    std::string error;

    EXPECT_FALSE(Process::start(tests::Sandbox::scratch("process-missing") / "absent", {}, {}, {}, nullptr, nullptr,
                                &error));
    EXPECT_FALSE(error.empty());
}

TEST(ProcessStart, MissingProgramWithOutputFails) {
    Process::Id id = 0;
    Process::Stream output = Process::NOTHING;
    std::string error;

    EXPECT_FALSE(Process::start("ttk-no-such-program-anywhere", {}, {}, {}, &id, &output, &error));
    EXPECT_FALSE(error.empty());
}

TEST(ProcessStart, FileWithoutExecutePermissionFails) {
    const fs::path directory = tests::Sandbox::scratch("process-noexec");
    const fs::path file = directory / "plain";

    std::ofstream(file) << "#!/bin/sh\ntrue\n";
    fs::permissions(file, fs::perms::owner_read | fs::perms::owner_write);

    std::string error;
    Process::Id id = 0;

    EXPECT_FALSE(Process::start(file, {}, {}, {}, &id, nullptr, &error));
    EXPECT_FALSE(error.empty());
}

TEST(ProcessStart, MissingProgramWithoutErrorPointer) {
    EXPECT_FALSE(Process::start("ttk-no-such-program-anywhere", {}, {}));
}

TEST(ProcessStart, UntrackedChildStarts) {
    EXPECT_TRUE(Process::start("true", {}, {})) << "no id means nothing needs to reap it";
}

TEST(ProcessStart, UntrackedChildStillCapturesOutput) {
    Process::Stream output = Process::NOTHING;

    ASSERT_TRUE(Process::start("echo", {"untracked"}, {}, {}, nullptr, &output));
    ASSERT_NE(output, Process::NOTHING);
    EXPECT_EQ(ttk::Text::trim(drain(output)), "untracked");

    Process::close_stream(output);
}

TEST(ProcessPoll, UnknownIdIsUnknown) {
    EXPECT_EQ(Process::poll(0), Process::State::Unknown);
    EXPECT_EQ(Process::poll(987654321), Process::State::Unknown);
}

TEST(ProcessPoll, EndIsReportedOnce) {
    Process::Id id = 0;

    ASSERT_TRUE(Process::start("true", {}, {}, {}, &id));
    ASSERT_EQ(finish(id).state, Process::State::Finished);

    int code = 99;

    EXPECT_EQ(Process::poll(id, &code), Process::State::Unknown);
    EXPECT_EQ(code, 99) << "the code is only written when the end is reported";
}

TEST(ProcessPoll, RunningChildIsRunning) {
    Process::Id id = 0;

    ASSERT_TRUE(Process::start("sleep", {"30"}, {}, {}, &id));

    int code = 99;

    EXPECT_EQ(Process::poll(id, &code), Process::State::Running);
    EXPECT_EQ(code, 99);

    Process::force(id);
    EXPECT_EQ(finish(id).state, Process::State::Failed);
}

TEST(ProcessPoll, CodeIsOptional) {
    Process::Id id = 0;

    ASSERT_TRUE(Process::start("false", {}, {}, {}, &id));

    const Clock::time_point deadline = Clock::now() + PATIENCE;
    Process::State state = Process::State::Running;

    while (state == Process::State::Running && Clock::now() < deadline) {
        state = Process::poll(id);
    }

    EXPECT_EQ(state, Process::State::Failed);
}

TEST(ProcessStop, EndsTheChildWithSigterm) {
    Process::Id id = 0;

    ASSERT_TRUE(Process::start("sleep", {"30"}, {}, {}, &id));

    const Clock::time_point began = Clock::now();

    Process::stop(id);

    const Ended ended = finish(id);

    EXPECT_EQ(ended.state, Process::State::Failed);
    EXPECT_EQ(ended.code, -SIGTERM) << "a signal is reported negated";
    EXPECT_LT(elapsed_seconds(began), 1.0);
}

TEST(ProcessForce, EndsTheChildWithSigkill) {
    Process::Id id = 0;

    ASSERT_TRUE(Process::start("sleep", {"30"}, {}, {}, &id));

    Process::force(id);

    const Ended ended = finish(id);

    EXPECT_EQ(ended.state, Process::State::Failed);
    EXPECT_EQ(ended.code, -SIGKILL);
}

TEST(ProcessForce, EndsAChildThatIgnoresStop) {
    Process::Id id = 0;
    Process::Stream output = Process::NOTHING;

    ASSERT_TRUE(Process::start("sh", {"-c", "trap '' TERM; echo ready; while :; do :; done"}, {}, {}, &id, &output));
    EXPECT_EQ(ttk::Text::trim(first_line(output)), "ready");

    Process::stop(id);
    EXPECT_EQ(Process::poll(id), Process::State::Running) << "TERM is ignored";

    Process::force(id);

    const Ended ended = finish(id);

    EXPECT_EQ(ended.state, Process::State::Failed);
    EXPECT_EQ(ended.code, -SIGKILL);

    Process::close_stream(output);
}

TEST(ProcessStop, UnknownIdDoesNothing) {
    Process::stop(0);
    Process::stop(987654321);
    Process::force(0);
    Process::force(987654321);

    SUCCEED();
}

TEST(ProcessStop, AfterTheEndWasReportedDoesNothing) {
    Process::Id id = 0;

    ASSERT_TRUE(Process::start("true", {}, {}, {}, &id));
    ASSERT_EQ(finish(id).state, Process::State::Finished);

    Process::stop(id);
    Process::force(id);

    EXPECT_EQ(Process::poll(id), Process::State::Unknown);
}

TEST(ProcessRead, NothingFailsAndEmpties) {
    std::string into = "stale";

    EXPECT_FALSE(Process::read(Process::NOTHING, into));
    EXPECT_EQ(into, "");
}

TEST(ProcessRead, NothingWaitingIsTrueAndEmpty) {
    Process::Stream readEnd = Process::NOTHING;
    Process::Stream writeEnd = Process::NOTHING;

    ASSERT_TRUE(Process::make_waker(&readEnd, &writeEnd));

    std::string into = "stale";

    EXPECT_TRUE(Process::read(readEnd, into));
    EXPECT_EQ(into, "");

    Process::close_stream(readEnd);
    Process::close_stream(writeEnd);
}

TEST(ProcessRead, EndOfStreamIsFalseAndEmpty) {
    Process::Stream readEnd = Process::NOTHING;
    Process::Stream writeEnd = Process::NOTHING;

    ASSERT_TRUE(Process::make_waker(&readEnd, &writeEnd));
    Process::close_stream(writeEnd);

    std::string into = "stale";

    EXPECT_FALSE(Process::read(readEnd, into));
    EXPECT_EQ(into, "");

    Process::close_stream(readEnd);
}

TEST(ProcessRead, ChildOutputEndsWithFalse) {
    Process::Id id = 0;
    Process::Stream output = Process::NOTHING;

    ASSERT_TRUE(Process::start("true", {}, {}, {}, &id, &output));
    EXPECT_EQ(drain(output), "");

    std::string into = "stale";

    EXPECT_FALSE(Process::read(output, into)) << "the stream has ended";
    EXPECT_EQ(into, "");
    EXPECT_EQ(finish(id).state, Process::State::Finished);

    Process::close_stream(output);
}

TEST(ProcessWaker, MakesTwoDistinctEnds) {
    Process::Stream readEnd = Process::NOTHING;
    Process::Stream writeEnd = Process::NOTHING;

    ASSERT_TRUE(Process::make_waker(&readEnd, &writeEnd));
    EXPECT_NE(readEnd, Process::NOTHING);
    EXPECT_NE(writeEnd, Process::NOTHING);
    EXPECT_NE(readEnd, writeEnd);

    Process::close_stream(readEnd);
    Process::close_stream(writeEnd);
}

TEST(ProcessWaker, WakeWritesOneByte) {
    Process::Stream readEnd = Process::NOTHING;
    Process::Stream writeEnd = Process::NOTHING;

    ASSERT_TRUE(Process::make_waker(&readEnd, &writeEnd));

    Process::wake(writeEnd);

    std::string into;

    EXPECT_TRUE(Process::read(readEnd, into));
    EXPECT_EQ(into.size(), 1U);

    EXPECT_TRUE(Process::read(readEnd, into));
    EXPECT_EQ(into, "") << "the byte was consumed";

    Process::close_stream(readEnd);
    Process::close_stream(writeEnd);
}

TEST(ProcessWaker, WokenWaitReturnsAtOnce) {
    Process::Stream readEnd = Process::NOTHING;
    Process::Stream writeEnd = Process::NOTHING;

    ASSERT_TRUE(Process::make_waker(&readEnd, &writeEnd));

    Process::wake(writeEnd);

    const Clock::time_point began = Clock::now();

    Process::wait_for(Process::NOTHING, readEnd, LONG_WAIT);
    Process::wait_for(Process::NOTHING, readEnd, LONG_WAIT);

    EXPECT_LT(elapsed_seconds(began), 1.0) << "the byte stays until read, so every wait returns";

    Process::close_stream(readEnd);
    Process::close_stream(writeEnd);
}

TEST(ProcessWaker, WakeFromAnotherThreadEndsTheWait) {
    Process::Stream readEnd = Process::NOTHING;
    Process::Stream writeEnd = Process::NOTHING;

    ASSERT_TRUE(Process::make_waker(&readEnd, &writeEnd));

    const Clock::time_point began = Clock::now();
    std::thread waker([writeEnd] { Process::wake(writeEnd); });

    Process::wait_for(Process::NOTHING, readEnd, LONG_WAIT);
    waker.join();

    EXPECT_LT(elapsed_seconds(began), 1.0);

    Process::close_stream(readEnd);
    Process::close_stream(writeEnd);
}

TEST(ProcessWaker, WakeNothingDoesNothing) {
    Process::wake(Process::NOTHING);
    Process::close_stream(Process::NOTHING);

    SUCCEED();
}

TEST(ProcessWaitFor, ReturnsAtOnceWithNothingToWaitOn) {
    const Clock::time_point began = Clock::now();

    Process::wait_for(Process::NOTHING, Process::NOTHING, LONG_WAIT);

    EXPECT_LT(elapsed_seconds(began), 1.0);
}

TEST(ProcessWaitFor, ReturnsWhenOutputArrives) {
    Process::Id id = 0;
    Process::Stream output = Process::NOTHING;

    ASSERT_TRUE(Process::start("echo", {"soon"}, {}, {}, &id, &output));

    const Clock::time_point began = Clock::now();

    Process::wait_for(output, Process::NOTHING, LONG_WAIT);

    EXPECT_LT(elapsed_seconds(began), 1.0);
    EXPECT_EQ(ttk::Text::trim(drain(output)), "soon");
    EXPECT_EQ(finish(id, output).state, Process::State::Finished);

    Process::close_stream(output);
}

TEST(ProcessWaitFor, TimesOutWhenNothingHappens) {
    Process::Stream readEnd = Process::NOTHING;
    Process::Stream writeEnd = Process::NOTHING;

    ASSERT_TRUE(Process::make_waker(&readEnd, &writeEnd));

    const Clock::time_point began = Clock::now();

    Process::wait_for(Process::NOTHING, readEnd, 0);

    EXPECT_LT(elapsed_seconds(began), 1.0) << "a zero timeout does not block";

    Process::close_stream(readEnd);
    Process::close_stream(writeEnd);
}
