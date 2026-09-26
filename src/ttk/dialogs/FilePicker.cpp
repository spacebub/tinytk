// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <utility>

#include "ttk/system/Paths.h"
#include "ttk/system/Text.h"
#include "ttk/dialogs/FilePicker.h"
#include "ttk/util/Desktop.h"
#include "ttk/util/Format.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace ttk {
    namespace {

        // Windows hands back backslashes.
        std::string forward(std::string path) {
            for (char &each : path) {
                if (each == '\\') {
                    each = '/';
                }
            }

            return path;
        }

        std::vector<std::string> split(const std::string &path) {
            std::vector<std::string> parts;

            for (std::string &part : Text::split(path, '/')) {
                if (!part.empty()) {
                    parts.push_back(std::move(part));
                }
            }

            return parts;
        }

        bool rooted(const std::string &path) {
            return path.starts_with('/');
        }

        // "~" is home. A relative path hangs off the working directory.
        std::string resolve(const std::string &typed) {
            std::string text = forward(typed);

            if (text == "~" || text.starts_with("~/")) {
                if (const std::filesystem::path home = Paths::home_directory(); !home.empty()) {
                    text = home.generic_string() + text.substr(1);
                }
            }

            std::error_code code;
            std::filesystem::path path = std::filesystem::absolute(text, code);

            if (code) {
                return text;
            }

            std::error_code asked;

            if (const std::filesystem::path real = std::filesystem::weakly_canonical(path, asked);
                !asked) {
                path = real;
            } else {
                path = path.lexically_normal();
            }

            // A trailing slash would leave an empty final name.
            if (path.filename().empty() && path.has_relative_path()) {
                path = path.parent_path();
            }

            return path.generic_string();
        }

        // Windows drops trailing dots and spaces.
        std::string tidy(std::string name) {
            while (!name.empty() && (name.back() == '.' || name.back() == ' ')) {
                name.pop_back();
            }

            return name;
        }

        // A leading dot, or the hidden attribute on Windows.
        bool concealed(const std::filesystem::directory_entry &step) {
#ifdef _WIN32
            if (step.path().filename().string().starts_with('.')) {
                return true;
            }

            const DWORD marks = GetFileAttributesW(step.path().c_str());

            return marks != INVALID_FILE_ATTRIBUTES && (marks & FILE_ATTRIBUTE_HIDDEN) != 0;
#else
            return step.path().filename().string().starts_with('.');
#endif
        }

    }

    FilePicker::FilePicker(Notifier *notifier) : _notifier(notifier) {}

    void FilePicker::touch() {
        if (changed) {
            changed();
        }
    }

    void FilePicker::open(const std::string &title, const std::vector<std::string> &filters, const bool directories,
                          const bool folders, const bool multiple, const std::string &remember, Chosen chosen,
                          const std::string &option, const std::string &optionHint) {
        _saving = false;
        start(title, filters, directories, folders, multiple, remember, std::move(chosen), option, optionHint);
        suggest("");
    }

    void FilePicker::open_save(const std::string &title, const std::vector<std::string> &filters,
                               const std::string &remember, const std::string &name, Chosen chosen) {
        _saving = true;
        start(title, filters, false, false, false, remember, std::move(chosen), "", "");
        suggest(name);
    }

    void FilePicker::named(const std::string &name) {
        _saveName = name;

        show_target();
    }

    void FilePicker::up() {
        // Above a drive letter is the list of drives, not a folder.
        if (!rooted(_path) && _parts.size() <= 1) {
            show_drives();

            return;
        }

        go(Format::from_path(std::filesystem::path(_path).parent_path()));
    }

    void FilePicker::up_to(const int index) {
        std::string wanted = rooted(_path) ? "/" : "";

        for (int part = 0; part <= index && std::cmp_less(part, _parts.size()); part++) {
            wanted += _parts[static_cast<size_t>(part)];

            if (part < index) {
                wanted += '/';
            }
        }

        // "C:" alone names the current directory on that drive, not its root.
        go(wanted.size() == 2 && wanted[1] == ':' ? wanted + "/" : wanted);
    }

    void FilePicker::choose_marked() {
        choose(_marked);
    }

    std::string FilePicker::start_directory(const std::string &key) const {
        std::string remembered;

        if (_memory.directory) {
            remembered = _memory.directory(key);
        } else if (const auto found = _remembered.find(key); found != _remembered.end()) {
            remembered = found->second;
        }

        std::error_code code;

        if (!remembered.empty() && std::filesystem::is_directory(remembered, code)) {
            return Format::from_path(remembered);
        }

        const std::filesystem::path home = Paths::home_directory();

        return Format::from_path(home.empty() ? std::filesystem::current_path(code) : home);
    }

    void FilePicker::remember_directory(const std::string &key, const std::string &path) {
        if (path.empty()) {
            return;
        }

        if (_memory.remember) {
            _memory.remember(key, path);
        } else {
            _remembered[key] = path;
        }
    }

    void FilePicker::start(const std::string &title, const std::vector<std::string> &filters, const bool directories,
                           const bool folders, const bool multiple, const std::string &remember, Chosen chosen,
                           const std::string &option, const std::string &optionHint) {
        _chosen = std::move(chosen);
        _remember = remember;
        _filters = filters.empty() ? std::vector<std::string>{"*"} : filters;
        _suffixes.clear();
        _patterns.clear();
        _anything = false;

        for (const std::string &filter : _filters) {
            _patterns.push_back(Text::lower(filter));

            if (filter == "*" || filter == "*.*") {
                _anything = true;
            } else if (filter.starts_with('*')) {
                _suffixes.push_back(Text::lower(filter.substr(1)));
            }
        }
        _directories = directories;
        _multiple = multiple;
        _marked.clear();

        State &state = _state;

        state.title = title;
        state.directories = directories;
        state.folders = folders;
        state.multiple = multiple;
        state.option = option;
        state.optionHint = optionHint;
        state.optionSet = false;
        state.hiddenShown = hidden();
        state.editing = false;
        state.saving = _saving;
        state.open = true;

        go(start_directory(_remember));
    }

    void FilePicker::go(const std::string &path) {
        // A multi-pick's marks survive navigation.
        if (!_multiple) {
            _marked.clear();
        }

        _drives = false;
        _path = forward(path);
        _parts = split(_path);

        walk();
        push();
        show_target();
    }

    void FilePicker::show_hidden(const bool value) {
        if (_memory.showHidden) {
            _memory.showHidden(value);
        } else {
            _hidden = value;
        }

        _state.hiddenShown = value;

        if (!_drives) {
            walk();
        }

        push();
    }

    void FilePicker::show_drives() {
        _drives = true;
        _entries.clear();

        for (const std::string &drive : Desktop::drives()) {
            _entries.push_back(Entry{
                .name = drive, .path = drive, .directory = true, .hidden = false, .key = drive,});
        }

        push();
    }

    // A filter is a glob over the name: * for any run, ? for one character, and the
    // case of neither side counts.
    bool FilePicker::matches(const std::string_view pattern, const std::string_view name) {
        size_t at = 0;
        size_t seen = 0;
        size_t star = std::string_view::npos;
        size_t back = 0;

        while (seen < name.size()) {
            if (at < pattern.size() && (pattern[at] == '?' || pattern[at] == name[seen])) {
                ++at;
                ++seen;
            } else if (at < pattern.size() && pattern[at] == '*') {
                star = at++;
                back = seen;
            } else if (star != std::string_view::npos) {
                at = star + 1;
                seen = ++back;
            } else {
                return false;
            }
        }

        while (at < pattern.size() && pattern[at] == '*') {
            ++at;
        }

        return at == pattern.size();
    }

    bool FilePicker::wanted(const std::string &name) const {
        if (_anything) {
            return true;
        }

        return std::ranges::any_of(_patterns, [&name](const std::string &pattern) {
            return matches(pattern, name);
        });
    }

    void FilePicker::walk() {
        _entries.clear();

        std::vector<Entry> directories;
        std::vector<Entry> files;
        std::error_code code;
        const bool showHidden = hidden();

        for (std::filesystem::directory_iterator step(_path, code), end;
             step != end && !code; step.increment(code)) {
            std::error_code asked;
            const std::filesystem::path &path = step->path();
            const bool aside = concealed(*step);

            if (aside && !showHidden) {
                continue;
            }

            std::string name = path.filename().string();
            std::string key = Text::lower(name);

            Entry entry{
                .name = std::move(name),
                .path = Format::from_path(path),
                .directory = step->is_directory(asked),
                .hidden = aside,
                .key = std::move(key),
            };

            if (entry.directory) {
                directories.push_back(std::move(entry));
            } else if (!_directories && wanted(entry.key)) {
                files.push_back(std::move(entry));
            }
        }

        const auto byName = [](const Entry &left, const Entry &right) {
            return Text::natural_less(left.key, right.key);
        };

        std::ranges::sort(directories, byName);
        std::ranges::sort(files, byName);

        _entries = std::move(directories);
        _entries.insert(_entries.end(), std::make_move_iterator(files.begin()),
                        std::make_move_iterator(files.end()));
    }

    void FilePicker::mark(const std::string &path) {
        const auto found = std::ranges::find(_marked, path);
        const bool already = found != _marked.end();

        if (_multiple) {
            if (already) {
                _marked.erase(found);
            } else {
                _marked.push_back(path);
            }
        } else {
            _marked.clear();

            if (!already) {
                _marked.push_back(path);
            }
        }

        push();
    }

    void FilePicker::choose(const std::vector<std::string> &paths) {
        // Copies: dismiss() clears _marked, which is what choose_marked passes in.
                // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
        const std::vector<std::string> chosen = paths;
        const bool option = _state.optionSet;
        const Chosen tell = _chosen;

        remember_directory(_remember, _path);
        dismiss();

                if (tell && !chosen.empty()) {
            tell(chosen, option);
        }
    }

    std::string FilePicker::target(const std::string &name) const {
        const std::string bare = tidy(
            Text::trim(std::filesystem::path(forward(Text::trim(name))).filename().string()));

        if (bare.empty()) {
            return {};
        }

        const bool suffixed = _anything || _suffixes.empty()
            || std::ranges::any_of(_suffixes, [&bare](const std::string &suffix) {
                   return Text::iends_with(bare, suffix);
               });

        return Format::from_path(std::filesystem::path(_path)
                                / (suffixed ? bare : bare + _suffixes.front()));
    }

    void FilePicker::suggest(const std::string &name) {
        State &state = _state;

        _saveName = name;

        state.name = name;
        state.nameStem = static_cast<int>(std::filesystem::path(name).stem().string().size());
        state.nameSeed++;

        show_target();
    }

    void FilePicker::show_target() {
        State &state = _state;
        const std::string wanted = target(_saveName);
        std::error_code code;

        state.target = wanted;
        state.replacing = !wanted.empty() && std::filesystem::is_regular_file(wanted, code);

        touch();
    }

    void FilePicker::save(const std::string &name, const bool replacing) {
        const std::string wanted = target(name);
        std::error_code code;

        if (wanted.empty()) {
            _notifier->warning("Give the file a name first.");

            return;
        }

        if (!replacing && std::filesystem::is_regular_file(wanted, code)) {
            _notifier->warning(std::filesystem::path(wanted).filename().string()
                               + " is already there. Use Replace to write over it.");

            return;
        }

        choose({wanted});
    }

    void FilePicker::typed(const std::string &path) {
        State &state = _state;
        const std::string text = Text::trim(path);
        std::error_code code;

        if (text.empty()) {
            state.editing = false;

            touch();

            return;
        }

        const std::string wanted = resolve(text);

        if (std::filesystem::is_directory(wanted, code)) {
            state.editing = false;

            go(wanted);

            return;
        }

        if (!_directories && std::filesystem::is_regular_file(wanted, code)) {
            state.editing = false;

            const std::filesystem::path file(wanted);

            if (_saving) {
                go(Format::from_path(file.parent_path()));
                suggest(file.filename().string());

                return;
            }

            choose({wanted});

            return;
        }

        _notifier->warning(wanted + " is not there.");
    }

    void FilePicker::dismiss() {
        State &state = _state;

        // Escape ends editing first, then closes the state.
        if (state.editing) {
            state.editing = false;

            touch();

            return;
        }

        state.open = false;
        _multiple = false;
        _marked.clear();

        touch();
    }

    void FilePicker::push() {
        State &state = _state;
        std::vector<Row> rows;

        rows.reserve(_entries.size());

        for (const Entry &entry : _entries) {
            rows.push_back(Row{
                .name = entry.name,
                .path = entry.path,
                .directory = entry.directory,
                .hidden = entry.hidden,
                .marked = std::ranges::find(_marked, entry.path) != _marked.end(),
            });
        }

        int folders = 0;

        for (const std::string &path : _marked) {
            std::error_code code;

            if (std::filesystem::is_directory(path, code)) {
                ++folders;
            }
        }

        state.entries = std::move(rows);
        state.markedFolders = folders;
        state.path = _path;
        state.parts = _parts;
        state.rooted = rooted(_path);
        state.drives = _drives;
        state.marked = static_cast<int>(_marked.size());

        if (_drives) {
            state.nothing = "No drives found";
        } else if (_directories) {
            state.nothing = "No directories here";
        } else {
            state.nothing = "Nothing here matches " + Text::join(_filters, ", ");
        }

        touch();
    }
}
