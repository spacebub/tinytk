// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <memory>

#include <benchmark/benchmark.h>

#include "support/Canvas.h"
#include "support/Fixtures.h"
#include "ttk/dialogs/ConfirmDialog.h"
#include "ttk/dialogs/DialogLayer.h"
#include "ttk/toolkit/Root.h"

namespace {
bench::Canvas &sheet() {
    static bench::Canvas made(1280, 800);

    return made;
}

// `count` dialogs stacked, each grown in, with only the top one taking input.
ttk::DialogLayer *stacked(const int count) {
    bench::Canvas &canvas = sheet();

    canvas.bare();

    auto *layer = static_cast<ttk::DialogLayer *>(
            canvas.ui().layer(ttk::Root::DIALOGS)->add(std::make_unique<ttk::DialogLayer>()));

    for (int at = 0; at < count; ++at) {
        layer->show(std::make_unique<ttk::ConfirmDialog>(bench::Fixtures::words(3, static_cast<unsigned>(at) + 1U),
                                                         bench::Fixtures::words(24, static_cast<unsigned>(at) + 7U),
                                                         "Delete",
                                                         true,
                                                         [] {}));

        // The grow starts when the dialog is first laid out, so a second frame sees it finished.
        canvas.frame_at(canvas.tick(1.0));
        canvas.frame_at(canvas.tick(1.0));
    }

    return layer;
}

void DialogLayer_paint(benchmark::State &state) {
    stacked(static_cast<int>(state.range(0)));

    for (auto _ : state) {
        benchmark::DoNotOptimize(sheet().full());
    }
}

void DialogLayer_relayout(benchmark::State &state) {
    stacked(static_cast<int>(state.range(0)));

    for (auto _ : state) {
        sheet().ui().relayout();
        sheet().settle();
    }
}

// A frame owed by part of the top dialog's card, as a hover or its caret owes one.
void DialogLayer_cardFrame(benchmark::State &state) {
    stacked(static_cast<int>(state.range(0)));

    const BLRect part{(sheet().width() / 2.0) - 120.0, (sheet().height() / 2.0) - 20.0, 240.0, 40.0};

    for (auto _ : state) {
        sheet().ui().damage(part);
        benchmark::DoNotOptimize(sheet().frame());
    }
}

// Showing one over another and taking it down again, each with its frame.
void DialogLayer_showOver(benchmark::State &state) {
    ttk::DialogLayer *layer = stacked(1);

    for (auto _ : state) {
        layer->show(std::make_unique<ttk::ConfirmDialog>("Discard", "Changes are lost.", "Discard", true, [] {}));
        sheet().frame_at(sheet().tick(1.0));
        sheet().frame_at(sheet().tick(1.0));
        layer->dismiss();
        sheet().frame_at(sheet().tick(1.0));
    }
}
}

BENCHMARK(DialogLayer_paint)->Arg(1)->Arg(2)->Arg(3)->Unit(benchmark::kMicrosecond);
BENCHMARK(DialogLayer_relayout)->Arg(1)->Arg(2)->Arg(3)->Unit(benchmark::kMicrosecond);
BENCHMARK(DialogLayer_cardFrame)->Arg(1)->Arg(2)->Arg(3)->Unit(benchmark::kMicrosecond);
BENCHMARK(DialogLayer_showOver)->Unit(benchmark::kMicrosecond);
