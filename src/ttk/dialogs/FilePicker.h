// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_DIALOGS_FILEPICKER_H
#define TTK_DIALOGS_FILEPICKER_H


#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "ttk/notices/Notifier.h"

namespace ttk {
    class FilePicker {
    public:
                struct Row {
            std::string name;
            std::string path;
            bool directory = false;
            bool hidden = false;
            bool marked = false;

            bool operator==(const Row &) const = default;
        };

        // What the dialog reads. The picker writes a field and says so through `changed`.
        struct State {
            bool open = false;
            std::string title;
            bool directories = false;

            // A folder can be taken as well as entered.
            bool folders = false;

            int markedFolders = 0;
            bool multiple = false;

            // A checkbox beside the pick button, shown when the label is not empty.
            std::string option;
            std::string optionHint;
            bool optionSet = false;

            std::string path;
            std::vector<std::string> parts;
            bool rooted = false;

            // The Windows drive list, above any root.
            bool drives = false;

            std::vector<Row> entries;
            int marked = 0;

            bool hiddenShown = false;

            // Shown when the list is empty.
            std::string nothing;

            // Typing a path.
            bool editing = false;
            bool saving = false;

            // nameSeed is bumped each time a name is pushed in. The field is the user's
            // after that.
            std::string name;
            int nameStem = 0;
            int nameSeed = 0;

            // What the name would write, and whether that file exists.
            std::string target;
            bool replacing = false;
        };

        using Chosen = std::function<void(const std::vector<std::string> &paths, bool option)>;

        explicit FilePicker(Notifier *notifier);

        [[nodiscard]] const State &state() const { return _state; }
        [[nodiscard]] State &state() { return _state; }

        // Called whenever the state changes, so the dialog can sync.
        std::function<void()> changed;
        void touch();

        // The directory each key was last left on. Kept for the application to persist.
        [[nodiscard]] std::string start_directory(const std::string &key) const;
        void remember_directory(const std::string &key, const std::string &path);

        // Opens the dialog on a directory, for one file or several.
                void open(const std::string &title, const std::vector<std::string> &filters, bool directories,
                  bool folders, bool multiple, const std::string &remember, Chosen chosen,
                  const std::string &option = {}, const std::string &optionHint = {});

        // Opens it to write a file, with `name` offered.
                void open_save(const std::string &title, const std::vector<std::string> &filters,
                       const std::string &remember, const std::string &name, Chosen chosen);

        void named(const std::string &name);

        void save(const std::string &name, bool replacing);

        void go(const std::string &path);
        void up();
        void up_to(int index);
        void show_drives();
        void show_hidden(bool value);
        void mark(const std::string &path);
        void choose(const std::vector<std::string> &paths);
        void choose_marked();
        void typed(const std::string &path);
        void dismiss();

    private:
                void start(const std::string &title, const std::vector<std::string> &filters, bool directories,
                   bool folders, bool multiple, const std::string &remember, Chosen chosen,
                   const std::string &option, const std::string &optionHint);

        // The open directory with the name under it, given the first filter's suffix
        // when it has none.
        [[nodiscard]] std::string target(const std::string &name) const;

        // Puts the name in the dialog's field with the stem selected.
        void suggest(const std::string &name);

        void show_target();

        void walk();

        void push();

        [[nodiscard]] static bool matches(std::string_view pattern, std::string_view name);
        [[nodiscard]] bool wanted(const std::filesystem::path &path) const;

                [[nodiscard]] bool hidden() const { return _hidden; }

        Notifier *_notifier;
                Chosen _chosen;
        std::string _remember;
        std::map<std::string, std::string> _remembered;
        bool _hidden = false;
        State _state;
        std::vector<std::string> _filters;

        std::vector<std::string> _suffixes;
        bool _anything{false};
        bool _directories{false};
        bool _multiple{false};

        // Set before start(), which pushes it to the dialog.
        bool _saving{false};

        std::string _saveName;

        std::string _path;
        std::vector<std::string> _parts;
        bool _drives{false};

        std::vector<std::string> _marked;

        struct Entry {
            std::string name;
            std::string path;
            bool directory{false};
            bool hidden{false};

            // Lowercased once, for sorting.
            std::string key;
        };

        std::vector<Entry> _entries;
    };
}


#endif //TTK_DIALOGS_FILEPICKER_H
