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

namespace ttk {
    namespace Json {

        // Lenient conversions: numbers and booleans a hand edited config wrote as strings are accepted.
        std::string as_string(const yyjson_val *val, const std::string &def = {});
        int as_int(const yyjson_val *val, int def = 0);
        bool as_bool(const yyjson_val *val, bool def = false);
        std::vector<std::string> as_string_list(const yyjson_val *val);
        bool as_int_array(const yyjson_val *val, int *out, int count);

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

        yyjson_val *obj_get(yyjson_val *obj, const char *key);
        std::string obj_get_string(yyjson_val *obj, const char *key, const std::string &def = {});
        int obj_get_int(yyjson_val *obj, const char *key, int def = 0);

        class Doc {
        public:
            Doc() = default;

            explicit Doc(yyjson_doc *doc) : _doc(doc) {}

            Doc(yyjson_doc *doc, std::string text) : _doc(doc), _text(std::move(text)) {}

            ~Doc();

            Doc(const Doc &) = delete;

            Doc &operator=(const Doc &) = delete;

            Doc(Doc &&other) noexcept;

            Doc &operator=(Doc &&other) noexcept;

            [[nodiscard]] bool valid() const {
                return _doc != nullptr;
            }

            [[nodiscard]] yyjson_val *root() const;

        private:
            yyjson_doc *_doc{nullptr};

            // Backing text of an in-situ parse. The document's strings point into it.
            std::string _text;
        };

        Doc read_file(const std::filesystem::path &path, std::string *error = nullptr);
        Doc read_data(const std::string &data, std::string *error = nullptr);

        class Builder {
        public:
            Builder();

            ~Builder();

            Builder(const Builder &) = delete;

            Builder &operator=(const Builder &) = delete;

            Builder(Builder &&) = delete;

            Builder &operator=(Builder &&) = delete;

            [[nodiscard]] yyjson_mut_val *new_object() const;

            [[nodiscard]] yyjson_mut_val *new_array() const;

            void set_root(yyjson_mut_val *val) const;

            // Keys and values are referenced, not copied. They must outlive write_file().
            void add_string(yyjson_mut_val *obj, const char *key, std::string_view value) const;
            void add_int(yyjson_mut_val *obj, const char *key, int value) const;
            void add_bool(yyjson_mut_val *obj, const char *key, bool value) const;
            void add_value(yyjson_mut_val *obj, const char *key, yyjson_mut_val *value) const;

            void append_string(yyjson_mut_val *arr, std::string_view value) const;
            void append_int(yyjson_mut_val *arr, int value) const;
            static void append_value(yyjson_mut_val *arr, yyjson_mut_val *value);

            bool write_file(const std::filesystem::path &path, std::string *error = nullptr) const;

        private:
            [[nodiscard]] yyjson_mut_val *key_of(const char *key) const;

            yyjson_mut_doc *_doc;
        };

    }
}


#endif //TTK_SYSTEM_JSON_H
