// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_SYSTEM_JSON_H
#define TTK_SYSTEM_JSON_H


#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <yyjson.h>

//! Reading and writing JSON documents through yyjson, with lenient conversions suited to hand edited config files.
namespace ttk::Json {

    //! Returns the string held by `val`, or `def` when `val` is null or not a string.
    std::string as_string(const yyjson_val *val, const std::string &def = {});

    //! Returns `val` as an `int`, or `def` when it cannot be read as one.
    //!
    //! Integers are returned as is, reals are truncated toward zero, and strings are parsed with \ref Text::to_int(),
    //! which returns `def` when the string is not a number. Null and every other type return `def`.
    int as_int(const yyjson_val *val, int def = 0);

    //! Returns `val` as a `bool`, or `def` when it cannot be read as one.
    //!
    //! Booleans are returned as is and integers are true when non-zero. A string is true when it is `1` or `true`
    //! in any case, and false otherwise. Null and every other type return `def`.
    bool as_bool(const yyjson_val *val, bool def = false);

    //! Returns the string items of the array `val` in order. Items that are not strings are skipped, and a null or
    //! non-array `val` gives an empty list.
    std::vector<std::string> as_string_list(const yyjson_val *val);

    //! Reads the first `count` items of the array `val` into `out` as integers, truncating reals, and returns true.
    //!
    //! Returns false when `val` is null, not an array, shorter than `count`, or one of those items is not a number.
    //! Items before a failing one have already been written to `out`. Extra items are ignored.
    bool as_int_array(const yyjson_val *val, int *out, int count);

    //! Calls `visit` with the key, as a `std::string_view`, and the value of each field of the object `obj` in
    //! document order. Does nothing when `obj` is null or not an object.
    template<class F>
    void each_field(yyjson_val *obj, F &&visit) {
        if (obj == nullptr || !yyjson_is_obj(obj)) {
            return;
        }

        size_t idx = 0;
        size_t max = 0;
        yyjson_val *key = nullptr;
        yyjson_val *val = nullptr;

        yyjson_obj_foreach(obj, idx, max, key, val) {
            visit(std::string_view(yyjson_get_str(key), yyjson_get_len(key)), val);
        }
    }

    //! Calls `visit` with each item of the array `arr` in order. Does nothing when `arr` is null or not an array.
    template<class F>
    void each_item(yyjson_val *arr, F &&visit) {
        if (arr == nullptr || !yyjson_is_arr(arr)) {
            return;
        }

        size_t idx = 0;
        size_t max = 0;
        yyjson_val *item = nullptr;

        yyjson_arr_foreach(arr, idx, max, item) {
            visit(item);
        }
    }

    //! Returns the value of the field `key` of the object `obj`, or null when `obj` is null, not an object, or has no
    //! such field.
    yyjson_val *obj_get(yyjson_val *obj, const char *key);

    //! Returns the field `key` of the object `obj` read by \ref as_string(), or `def` when it is missing.
    std::string obj_get_string(yyjson_val *obj, const char *key, const std::string &def = {});

    //! Returns the field `key` of the object `obj` read by \ref as_int(), or `def` when it is missing.
    int obj_get_int(yyjson_val *obj, const char *key, int def = 0);

    //! Parsed, read-only JSON document that owns its yyjson document. Movable, not copyable.
    //!
    //! Every `yyjson_val` taken from a document points into it and is valid only while the document lives.
    class Doc {
    public:
        //! Creates an empty document, for which \ref valid() returns false.
        Doc() = default;

        //! Creates a document that takes ownership of `doc`, which may be null.
        explicit Doc(yyjson_doc *doc) : _doc(doc) {}

        //! Creates a document that takes ownership of `doc` and of `text`, the buffer `doc` was parsed from in place
        //! and whose strings it points into.
        Doc(yyjson_doc *doc, std::string text) : _doc(doc), _text(std::move(text)) {}

        ~Doc();

        Doc(const Doc &) = delete;

        Doc &operator=(const Doc &) = delete;

        Doc(Doc &&other) noexcept;

        Doc &operator=(Doc &&other) noexcept;

        //! Tests whether the document holds a parsed document.
        [[nodiscard]] bool valid() const {
            return _doc != nullptr;
        }

        //! Returns the root value, or null when the document is not \ref valid().
        [[nodiscard]] yyjson_val *root() const;

    private:
        yyjson_doc *_doc{nullptr};

        // Backing text of an in-situ parse. The document's strings point into it.
        std::string _text;
    };

    //! Parses the file at `path` and returns the document.
    //!
    //! Returns a document that is not \ref Doc::valid() when the file cannot be opened or read or does not hold
    //! valid JSON, and stores a message in `error` when it is not null. A parse error message includes the byte
    //! offset.
    Doc read_file(const std::filesystem::path &path, std::string *error = nullptr);

    //! Parses `data` and returns the document, which does not refer to `data` afterwards.
    //!
    //! Returns a document that is not \ref Doc::valid() when `data` is not valid JSON, and stores a message with the
    //! byte offset in `error` when it is not null.
    Doc read_data(const std::string &data, std::string *error = nullptr);

    //! Builds a mutable JSON document and writes it to a file.
    //!
    //! Values are created by the builder, linked into objects and arrays, and the outermost one is made the root
    //! with \ref set_root(). Every value belongs to the builder and is freed with it. Methods that take an object or
    //! array do nothing when it is null.
    class Builder {
    public:
        //! Creates a builder with an empty document.
        Builder();

        ~Builder();

        Builder(const Builder &) = delete;

        Builder &operator=(const Builder &) = delete;

        Builder(Builder &&) = delete;

        Builder &operator=(Builder &&) = delete;

        //! Returns a new empty object owned by the builder.
        [[nodiscard]] yyjson_mut_val *new_object() const;

        //! Returns a new empty array owned by the builder.
        [[nodiscard]] yyjson_mut_val *new_array() const;

        //! Sets `val` as the root of the document that \ref write_file() writes.
        void set_root(yyjson_mut_val *val) const;

        //! Adds the field `key` with the string `value` to the object `obj`.
        //!
        //! \note Neither `key` nor the bytes of `value` are copied, so both must outlive \ref write_file(). `key`
        //! is written without escaping and must be plain ASCII that needs none.
        void add_string(yyjson_mut_val *obj, const char *key, std::string_view value) const;

        //! Adds the field `key` with the integer `value` to the object `obj`.
        //!
        //! \note `key` is not copied and must outlive \ref write_file(). It is written without escaping and must be
        //! plain ASCII that needs none.
        void add_int(yyjson_mut_val *obj, const char *key, int value) const;

        //! Adds the field `key` with the boolean `value` to the object `obj`.
        //!
        //! \note `key` is not copied and must outlive \ref write_file(). It is written without escaping and must be
        //! plain ASCII that needs none.
        void add_bool(yyjson_mut_val *obj, const char *key, bool value) const;

        //! Adds the field `key` with `value`, an object or array from this builder, to the object `obj`. Does nothing
        //! when `value` is null.
        //!
        //! \note `key` is not copied and must outlive \ref write_file(). It is written without escaping and must be
        //! plain ASCII that needs none.
        void add_value(yyjson_mut_val *obj, const char *key, yyjson_mut_val *value) const;

        //! Appends the string `value` to the array `arr`.
        //!
        //! \note The bytes of `value` are not copied and must outlive \ref write_file().
        void append_string(yyjson_mut_val *arr, std::string_view value) const;

        //! Appends the integer `value` to the array `arr`.
        void append_int(yyjson_mut_val *arr, int value) const;

        //! Appends `value`, a value from the same builder, to the array `arr`. Does nothing when `value` is null.
        static void append_value(yyjson_mut_val *arr, yyjson_mut_val *value);

        //! Writes the document to `path` as JSON indented by two spaces with a trailing newline, and returns true.
        //!
        //! Missing parent directories are created. The text is written to `path` with `.new` appended and then
        //! renamed over `path`, so an existing file is either kept or fully replaced. Returns false and stores a
        //! message in `error`, when it is not null, if any step fails.
        bool write_file(const std::filesystem::path &path, std::string *error = nullptr) const;

    private:
        [[nodiscard]] yyjson_mut_val *key_of(const char *key) const;

        yyjson_mut_doc *_doc;
    };

}


#endif //TTK_SYSTEM_JSON_H
