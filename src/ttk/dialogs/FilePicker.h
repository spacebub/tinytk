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
    //! File picker without widgets: it browses the filesystem, filters and marks entries, and reports the choice.
    //!
    //! Everything a dialog needs to show is kept in \ref State, and \ref changed is called whenever it changes.
    //! \ref FilePickerDialog presents it. All paths are UTF-8 with forward slashes.
    class FilePicker {
    public:
        //! Entry of the directory listing.
        struct Row {
            //! File or directory name, or the drive name in the drive list.
            std::string name;
            //! Full path of the entry.
            std::string path;
            //! Whether the entry is a directory or a drive.
            bool directory = false;
            //! Whether the name starts with a dot or, on Windows, the entry has the hidden attribute.
            bool hidden = false;
            //! Whether the entry's path is marked.
            bool marked = false;

            //! Tests whether two rows are equal in every field.
            bool operator==(const Row &) const = default;
        };

        //! Everything the dialog shows.
        //!
        //! The picker writes it and then calls \ref FilePicker::changed. The dialog writes \ref editing and
        //! \ref optionSet itself.
        struct State {
            //! Whether the picker is open. Set by \ref FilePicker::open() and \ref FilePicker::open_save(), cleared
            //! by \ref FilePicker::dismiss().
            bool open = false;
            //! Title shown at the top of the dialog.
            std::string title;
            //! Whether a directory is being picked. Files are left out of the listing.
            bool directories = false;

            //! Whether a folder can be marked and chosen as well as entered.
            bool folders = false;

            //! Number of marked paths that are directories.
            int markedFolders = 0;
            //! Whether several paths can be marked at once.
            bool multiple = false;

            //! Label of a checkbox shown beside the pick button. Empty for no checkbox.
            std::string option;
            //! Tooltip of the option checkbox.
            std::string optionHint;
            //! Whether the option checkbox is ticked. Cleared on open and passed to the \ref FilePicker::Chosen
            //! callback.
            bool optionSet = false;

            //! Directory being listed. Left as it was while the drive list is shown.
            std::string path;
            //! Non-empty components of \ref path, in order.
            std::vector<std::string> parts;
            //! Whether \ref path starts with a slash. A Windows drive path does not.
            bool rooted = false;

            //! Whether the listing shows the Windows drives instead of a directory.
            bool drives = false;

            //! Listing of \ref path: directories first, then the files that match the filters, each group sorted
            //! case-insensitively with digit runs compared by value. Hidden entries are included only while
            //! \ref hiddenShown is set.
            std::vector<Row> entries;
            //! Number of marked paths, including those marked in other directories during a multiple pick.
            int marked = 0;

            //! Whether hidden entries are listed.
            bool hiddenShown = false;

            //! Message to show when \ref entries is empty.
            std::string nothing;

            //! Whether the user is typing a path instead of browsing.
            bool editing = false;
            //! Whether the picker was opened by \ref FilePicker::open_save().
            bool saving = false;

            //! File name the picker last offered for saving.
            //!
            //! The name field belongs to the user once it is filled, so a dialog copies this in only when
            //! \ref nameSeed changes.
            std::string name;
            //! Counter bumped each time a \ref name is offered, so offering the same name twice is still seen.
            int nameSeed = 0;

            //! Full path the current name would save to, or empty when the name is empty.
            //!
            //! When the name does not end in the suffix of any filter, the suffix of the first filter is added.
            std::string target;
            //! Whether \ref target names an existing regular file.
            bool replacing = false;
        };

        //! Callback that receives the chosen `paths` and whether the option checkbox was ticked.
        using Chosen = std::function<void(const std::vector<std::string> &paths, bool option)>;

        //! Creates a picker that reports problems, such as a path that does not exist, as warnings through
        //! `notifier`, which must not be null and must outlive the picker.
        explicit FilePicker(Notifier *notifier);

        //! Returns the state the dialog shows.
        [[nodiscard]] const State &state() const { return _state; }

        //! Returns the state for the dialog to write \ref State::editing or \ref State::optionSet. Call
        //! \ref touch() after changing it.
        [[nodiscard]] State &state() { return _state; }

        //! Called synchronously after every change to \ref state(), from within the method that made it.
        std::function<void()> changed;

        //! Calls \ref changed when it is set.
        void touch();

        //! Hooks through which the application keeps what the picker remembers between runs.
        //!
        //! A hook left empty falls back to the picker's own store, which lives only as long as the picker.
        struct Memory {
            //! Returns the directory remembered under `key`, or an empty string.
            std::function<std::string(const std::string &key)> directory;
            //! Stores `path` as the directory remembered under `key`.
            std::function<void(const std::string &key, const std::string &path)> remember;
            //! Returns whether hidden entries are shown.
            std::function<bool()> hidden;
            //! Stores whether hidden entries are `shown`.
            std::function<void(bool shown)> showHidden;
        };

        //! Sets the hooks the picker remembers through to `memory`.
        void set_memory(Memory memory) { _memory = std::move(memory); }

        //! Returns the directory to open for `key`: the one remembered under it when that still exists, otherwise
        //! the home directory, otherwise the working directory.
        [[nodiscard]] std::string start_directory(const std::string &key) const;

        //! Remembers `path` as the directory to open for `key`. Does nothing when `path` is empty.
        void remember_directory(const std::string &key, const std::string &path);

        //! Opens the picker to pick files or a directory, starting in \ref start_directory() for `remember`.
        //!
        //! `filters` are glob patterns matched case-insensitively against file names, `*` for any run and `?` for
        //! one character, and an empty list shows all files. `directories` picks a directory instead of files,
        //! `folders` lets folders be marked and chosen as well as entered, and `multiple` allows several marks.
        //! A non-empty `option` adds a checkbox with that label and `optionHint` as its tooltip. `chosen` is
        //! called once with the result, and never when the picker is dismissed. The directory the choice is made
        //! in is remembered under `remember`.
        void open(const std::string &title, const std::vector<std::string> &filters, bool directories,
                  bool folders, bool multiple, const std::string &remember, Chosen chosen,
                  const std::string &option = {}, const std::string &optionHint = {});

        //! Opens the picker to save a file, offering `name` and starting in \ref start_directory() for `remember`.
        //!
        //! `chosen` receives the single \ref State::target path. `filters` decide which files are listed and which
        //! suffix is added to a name that lacks one. The directory the file is saved in is remembered under
        //! `remember`.
        void open_save(const std::string &title, const std::vector<std::string> &filters,
                       const std::string &remember, const std::string &name, Chosen chosen);

        //! Sets the file name being typed to `name` and updates \ref State::target and \ref State::replacing.
        void named(const std::string &name);

        //! Chooses the save target for `name`, worked out as \ref State::target describes.
        //!
        //! Warns and does nothing when `name` is empty, or when the file exists and `replacing` is false.
        void save(const std::string &name, bool replacing);

        //! Lists the directory `path`. Clears the marks unless several paths can be marked. Lists nothing when
        //! `path` cannot be read.
        void go(const std::string &path);

        //! Lists the parent of the current directory, or the drive list when the current directory is a drive
        //! root such as `C:/`.
        void up();

        //! Lists the ancestor of the current directory whose last component is \ref State::parts at `index`.
        void up_to(int index);

        //! Lists the drives instead of a directory. The list is empty outside Windows.
        void show_drives();

        //! Shows hidden entries when `value` is true and hides them otherwise, stores the choice and lists the
        //! directory again.
        void show_hidden(bool value);

        //! Toggles the mark on `path`. When only one path can be marked, marking it clears any other mark.
        void mark(const std::string &path);

        //! Remembers the current directory, dismisses the picker as \ref dismiss() does, then calls the
        //! \ref Chosen callback with `paths` and the option checkbox. The callback is skipped when `paths` is empty.
        void choose(const std::vector<std::string> &paths);

        //! Chooses the marked paths as \ref choose() does.
        void choose_marked();

        //! Acts on a `path` the user typed, after trimming it.
        //!
        //! `~` stands for the home directory and a relative path is taken from the working directory. A directory
        //! is listed. A file is chosen, or when saving its directory is listed and its name offered, unless a
        //! directory is being picked. Anything else is reported as a warning. An empty `path` only ends typing.
        void typed(const std::string &path);

        //! Ends typing a path when the user is typing one. Otherwise closes the picker and clears the marks
        //! without calling the \ref Chosen callback.
        void dismiss();

    private:
        void start(const std::string &title, const std::vector<std::string> &filters, bool directories,
                   bool folders, bool multiple, const std::string &remember, Chosen chosen,
                   const std::string &option, const std::string &optionHint);

        [[nodiscard]] std::string target(const std::string &name) const;

        void suggest(const std::string &name);

        void show_target();

        void walk();

        void push();

        [[nodiscard]] static bool matches(std::string_view pattern, std::string_view name);
        [[nodiscard]] bool wanted(const std::string &name) const;

        [[nodiscard]] bool hidden() const { return _memory.hidden ? _memory.hidden() : _hidden; }

        Notifier *_notifier;
        Chosen _chosen;
        std::string _remember;
        Memory _memory;
        std::map<std::string, std::string> _remembered;
        bool _hidden = false;
        State _state;
        std::vector<std::string> _filters;
        std::vector<std::string> _patterns;

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

            std::string key;
        };

        std::vector<Entry> _entries;
    };
}


#endif //TTK_DIALOGS_FILEPICKER_H
