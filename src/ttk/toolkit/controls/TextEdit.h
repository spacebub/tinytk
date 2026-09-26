// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_CONTROLS_TEXTEDIT_H
#define TTK_CONTROLS_TEXTEDIT_H


#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "ttk/toolkit/Widget.h"
#include "ttk/toolkit/controls/GlyphButton.h"
#include "ttk/toolkit/overlays/Menu.h"

namespace ttk {
    // Many lines of fixed width text, with a line number down the left and wrapping
    // at the edge if asked. It scrolls itself both ways, so it stands on its own
    // rather than in a Scroll. The caret is a byte offset into the text.
    class TextEdit : public Widget {
    public:
        // A cog in the corner opens a menu of the settings the reader can change.
        enum class Cog : std::uint8_t {
            Hidden,
            Shown,
        };

        explicit TextEdit(std::function<void(const std::string &)> edited = {}, Cog cog = Cog::Hidden);
        ~TextEdit() override;

        TextEdit(const TextEdit &) = delete;
        TextEdit &operator=(const TextEdit &) = delete;
        TextEdit(TextEdit &&) = delete;
        TextEdit &operator=(TextEdit &&) = delete;

        [[nodiscard]] const std::string &text() const { return _text; }

        // Starts over: the caret goes home and nothing can be undone past this.
        void set_text(std::string text);

        TextEdit *read_only(bool value = true);

        // A line number down the left of every line.
        TextEdit *numbered(bool value = true);

        // Lines broken at the edge, on a space where there is one, instead of
        // scrolling sideways.
        TextEdit *wrapped(bool value = true);

        [[nodiscard]] bool editable() const { return !_readOnly; }

        TextEdit *placeholder(std::string text);

        [[nodiscard]] std::string selection() const;

        void select_all();

        double natural_width(Typeface &type) override;
        double natural_height(Typeface &type, double width) override;

        void arrange(Typeface &type) override;

        void paint(const Painter &painter) override;

        [[nodiscard]] Cursor cursor_at(double x, double y) const override;

        bool press(const Pointer &at) override;
        void drag(const Pointer &at) override;
        void release(const Pointer &at) override;

        bool wheel(double steps, const Pointer &at) override;

        bool key(const Key &pressed) override;
        void wrote(const std::string &text) override;

        [[nodiscard]] bool takes_focus() const override { return enabled(); }
        void gained_focus() override;
        void lost_focus() override;

        bool advance(double now) override;

    private:
        // One change, small enough to keep and to take back whatever the text's size.
        struct Edit {
            size_t at = 0;
            std::string gone;
            std::string came;

            // Where the caret and anchor were before it.
            size_t caret = 0;
            size_t anchor = 0;
        };

        enum class Setting : std::uint8_t {
            Numbers,
            Wrap,
        };

        void measure(Typeface &type);

        // Where each line starts, how many columns each has and where its rows begin,
        // from scratch.
        void reindex();

        // The same for the lines a change touched, the rest moved along by it.
        void reindex(size_t from, size_t gone, size_t came);

        void widen(size_t fromLine, size_t toLine, bool shrank);

        // The rows of every line again, for a width that changed.
        void rewrap();

        // Where `line` breaks into rows at the width held, appended to `into`.
        void wrap_line(size_t line, std::vector<size_t> &into) const;

        // Takes the width on again, and wraps to it if that changed.
        void refit();

        [[nodiscard]] size_t line_count() const { return _starts.size(); }
        [[nodiscard]] size_t line_of(size_t offset) const;
        [[nodiscard]] size_t line_end(size_t line) const;

        // A row is what one line of the screen shows: a line, or a piece of one
        // when wrapping.
        [[nodiscard]] size_t row_count() const { return _rowStarts.size(); }
        [[nodiscard]] size_t row_of(size_t offset) const;
        [[nodiscard]] size_t row_end(size_t row) const;

        // Columns, with tabs expanded, which is what the pointer and the arrows go by.
        // A column counts from the start of the line, and a row's base is where it
        // starts counting from.
        [[nodiscard]] size_t column_of(size_t offset) const;
        // The columns from `start`, the beginning of its line, up to `offset`.
        [[nodiscard]] size_t columns_from(size_t start, size_t offset) const;
        [[nodiscard]] size_t base_of(size_t row) const;
        [[nodiscard]] size_t offset_at(size_t row, size_t column) const;

        // The row as drawn, tabs expanded, into `_shown`.
        [[nodiscard]] const std::string &expand(size_t row);

        [[nodiscard]] size_t before(size_t at) const;
        [[nodiscard]] size_t after(size_t at) const;
        [[nodiscard]] size_t word_left(size_t at) const;
        [[nodiscard]] size_t word_right(size_t at) const;

        [[nodiscard]] double gutter_width() const;
        [[nodiscard]] double text_left() const;
        [[nodiscard]] double text_width() const;
        [[nodiscard]] double view_top() const;
        [[nodiscard]] double view_height() const;
        [[nodiscard]] double farthest_x() const;
        [[nodiscard]] double farthest_y() const;
        [[nodiscard]] bool over_lane(double x) const;
        [[nodiscard]] bool over_rail(double x, double y) const;

        [[nodiscard]] size_t hit(double x, double y) const;

        // The selection in order, or the caret twice over.
        void span(size_t &from, size_t &to) const;

        void move_to(size_t offset, bool selecting, bool keepGoal = false);
        void keep_caret();
        void clamp_scroll();

        void remember(size_t from, size_t to, const std::string &with, bool typing);
        void restore(std::vector<Edit> &from, std::vector<Edit> &to);
        void replace(size_t from, size_t to, const std::string &with, bool typing = false);
        void insert(const std::string &what, bool typing = false);

        void show_settings();

        std::string _text;
        std::string _placeholder;
        std::function<void(const std::string &)> _edited;

        std::vector<size_t> _starts{0};
        std::vector<size_t> _columns{0};
        std::vector<size_t> _rowStarts{0};
        size_t _widest = 0;
        std::string _shown;

        // Columns a row holds when wrapping. Zero until the width is known.
        size_t _fit = 0;

        size_t _caret = 0;
        size_t _anchor = 0;

        // The column in its row that up and down aim for, kept across short rows.
        size_t _goal = 0;

        bool _readOnly = false;
        bool _numbered = false;
        bool _wrapped = false;

        double _scrollX = 0.0;
        double _scrollY = 0.0;

        double _charWidth = 0.0;
        double _lineHeight = 0.0;
        double _fontHeight = 0.0;

        std::vector<Edit> _undo;
        std::vector<Edit> _redo;

        // A run of typing is one step back, until the caret moves some other way.
        bool _typing = false;

        double _lastClick = -1.0;
        double _lastX = 0.0;
        double _lastY = 0.0;
        int _clicks = 0;

        // The bar down the right or the one along the bottom, held by the pointer.
        bool _thumbDrag = false;
        bool _railDrag = false;
        double _grabAt = 0.0;
        double _grabScroll = 0.0;

        GlyphButton *_cog = nullptr;
        Menu *_menu = nullptr;

        // Blinks while focused, which is the one thing that keeps the loop awake.
        double _blinked = 0.0;
        bool _showCaret = true;
    };
}


#endif //TTK_CONTROLS_TEXTEDIT_H
