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
#include <string_view>
#include <vector>

#include "ttk/toolkit/Widget.h"
#include "ttk/toolkit/controls/GlyphButton.h"
#include "ttk/toolkit/overlays/Menu.h"

namespace ttk {
//! Multi-line text editor in a fixed-width face, with optional line numbers and word wrap.
//!
//! The editor scrolls itself in both directions and draws its own scroll bars, so it is not placed inside a
//! \ref Scroll. The text is UTF-8 with `\n` line breaks. Carriage returns are removed from everything that
//! enters it. Tab stops are every 4 columns, and the caret moves by whole code points.
class TextEdit : public Widget {
public:
    //! Whether the editor shows a settings button in its top-right corner.
    enum class Cog : std::uint8_t {
        //! No settings button.
        Hidden,
        //! A cog button that opens a menu toggling line numbers and word wrap.
        Shown,
    };

    //! A run of a logical line whose text is drawn in a colour of its own, given by \ref Decorator::inks().
    struct Ink {
        //! Byte offset in the line where the run starts.
        size_t from = 0;
        //! Byte offset in the line just past the run.
        size_t to = 0;
        //! Colour of the run's text.
        BLRgba32 tone{};
    };

    //! Byte range of the selection in the text, with `from` never after `to`.
    struct Range {
        //! Offset of the first selected byte.
        size_t from = 0;
        //! Offset just past the last selected byte, equal to `from` when nothing is selected.
        size_t to = 0;
    };

    //! What an application attaches through \ref decorate() to colour, tint, mark or replace lines.
    //!
    //! Every question is about one logical line, counted from 0, and is asked while the editor paints, only for
    //! lines that have a row being repainted. Colours are taken as they are answered, so a decorator that reads
    //! them from \ref Theme::palette() each time follows theme changes. The default answers decorate nothing.
    //! After its own state changes, the application calls \ref Widget::invalidate() on the editor.
    class Decorator {
    public:
        Decorator() = default;
        virtual ~Decorator() = default;

        Decorator(const Decorator &) = default;
        Decorator &operator=(const Decorator &) = default;
        Decorator(Decorator &&) = default;
        Decorator &operator=(Decorator &&) = default;

        //! Called when the text from the start of `line` on has changed, so whatever was worked out for that line
        //! and every line after it is stale.
        //!
        //! Called after every edit with the first line it touched, with the text already changed and before the
        //! `edited` callback, and with 0 from \ref TextEdit::set_text() and \ref TextEdit::decorate().
        virtual void changed(size_t /*line*/) {}

        //! Appends to `out`, which arrives empty, the runs of `line` whose text is drawn in a colour of their own.
        //!
        //! `text` is the line without its line break. The runs are in order and do not overlap. Text no run covers
        //! keeps the editor's text colour. Asked once per paint for each line with a row being repainted.
        virtual void inks(size_t /*line*/, std::string_view /*text*/, std::vector<Ink> & /*out*/) {}

        //! Returns the colour filled behind every row of `line`, or a fully transparent one for none.
        //!
        //! The fill spans the text area and lies under the selection and the text.
        [[nodiscard]] virtual BLRgba32 wash(size_t /*line*/) { return BLRgba32{}; }

        //! Returns the colour of a bar beside every row of `line`, or a fully transparent one for none.
        //!
        //! The bar is 3 pixels wide and as tall as the rows, so marks on neighbouring lines join into one bar. It
        //! sits between the line numbers and the text in a numbered editor, and along the left edge otherwise.
        [[nodiscard]] virtual BLRgba32 mark(size_t /*line*/) { return BLRgba32{}; }

        //! Draws the row of `line` in `row` instead of its text, and returns true when it did.
        //!
        //! Asked for each row being repainted, with `row` the text area of that row: from the left edge of the
        //! text, unscrolled, across the width the text has, and one row tall. The wash is drawn before it, and
        //! the caret row's tint, the selection and the caret after it. Returning false draws the text as usual.
        virtual bool draw(size_t /*line*/, const Painter & /*painter*/, const BLRect & /*row*/) { return false; }
    };

    //! Creates an empty, editable editor that reports edits to `edited`, with a settings button when `cog` is
    //! \ref Cog::Shown.
    //!
    //! `edited` is called with the whole text after every change made from the keyboard, the clipboard, undo
    //! or redo, but not after \ref set_text(). It may be null.
    explicit TextEdit(std::function<void(const std::string &)> edited = {}, Cog cog = Cog::Hidden);

    //! Closes the settings menu when it is open.
    ~TextEdit() override;

    TextEdit(const TextEdit &) = delete;
    TextEdit &operator=(const TextEdit &) = delete;
    TextEdit(TextEdit &&) = delete;
    TextEdit &operator=(TextEdit &&) = delete;

    //! Returns the text, in UTF-8 with `\n` line breaks.
    [[nodiscard]] const std::string &text() const { return _text; }

    //! Replaces the text with `text`, without carriage returns, and repaints.
    //!
    //! The caret moves to the start, the view scrolls to the top-left corner and the undo and redo history is
    //! cleared. The `edited` callback is not called.
    void set_text(std::string text);

    //! Sets whether the text is protected from editing, and returns this.
    //!
    //! A read-only editor still takes focus, selects, copies and scrolls, but shows no caret and ignores
    //! typing, pasting, cutting, deleting, undo and redo.
    TextEdit *read_only(bool value = true);

    //! Sets whether a gutter on the left shows the number of each line, and returns this.
    //!
    //! With wrapping on, only the first row of a line carries its number.
    TextEdit *numbered(bool value = true);

    //! Sets whether lines wider than the view are wrapped instead of scrolled sideways, and returns this.
    //!
    //! A line breaks after the last space that fits in the row, or at the edge when the row has no space.
    TextEdit *wrapped(bool value = true);

    //! Sets whether the editor is drawn without its own body, border and gutter shading, on the window's
    //! background, for an editor that is the page itself rather than a field on it. Returns this.
    TextEdit *bare(bool value = true);

    //! Tests whether the text can be edited, which is when \ref read_only() is not set.
    [[nodiscard]] bool editable() const { return !_readOnly; }

    //! Sets the text shown in a faint colour while the editor is empty, and returns this.
    TextEdit *placeholder(std::string_view text);

    //! Returns the selected text, or an empty string when nothing is selected.
    [[nodiscard]] std::string selection() const;

    //! Selects the whole text, with the caret at the end.
    void select_all();

    //! Returns the byte offset of the caret.
    [[nodiscard]] size_t caret() const { return _caret; }

    //! Returns the byte range of the selection, empty at the caret when nothing is selected.
    [[nodiscard]] Range selected() const;

    //! Selects from `anchor` to `caret`, byte offsets clamped to the text and moved back onto the start of a
    //! character, and scrolls the caret into view. Equal offsets place the caret with nothing selected.
    //!
    //! A caret out of view is scrolled to the middle of it. Before the editor has a size, the scroll waits for
    //! its first layout.
    void select(size_t anchor, size_t caret);

    //! Places the caret at the start of `line`, clamped to the last line, as \ref select() does.
    void go_to(size_t line);

    //! Replaces the selection with `text`, in UTF-8, or inserts it at the caret, as one step of undo.
    //!
    //! Behaves as typing does otherwise: carriage returns are removed, the caret ends after `text` and the
    //! `edited` callback is called. Ignored when read-only.
    void insert(std::string_view text);

    //! Sets the function called whenever the caret or the selection changes, and returns this.
    //!
    //! Called once for each change, from the pointer, the keyboard, an edit or a call such as \ref select(),
    //! after the caret has moved and before the `edited` callback of an edit. Read the new place through
    //! \ref caret() and \ref selected(). A null `moved` stops the calls.
    TextEdit *on_select(std::function<void()> moved);

    //! Sets the function called whenever the view scrolls, by the wheel, the keyboard, a drag of a scroll bar or
    //! the caret being kept in sight, and returns this. Called after the scroll has changed. A null `scrolled` stops
    //! the calls.
    TextEdit *on_scroll(std::function<void()> scrolled);

    //! Returns the box the character at byte offset `offset` takes on screen, one character wide and one row tall,
    //! in window coordinates. It lies outside \ref text_box() when the character is scrolled out of view, and is
    //! empty before the editor has a size.
    [[nodiscard]] BLRect spot(size_t offset) const;

    //! Returns the part of the editor the text is shown in, right of the gutter, in window coordinates.
    [[nodiscard]] BLRect text_box() const;

    //! Attaches `decorator`, which colours, tints, marks or replaces lines, and returns this. Null detaches it.
    //!
    //! The editor does not own the decorator, which must outlive it or be detached first. Its
    //! \ref Decorator::changed() is called with 0 and the editor repaints. Without one, painting asks nothing.
    TextEdit *decorate(Decorator *decorator);

    //! Returns the number of logical lines, which is one more than the number of line breaks.
    [[nodiscard]] size_t line_count() const { return _starts.size(); }

    //! Returns the logical line holding the byte offset `offset`, counted from 0. Past the end it is the last.
    [[nodiscard]] size_t line_of(size_t offset) const;

    //! Returns the byte offset where `line` starts, or the end of the text past the last line.
    [[nodiscard]] size_t line_start(size_t line) const;

    //! Returns the byte offset of the line break that ends `line`, or the end of the text for the last line.
    [[nodiscard]] size_t line_end(size_t line) const;

    //! Returns \ref Widget::fixedWidth when it is set, otherwise 240 pixels.
    double natural_width(Typeface &type) override;

    //! Returns \ref Widget::fixedHeight when it is set, otherwise 180 pixels.
    double natural_height(Typeface &type, double width) override;

    //! Places the settings button and wraps the text again when the width available to it has changed.
    void arrange(Typeface &type) override;

    //! Paints the frame, the gutter, the visible rows, the selection, the caret and the scroll bars.
    //!
    //! The selection stays visible, in a lighter tint, while the editor does not have focus.
    void paint(const Painter &painter) override;

    //! Returns the default cursor over the scroll bars and the line number gutter, and the text cursor
    //! elsewhere.
    [[nodiscard]] Cursor cursor_at(double x, double y) const override;

    //! Handles a press on a scroll bar, which starts dragging it, or on the text, which takes focus and
    //! places the caret.
    //!
    //! Shift extends the selection. A second press within 0.4 seconds and 4 pixels of the last selects a
    //! word, and a third selects the whole line with its line break. Returns false when the editor is
    //! disabled or not attached to a root.
    bool press(const Pointer &at) override;

    //! Drags the held scroll bar, or extends the selection to `at` after a single press.
    void drag(const Pointer &at) override;

    //! Ends a scroll bar drag.
    void release(const Pointer &at) override;

    //! Scrolls by 3 rows per notch of `steps`, or sideways by 3 columns per notch while Shift is held.
    //!
    //! Returns false, leaving the scroll to the parent, when there is nothing to scroll in that direction.
    bool wheel(double steps, const Pointer &at) override;

    //! Handles editing and caret keys and returns true when the key was consumed.
    //!
    //! Arrows move by a character or a row, Ctrl+Left and Ctrl+Right by a word, PageUp and PageDown by a view
    //! less one row. Home goes to the first non-blank character of the row, then to the row's start. End goes
    //! to the row's end. Ctrl+Home and Ctrl+End go to either end of the text. Shift extends the selection.
    //! Backspace and Delete remove the selection, or else a character, or a word with Ctrl. Return inserts a line
    //! break that keeps the current line's indent. Tab inserts spaces up to the next tab stop. Ctrl+A,
    //! Ctrl+C, Ctrl+X and Ctrl+V select all, copy, cut and paste. Ctrl+Z undoes, and Ctrl+Shift+Z and Ctrl+Y
    //! redo. A run of typing undoes as one step, and up to 256 steps are kept. Tab with any modifier, and Return
    //! and Tab while read-only, are not consumed, so Shift+Tab and Ctrl+Tab leave the editor.
    bool key(const Key &pressed) override;

    //! Replaces the selection with `text`, in UTF-8, or inserts it at the caret. Ignored when read-only.
    void wrote(const std::string &text) override;

    //! Tests whether the editor shows its own focus, which it does with the caret and the accent outline.
    [[nodiscard]] bool paints_focus() const override { return true; }

    //! Called when the editor receives focus. Shows the caret, starts it blinking and repaints.
    void gained_focus() override;

    //! Called when the editor loses focus. Repaints it. The selection is kept.
    void lost_focus() override;

    //! Blinks the caret while the editor has focus and is editable, sleeping between blinks. Returns false
    //! otherwise.
    bool advance(double now) override;

private:
    struct Edit {
        size_t at = 0;
        std::string gone;
        std::string came;

        size_t caret = 0;
        size_t anchor = 0;
    };

    enum class Setting : std::uint8_t {
        Numbers,
        Wrap,
    };

    void measure(Typeface &type);

    void reindex();

    void reindex(size_t from, size_t gone, size_t came);

    void widen(size_t fromLine, size_t toLine, bool shrank);

    void rewrap();

    void wrap_line(size_t line, std::vector<size_t> &into) const;

    void refit();

    [[nodiscard]] size_t row_count() const { return _rowStarts.size(); }
    [[nodiscard]] size_t row_of(size_t offset) const;
    [[nodiscard]] size_t row_end(size_t row) const;

    [[nodiscard]] size_t column_of(size_t offset) const;
    [[nodiscard]] size_t columns_from(size_t start, size_t offset) const;
    [[nodiscard]] size_t base_of(size_t row) const;
    [[nodiscard]] size_t offset_at(size_t row, size_t column) const;

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

    [[nodiscard]] BLRect text_clip(double gutter) const;
    [[nodiscard]] BLRect caret_box(size_t row, double left) const;

    void span(size_t &from, size_t &to) const;
    void invalidate_caret() const;

    void move_to(size_t offset, bool selecting, bool keepGoal = false);
    void keep_caret();
    void centre_caret();
    void clamp_scroll();
    void tell_scrolled();
    void tell_moved();
    void touched(size_t from);

    void paint_marks(const Painter &painter, size_t first, size_t last) const;
    void ink_row(size_t row, size_t base, size_t lineStart);

    void remember(size_t from, size_t to, const std::string &with, bool typing);
    void restore(std::vector<Edit> &from, std::vector<Edit> &to);
    void replace(size_t from, size_t to, const std::string &with, bool typing = false);
    void put(const std::string &what, bool typing = false);

    void show_settings();

    std::string _text;
    std::string _placeholder;
    std::function<void(const std::string &)> _edited;

    std::vector<size_t> _starts{0};
    std::vector<size_t> _columns{0};
    std::vector<size_t> _rowStarts{0};
    size_t _widest = 0;
    std::string _shown;

    // Zero until the width is known.
    size_t _fit = 0;

    size_t _caret = 0;
    size_t _anchor = 0;

    // The column in its row that up and down aim for, kept across short rows.
    size_t _goal = 0;

    bool _readOnly = false;
    bool _numbered = false;
    bool _wrapped = false;
    bool _bare = false;

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

    bool _thumbDrag = false;
    bool _railDrag = false;
    double _grabAt = 0.0;
    double _grabScroll = 0.0;

    GlyphButton *_cog = nullptr;
    Menu *_menu = nullptr;

    double _blinked = 0.0;
    bool _showCaret = true;

    std::function<void()> _moved;
    std::function<void()> _scrolled;
    double _toldScrollX = 0.0;
    double _toldScrollY = 0.0;
    size_t _toldCaret = 0;
    size_t _toldAnchor = 0;

    // A select() made before the editor had a size scrolls at its first layout.
    bool _centrePending = false;

    Decorator *_decorator = nullptr;

    // Kept between paints, so a decorated paint allocates nothing.
    std::vector<Ink> _inks;
    std::vector<Typeface::Band> _bands;
};
}


#endif //TTK_CONTROLS_TEXTEDIT_H
