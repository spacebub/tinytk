// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <utility>
#include <cstdint>
#include <filesystem>
#include <string>

#include <benchmark/benchmark.h>

#include "ttk/system/Json.h"
#include "support/Fixtures.h"
#include "support/Sandbox.h"

namespace {
    const std::string &text(const int groups, const int files) {
        static std::string held;
        static int lastGroups = -1;
        static int lastFiles = -1;

        if (groups != lastGroups || files != lastFiles) {
            held = bench::Fixtures::json(groups, files);

            lastGroups = groups;
            lastFiles = files;
        }

        return held;
    }

    void Json_read_file(benchmark::State &state) {
        const auto groups = static_cast<int>(state.range(0));
        const std::filesystem::path &path = bench::Fixtures::json_file(groups, 16);

        for ([[maybe_unused]] auto step : state) {
            ttk::Json::Doc doc = ttk::Json::read_file(path);

            if (!doc.valid()) {
                state.SkipWithError("the file did not parse");

                break;
            }

            benchmark::DoNotOptimize(doc.root());
        }

        state.SetItemsProcessed(state.iterations() * groups);
    }

    BENCHMARK(Json_read_file)->Arg(8)->Arg(64)->Arg(512);

    void Json_read_data(benchmark::State &state) {
        const std::string &data = text(64, 16);

        for ([[maybe_unused]] auto step : state) {
            ttk::Json::Doc doc = ttk::Json::read_data(data);

            benchmark::DoNotOptimize(doc.root());
        }

        state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(data.size()));
    }

    BENCHMARK(Json_read_data);

    void Json_obj_get_string(benchmark::State &state) {
        const std::string &data = text(64, 16);
        const ttk::Json::Doc doc = ttk::Json::read_data(data);

        yyjson_val *root = doc.root();

        for ([[maybe_unused]] auto step : state) {
            benchmark::DoNotOptimize(ttk::Json::obj_get_string(root, "active"));
        }
    }

    BENCHMARK(Json_obj_get_string);

    void Json_build_and_write(benchmark::State &state) {
        const auto rows = static_cast<int>(state.range(0));
        const std::filesystem::path at = bench::Sandbox::scratch("json") / "out.json";

        for ([[maybe_unused]] auto step : state) {
            const ttk::Json::Builder builder;

            yyjson_mut_val *root = builder.new_object();
            yyjson_mut_val *list = builder.new_array();

            for (int item = 0; item < rows; ++item) {
                yyjson_mut_val *entry = builder.new_object();

                builder.add_string(entry, "file", "/items/item.dat");
                builder.add_int(entry, "index", item);
                builder.add_bool(entry, "enabled", item % 3 != 0);

                ttk::Json::Builder::append_value(list, entry);
            }

            builder.add_value(root, "files", list);
            builder.set_root(root);

            benchmark::DoNotOptimize(builder.write_file(at));
        }

        state.SetItemsProcessed(state.iterations() * rows);
    }

    BENCHMARK(Json_build_and_write)->Arg(16)->Arg(256);
}
