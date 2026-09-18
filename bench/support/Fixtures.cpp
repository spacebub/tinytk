// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <array>
#include <fstream>
#include <map>
#include <utility>

#include "support/Fixtures.h"
#include "support/Sandbox.h"

namespace bench::Fixtures {
    namespace {
        constexpr std::array<const char *, 32> VOCABULARY = {
            "window", "surface", "damage", "frame", "glyph", "label", "button", "field",
            "scroll", "panel", "shade", "accent", "border", "margin", "column", "row",
            "pointer", "focus", "caret", "layer", "popup", "dialog", "toast", "hint",
            "tween", "clock", "alarm", "thread", "process", "fetch", "config", "path",
        };

        unsigned next(unsigned &state) {
            state = (state * 1103515245U) + 12345U;

            return (state >> 16U) & 0x7fffU;
        }
    }

    std::string words(const int count, const unsigned seed) {
        unsigned state = seed;
        std::string out;

        for (int at = 0; at < count; ++at) {
            if (at > 0) {
                out.push_back(' ');
            }

            out += VOCABULARY[next(state) % VOCABULARY.size()];
        }

        return out;
    }

    std::string paragraph(const int sentences) {
        std::string out;

        for (int at = 0; at < sentences; ++at) {
            out += words(6 + (at % 7), static_cast<unsigned>(at) + 3U);
            out += ". ";
        }

        return out;
    }

    std::vector<std::string> lines(const int count) {
        std::vector<std::string> out;
        out.reserve(static_cast<size_t>(count));

        for (int at = 0; at < count; ++at) {
            out.push_back(std::to_string(at) + ": " + words(9, static_cast<unsigned>(at) + 1U));
        }

        return out;
    }

    std::string json(const int groups, const int items) {
        std::string out = "{\n  \"active\": \"group-0\",\n  \"groups\": [\n";

        for (int group = 0; group < groups; ++group) {
            out += "    {\"id\": \"group-" + std::to_string(group) + "\", \"name\": \""
                + words(3, static_cast<unsigned>(group) + 1U) + "\", \"items\": [\n";

            for (int item = 0; item < items; ++item) {
                out += "      {\"file\": \"/items/" + words(1, static_cast<unsigned>(item) + 7U)
                    + std::to_string(item) + ".dat\", \"index\": " + std::to_string(item)
                    + ", \"enabled\": " + (item % 3 == 0 ? "false" : "true") + "}"
                    + (item + 1 < items ? ",\n" : "\n");
            }

            out += "    ]}" + std::string(group + 1 < groups ? ",\n" : "\n");
        }

        out += "  ]\n}\n";

        return out;
    }

    const std::filesystem::path &json_file(const int groups, const int items) {
        static std::map<std::pair<int, int>, std::filesystem::path> written;

        const auto key = std::make_pair(groups, items);

        if (const auto found = written.find(key); found != written.end()) {
            return found->second;
        }

        const std::filesystem::path at = Sandbox::root() / "fixtures"
            / ("groups-" + std::to_string(groups) + "-" + std::to_string(items) + ".json");

        std::error_code code;
        std::filesystem::create_directories(at.parent_path(), code);

        std::ofstream out(at, std::ios::binary);
        out << json(groups, items);

        return written.emplace(key, at).first->second;
    }
}
