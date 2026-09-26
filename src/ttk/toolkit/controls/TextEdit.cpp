// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <cmath>
#include <utility>

#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/controls/TextEdit.h"
#include "ttk/util/Clipboard.h"

namespace ttk {
    namespace {

        constexpr double PAD_X = 10.0;
        constexpr double PAD_Y = 8.0;
        constexpr size_t TAB = 4;
        constexpr size_t UNDO_LIMIT = 256;
        constexpr double BLINK = 0.53;
        constexpr double MULTI_CLICK = 0.4;

        bool is_word_char(const char letter) {
            return (letter >= 'a' && letter <= 'z') || (letter >= 'A' && letter <= 'Z')
                || (letter >= '0' && letter <= '9') || letter == '_'
                || static_cast<unsigned char>(letter) >= 0x80;
        }

        bool is_continuation(const char letter) {
            return (static_cast<unsigned char>(letter) & 0xc0) == 0x80;
        }

        std::string without_returns(std::string text) {
            std::erase(text, '\r');

            return text;
        }

    }


    TextEdit::TextEdit(std::function<void(const std::string &)> edited, const Cog cog)
        : _edited(std::move(edited)) {
        _takesPointer = true;
        cursor = Cursor::Text;

        if (cog == Cog::Shown) {
            _cog = append(std::make_unique<GlyphButton>(Glyphs::Glyph::Cog, [this] { show_settings(); }));
            _cog->size(Theme::controlSmall)->tooltip("Line numbers and wrapping");
        }
    }

    TextEdit::~TextEdit() {
        if (_menu != nullptr && root() != nullptr) {
            root()->dismiss();
        }
    }

    void TextEdit::set_text(std::string text) {
        _text = without_returns(std::move(text));
        _caret = 0;
        _anchor = 0;
        _goal = 0;
        _scrollX = 0.0;
        _scrollY = 0.0;
        _undo.clear();
        _redo.clear();
        _typing = false;

        reindex();
        invalidate();
    }

    TextEdit *TextEdit::read_only(const bool value) {
        _readOnly = value;
        invalidate();

        return this;
    }

    TextEdit *TextEdit::numbered(const bool value) {
        _numbered = value;
        refit();

        return this;
    }

    TextEdit *TextEdit::wrapped(const bool value) {
        _wrapped = value;
        _fit = 0;
        _scrollX = 0.0;
        refit();

        return this;
    }

    TextEdit *TextEdit::placeholder(std::string text) {
        _placeholder = std::move(text);

        return this;
    }

    std::string TextEdit::selection() const {
        size_t from = 0;
        size_t to = 0;

        span(from, to);

        return _text.substr(from, to - from);
    }

    void TextEdit::select_all() {
        _anchor = 0;
        _caret = _text.size();

        invalidate();
    }

    void TextEdit::measure(Typeface &type) {
        const BLFont &font = type.at(Typeface::mono, Theme::fontSmall);

        _charWidth = type.width(font, "0000000000") / 10.0;
        _fontHeight = type.line_height(font);
        _lineHeight = std::ceil(_fontHeight + 4.0);
    }

    void TextEdit::reindex() {
        _starts.assign(1, 0);

        for (size_t at = 0; at < _text.size(); ++at) {
            if (_text[at] == '\n') {
                _starts.push_back(at + 1);
            }
        }

        _columns.resize(_starts.size());
        _widest = 0;

        for (size_t line = 0; line < line_count(); ++line) {
            _columns[line] = column_of(line_end(line));
            _widest = std::max(_widest, _columns[line]);
        }

        rewrap();
    }

    void TextEdit::reindex(const size_t from, const size_t gone, const size_t came) {
        const size_t first = line_of(from);

        // Lines that began inside what went are gone with it, and the ones after
        // moved by the difference. The rows of every touched line are made again.
        const auto low = std::ranges::upper_bound(_starts, from);
        const auto high = std::ranges::upper_bound(_starts, from + gone);
        const auto lowLine = static_cast<size_t>(low - _starts.begin());
        const auto highLine = static_cast<size_t>(high - _starts.begin());
        const bool shrank = std::any_of(_columns.begin() + static_cast<std::ptrdiff_t>(first),
                                        _columns.begin() + static_cast<std::ptrdiff_t>(highLine),
                                        [this](const size_t columns) { return columns == _widest; });
        const size_t was = _text.size() + gone - came;
        const size_t touchedEnd = highLine < _starts.size() ? _starts[highLine] - 1 : was;

        const auto rowLow = std::ranges::lower_bound(_rowStarts, _starts[first]);
        const auto rowHigh = std::ranges::upper_bound(_rowStarts, touchedEnd);
        const auto rowAt = static_cast<size_t>(rowLow - _rowStarts.begin());

        _rowStarts.erase(rowLow, rowHigh);

        for (size_t row = rowAt; row < _rowStarts.size(); ++row) {
            _rowStarts[row] = _rowStarts[row] + came - gone;
        }

        _starts.erase(low, high);
        _columns.erase(_columns.begin() + static_cast<std::ptrdiff_t>(lowLine),
                       _columns.begin() + static_cast<std::ptrdiff_t>(highLine));

        for (size_t line = lowLine; line < _starts.size(); ++line) {
            _starts[line] = _starts[line] + came - gone;
        }

        size_t added = 0;

        for (size_t at = from; at < from + came; ++at) {
            if (_text[at] == '\n') {
                _starts.insert(_starts.begin() + static_cast<std::ptrdiff_t>(lowLine + added), at + 1);
                _columns.insert(_columns.begin() + static_cast<std::ptrdiff_t>(lowLine + added), 0);
                ++added;
            }
        }

        widen(first, first + added, shrank);

        std::vector<size_t> fresh;

        for (size_t line = first; line <= first + added; ++line) {
            wrap_line(line, fresh);
        }

        _rowStarts.insert(_rowStarts.begin() + static_cast<std::ptrdiff_t>(rowAt), fresh.begin(), fresh.end());
    }

    void TextEdit::widen(const size_t fromLine, const size_t toLine, const bool shrank) {
        size_t widest = 0;

        for (size_t line = fromLine; line <= toLine; ++line) {
            _columns[line] = column_of(line_end(line));
            widest = std::max(widest, _columns[line]);
        }

        // The widest line may have been one of these and got shorter, and only a look
        // at every line says what is widest now.
        if (shrank && widest < _widest) {
            _widest = *std::ranges::max_element(_columns);
        } else {
            _widest = std::max(_widest, widest);
        }
    }

    void TextEdit::rewrap() {
        _rowStarts.clear();

        for (size_t line = 0; line < line_count(); ++line) {
            wrap_line(line, _rowStarts);
        }
    }

    void TextEdit::wrap_line(const size_t line, std::vector<size_t> &into) const {
        const size_t start = _starts[line];
        const size_t end = line_end(line);

        into.push_back(start);

        if (!_wrapped || _fit == 0) {
            return;
        }

        size_t column = 0;
        size_t base = 0;

        // Just past the last space in the row, and the column there. A row breaks
        // there when it has one, and where it runs out of room when it has not.
        size_t space = 0;
        size_t spaceColumn = 0;

        for (size_t at = start; at < end;) {
            const size_t next = _text[at] == '\t' ? ((column / TAB) + 1) * TAB : column + 1;

            if (next - base > _fit && at > into.back()) {
                if (space > into.back()) {
                    into.push_back(space);
                    base = spaceColumn;
                    column = spaceColumn;
                    at = space;
                    space = 0;

                    continue;
                }

                into.push_back(at);
                base = column;
                space = 0;
            }

            if (_text[at] == ' ') {
                space = at + 1;
                spaceColumn = column + 1;
            }

            column = _text[at] == '\t' ? ((column / TAB) + 1) * TAB : column + 1;
            at = after(at);
        }
    }

    void TextEdit::refit() {
        if (root() != nullptr) {
            measure(root()->type());
        }

        if (_charWidth > 0.0) {
            const auto fit = static_cast<size_t>(std::max(1.0, std::floor(text_width() / _charWidth)));

            if (_wrapped && fit != _fit) {
                _fit = fit;
                rewrap();
            }
        }

        clamp_scroll();
        invalidate();
    }

    size_t TextEdit::line_of(const size_t offset) const {
        return static_cast<size_t>(std::ranges::upper_bound(_starts, offset) - _starts.begin()) - 1;
    }

    size_t TextEdit::line_end(const size_t line) const {
        return line + 1 < line_count() ? _starts[line + 1] - 1 : _text.size();
    }

    size_t TextEdit::row_of(const size_t offset) const {
        return static_cast<size_t>(std::ranges::upper_bound(_rowStarts, offset)
                                   - _rowStarts.begin()) - 1;
    }

    size_t TextEdit::row_end(const size_t row) const {
        if (row + 1 >= row_count()) {
            return _text.size();
        }

        // The next row starts after a newline, or where this one was broken.
        const size_t next = _rowStarts[row + 1];

        return _text[next - 1] == '\n' ? next - 1 : next;
    }

    size_t TextEdit::column_of(const size_t offset) const {
        size_t column = 0;

        for (size_t at = _starts[line_of(offset)]; at < offset; ++at) {
            if (_text[at] == '\t') {
                column = ((column / TAB) + 1) * TAB;
            } else if (!is_continuation(_text[at])) {
                ++column;
            }
        }

        return column;
    }

    size_t TextEdit::base_of(const size_t row) const {
        return column_of(_rowStarts[row]);
    }

    size_t TextEdit::offset_at(const size_t row, const size_t column) const {
        const size_t end = row_end(row);
        size_t at = _rowStarts[row];
        size_t reached = base_of(row);
        const size_t wanted = reached + column;

        while (at < end) {
            const size_t next = _text[at] == '\t' ? ((reached / TAB) + 1) * TAB : reached + 1;

            if (next > wanted) {
                return wanted - reached <= next - wanted ? at : after(at);
            }

            reached = next;
            at = after(at);
        }

        return end;
    }

    const std::string &TextEdit::expand(const size_t row) {
        size_t column = base_of(row);

        _shown.clear();

        for (size_t at = _rowStarts[row], end = row_end(row); at < end; ++at) {
            if (_text[at] == '\t') {
                const size_t next = ((column / TAB) + 1) * TAB;

                _shown.append(next - column, ' ');
                column = next;
            } else {
                _shown.push_back(_text[at]);
                column += is_continuation(_text[at]) ? 0 : 1;
            }
        }

        return _shown;
    }

    size_t TextEdit::before(const size_t at) const {
        if (at == 0) {
            return 0;
        }

        size_t back = at - 1;

        while (back > 0 && is_continuation(_text[back])) {
            --back;
        }

        return back;
    }

    size_t TextEdit::after(const size_t at) const {
        if (at >= _text.size()) {
            return _text.size();
        }

        size_t next = at + 1;

        while (next < _text.size() && is_continuation(_text[next])) {
            ++next;
        }

        return next;
    }

    size_t TextEdit::word_left(size_t at) const {
        while (at > 0 && !is_word_char(_text[at - 1]) && _text[at - 1] != '\n') {
            at = before(at);
        }

        if (at > 0 && _text[at - 1] == '\n') {
            return at - 1;
        }

        while (at > 0 && is_word_char(_text[at - 1])) {
            at = before(at);
        }

        return at;
    }

    size_t TextEdit::word_right(size_t at) const {
        while (at < _text.size() && !is_word_char(_text[at]) && _text[at] != '\n') {
            at = after(at);
        }

        if (at < _text.size() && _text[at] == '\n' && (at == 0 || !is_word_char(_text[at - 1]))) {
            return at + 1;
        }

        while (at < _text.size() && is_word_char(_text[at])) {
            at = after(at);
        }

        return at;
    }

    double TextEdit::gutter_width() const {
        if (!_numbered) {
            return 0.0;
        }

        const size_t digits = std::max<size_t>(2, std::to_string(line_count()).size());

        return (static_cast<double>(digits) * _charWidth) + (2.0 * PAD_X);
    }

    double TextEdit::text_left() const {
        return _box.x + gutter_width() + PAD_X;
    }

    double TextEdit::text_width() const {
        return std::max(0.0, _box.x + _box.w - Theme::lane - text_left());
    }

    double TextEdit::view_top() const {
        return _box.y + PAD_Y;
    }

    double TextEdit::view_height() const {
        return std::max(0.0, _box.h - (2.0 * PAD_Y));
    }

    double TextEdit::farthest_x() const {
        if (_wrapped) {
            return 0.0;
        }

        return std::max(0.0, (static_cast<double>(_widest + 2) * _charWidth) - text_width());
    }

    double TextEdit::farthest_y() const {
        return std::max(0.0, (static_cast<double>(row_count()) * _lineHeight) - view_height());
    }

    bool TextEdit::over_lane(const double x) const {
        return farthest_y() > 0.0 && x >= _box.x + _box.w - Theme::lane;
    }

    bool TextEdit::over_rail(const double x, const double y) const {
        return farthest_x() > 0.0 && y >= _box.y + _box.h - Theme::lane && x >= text_left()
            && x < text_left() + text_width();
    }

    size_t TextEdit::hit(const double x, const double y) const {
        if (_lineHeight <= 0.0) {
            return 0;
        }

        const double which = std::floor((y - view_top() + _scrollY) / _lineHeight);
        const auto row = static_cast<size_t>(std::clamp(which, 0.0, static_cast<double>(row_count() - 1)));
        const double column = std::round(std::max(0.0, x - text_left() + _scrollX) / _charWidth);

        return offset_at(row, static_cast<size_t>(column));
    }

    void TextEdit::span(size_t &from, size_t &to) const {
        from = std::min(_caret, _anchor);
        to = std::max(_caret, _anchor);
    }

    void TextEdit::move_to(const size_t offset, const bool selecting, const bool keepGoal) {
        _caret = std::min(offset, _text.size());

        if (!selecting) {
            _anchor = _caret;
        }

        if (!keepGoal) {
            _goal = column_of(_caret) - base_of(row_of(_caret));
        }

        _typing = false;
        _showCaret = true;
        _blinked = now();

        keep_caret();
        invalidate();
    }

    void TextEdit::keep_caret() {
        if (_charWidth <= 0.0 && root() != nullptr) {
            measure(root()->type());
        }

        const size_t row = row_of(_caret);
        const double top = static_cast<double>(row) * _lineHeight;

        if (top < _scrollY) {
            _scrollY = top;
        } else if (top + _lineHeight > _scrollY + view_height()) {
            _scrollY = top + _lineHeight - view_height();
        }

        if (!_wrapped) {
            const double x = static_cast<double>(column_of(_caret)) * _charWidth;
            const double margin = 4.0 * _charWidth;

            if (x < _scrollX) {
                _scrollX = x - margin;
            } else if (x > _scrollX + text_width() - _charWidth) {
                _scrollX = x - text_width() + margin;
            }
        }

        clamp_scroll();
    }

    void TextEdit::clamp_scroll() {
        _scrollX = std::clamp(_scrollX, 0.0, farthest_x());
        _scrollY = std::clamp(_scrollY, 0.0, farthest_y());
    }

    void TextEdit::remember(const size_t from, const size_t to, const std::string &with, const bool typing) {
        // A run of typing at the caret grows the one change, so it goes back as one.
        if (typing && _typing && !_undo.empty() && from == to
            && _undo.back().at + _undo.back().came.size() == from) {
            _undo.back().came += with;

            return;
        }

        _typing = typing;

        _undo.push_back(Edit{.at = from,
                             .gone = _text.substr(from, to - from),
                             .came = with,
                             .caret = _caret,
                             .anchor = _anchor,});
        _redo.clear();

        if (_undo.size() > UNDO_LIMIT) {
            _undo.erase(_undo.begin());
        }
    }

    void TextEdit::restore(std::vector<Edit> &from, std::vector<Edit> &to) {
        if (_readOnly || from.empty()) {
            return;
        }

        Edit back = std::move(from.back());
        from.pop_back();

        // What came goes and what went comes back, and the same again the other way.
        _text.replace(back.at, back.came.size(), back.gone);
        reindex(back.at, back.came.size(), back.gone.size());

        const size_t caret = back.caret;
        const size_t anchor = back.anchor;

        back.caret = _caret;
        back.anchor = _anchor;
        std::swap(back.gone, back.came);
        to.push_back(std::move(back));

        _anchor = anchor;
        move_to(caret, true);

        if (_edited) {
            _edited(_text);
        }
    }

    void TextEdit::replace(const size_t from, const size_t to, const std::string &with, const bool typing) {
        if (_readOnly || (from == to && with.empty())) {
            return;
        }

        remember(from, to, with, typing);

        _text.replace(from, to - from, with);
        reindex(from, to - from, with.size());

        const bool keepTyping = _typing;

        move_to(from + with.size(), false);
        _typing = keepTyping;

        if (_edited) {
            _edited(_text);
        }
    }

    void TextEdit::insert(const std::string &what, const bool typing) {
        size_t from = 0;
        size_t to = 0;

        span(from, to);
        replace(from, to, without_returns(what), typing && from == to);
    }

    double TextEdit::natural_width(Typeface & /*type*/) {
        return fixedWidth >= 0.0 ? fixedWidth : 240.0;
    }

    double TextEdit::natural_height(Typeface & /*type*/, double /*width*/) {
        return fixedHeight >= 0.0 ? fixedHeight : 180.0;
    }

    void TextEdit::arrange(Typeface &type) {
        if (_cog != nullptr) {
            const double side = _cog->natural_width(type);

            _cog->place(BLRect{_box.x + _box.w - Theme::lane - side - 2.0, _box.y + 2.0, side, side}, type);
        }

        refit();
    }

    void TextEdit::show_settings() {
        Root *top = root();

        if (top == nullptr) {
            return;
        }

        if (_menu != nullptr) {
            top->dismiss();

            return;
        }

        const std::vector<Menu::Row> rows = {
            Menu::item(Setting::Numbers, "Line numbers", _numbered ? Glyphs::Glyph::Check : Glyphs::Glyph::Empty),
            Menu::item(Setting::Wrap, "Word wrap", _wrapped ? Glyphs::Glyph::Check : Glyphs::Glyph::Empty),
        };

        const double tall = Menu::height_of(rows);
        const BLRect at = _cog->box();
        const bool below = at.y + at.h + 4.0 + tall <= top->height();

        _menu = top->layer(Root::POPUPS)->append(std::make_unique<Menu>(rows, [this](const int action) {
            root()->dismiss();

            switch (static_cast<Setting>(action)) {
                case Setting::Numbers:
                    numbered(!_numbered);
                    break;
                case Setting::Wrap:
                    wrapped(!_wrapped);
                    break;
            }
        }));

        _menu->place(BLRect{std::max(0.0, at.x + at.w - Menu::WIDTH),
                            below ? at.y + at.h + 4.0 : at.y - tall - 4.0, Menu::WIDTH, tall},
                     top->type());
        _menu->invalidate();

        top->set_dismiss([this] {
            if (_menu != nullptr && root() != nullptr) {
                root()->layer(Root::POPUPS)->erase(_menu);
            }

            _menu = nullptr;
        }, _cog);
    }

    void TextEdit::paint(const Painter &painter) {
        const Theme::Palette &palette = Theme::palette();
        const BLFont &font = painter.font(Typeface::mono, Theme::fontSmall);
        const double gutter = gutter_width();

        painter.round(_box, Theme::radiusSmall, palette.field);
        painter.push(BLRect{_box.x + 1.0, _box.y + 1.0, _box.w - 2.0, _box.h - 2.0});

        if (_numbered) {
            painter.fill(BLRect{_box.x, _box.y, gutter, _box.h}, palette.sunken);
            painter.fill(BLRect{_box.x + gutter, _box.y, 1.0, _box.h}, palette.border);
        }

        size_t from = 0;
        size_t to = 0;

        span(from, to);

        const size_t caretRow = row_of(_caret);
        const size_t caretLine = line_of(_caret);
        const auto first = static_cast<size_t>(std::floor(_scrollY / _lineHeight));
        const auto last = std::min(row_count() - 1,
                                   static_cast<size_t>(std::floor((_scrollY + view_height()) / _lineHeight)));
        const double left = text_left() - _scrollX;
        const double inset = (_lineHeight - _fontHeight) / 2.0;

        for (size_t row = first; _numbered && row <= last; ++row) {
            const size_t start = _rowStarts[row];
            const size_t line = line_of(start);

            // Only the first row of a line carries its number.
            if (start != _starts[line]) {
                continue;
            }

            const double y = view_top() + (static_cast<double>(row) * _lineHeight) - _scrollY;
            const bool current = line == caretLine && focused() && !_readOnly;

            painter.label(font, BLRect{_box.x, y, gutter - PAD_X, _lineHeight}, Align::End,
                          std::to_string(line + 1), current ? palette.muted : palette.faint);
        }

        painter.push(BLRect{_box.x + gutter + 1.0, _box.y + 1.0, _box.w - gutter - 2.0, _box.h - 2.0});

        for (size_t row = first; row <= last; ++row) {
            const double y = view_top() + (static_cast<double>(row) * _lineHeight) - _scrollY;
            const size_t start = _rowStarts[row];
            const size_t end = row_end(row);
            const size_t base = base_of(row);

            if (row == caretRow && focused() && from == to) {
                painter.fill(BLRect{_box.x + gutter + 1.0, y, _box.w - gutter - 2.0, _lineHeight},
                             Theme::alpha(palette.accent, 0.06));
            }

            if (from != to && from <= end && to >= start) {
                const size_t head = std::max(from, start);
                const size_t tail = std::min(to, end);
                const double x1 = left + (static_cast<double>(column_of(head) - base) * _charWidth);
                double x2 = left + (static_cast<double>(column_of(tail) - base) * _charWidth);

                if (to > end) {
                    x2 += _charWidth * 0.6;
                }

                painter.fill(BLRect{x1, y, x2 - x1, _lineHeight},
                             Theme::alpha(palette.accent, focused() ? 0.32 : 0.18));
            }

            if (start != end) {
                // Given the whole row's width, so nothing is cut: what is off the edge
                // is scrolled to, not elided.
                const std::string &shown = expand(row);
                const double wide = static_cast<double>(column_of(end) - base + 1) * _charWidth;

                painter.row(font, BLRect{left, y, wide, _lineHeight}, shown, palette.text);
            }
        }

        if (_text.empty() && !_placeholder.empty()) {
            painter.text(font, BLPoint{left, view_top() + inset}, _placeholder, palette.faint);
        }

        if (focused() && _showCaret && !_readOnly) {
            const double x = left + (static_cast<double>(column_of(_caret) - base_of(caretRow)) * _charWidth);
            const double y = view_top() + (static_cast<double>(caretRow) * _lineHeight) - _scrollY;

            painter.fill(BLRect{x, y + 2.0, 1.5, _lineHeight - 4.0}, palette.accent);
        }

        painter.pop();

        if (const double reach = farthest_y(); reach > 0.0) {
            const double whole = static_cast<double>(row_count()) * _lineHeight;
            const double tall = std::max(24.0, view_height() * view_height() / whole);
            const double y = view_top() + ((view_height() - tall) * (_scrollY / reach));

            painter.round(BLRect{_box.x + _box.w - Theme::lane + 3.0, y, 4.0, tall}, 2.0,
                          Theme::alpha(palette.muted, _thumbDrag ? 0.7 : 0.4));
        }

        if (const double reach = farthest_x(); reach > 0.0) {
            const double room = text_width();
            const double whole = room + reach;
            const double wide = std::max(24.0, room * room / whole);
            const double x = text_left() + ((room - wide) * (_scrollX / reach));

            painter.round(BLRect{x, _box.y + _box.h - 7.0, wide, 4.0}, 2.0,
                          Theme::alpha(palette.muted, _railDrag ? 0.7 : 0.4));
        }

        painter.pop();

        Widget::paint(painter);

        painter.outline(_box, Theme::radiusSmall, 1.0, focused() ? palette.accent : palette.border);
    }

    Cursor TextEdit::cursor_at(const double x, const double y) const {
        if (over_lane(x) || over_rail(x, y) || (_numbered && x < _box.x + gutter_width())) {
            return Cursor::Default;
        }

        return Cursor::Text;
    }

    bool TextEdit::press(const Pointer &at) {
        if (!enabled() || root() == nullptr) {
            return false;
        }

        if (over_lane(at.x)) {
            _thumbDrag = true;
            _grabAt = at.y;
            _grabScroll = _scrollY;

            invalidate();

            return true;
        }

        if (over_rail(at.x, at.y)) {
            _railDrag = true;
            _grabAt = at.x;
            _grabScroll = _scrollX;

            invalidate();

            return true;
        }

        root()->focus(this);

        const double time = now();
        const bool again = time - _lastClick < MULTI_CLICK && std::abs(at.x - _lastX) < 4.0
            && std::abs(at.y - _lastY) < 4.0;

        _clicks = again ? (_clicks % 3) + 1 : 1;
        _lastClick = time;
        _lastX = at.x;
        _lastY = at.y;

        const size_t offset = hit(at.x, at.y);

        if (_clicks == 2) {
            size_t start = offset;
            size_t end = offset;

            while (start > 0 && is_word_char(_text[start - 1])) {
                start = before(start);
            }

            while (end < _text.size() && is_word_char(_text[end])) {
                end = after(end);
            }

            _anchor = start;
            move_to(end, true);
        } else if (_clicks == 3) {
            const size_t line = line_of(offset);

            _anchor = _starts[line];
            move_to(line + 1 < line_count() ? _starts[line + 1] : _text.size(), true);
        } else {
            move_to(offset, at.shift);
        }

        return true;
    }

    void TextEdit::drag(const Pointer &at) {
        if (_thumbDrag) {
            const double whole = static_cast<double>(row_count()) * _lineHeight;

            _scrollY = _grabScroll + ((at.y - _grabAt) * whole / std::max(1.0, view_height()));
            clamp_scroll();
            invalidate();

            return;
        }

        if (_railDrag) {
            const double room = text_width();

            _scrollX = _grabScroll + ((at.x - _grabAt) * (room + farthest_x()) / std::max(1.0, room));
            clamp_scroll();
            invalidate();

            return;
        }

        if (_clicks == 1) {
            move_to(hit(at.x, at.y), true);
        }
    }

    void TextEdit::release(const Pointer & /*at*/) {
        if (_thumbDrag || _railDrag) {
            _thumbDrag = false;
            _railDrag = false;

            invalidate();
        }
    }

    bool TextEdit::wheel(const double steps, const Pointer &at) {
        // Shift turns the wheel sideways, where there is anything that way.
        if (at.shift) {
            if (farthest_x() <= 0.0) {
                return false;
            }

            _scrollX -= steps * 3.0 * _charWidth;
        } else {
            if (farthest_y() <= 0.0) {
                return false;
            }

            _scrollY -= steps * 3.0 * _lineHeight;
        }

        clamp_scroll();
        invalidate();

        return true;
    }

    bool TextEdit::key(const Key &pressed) {
        size_t from = 0;
        size_t to = 0;

        span(from, to);

        const int letter = pressed.code >= 'A' && pressed.code <= 'Z' ? pressed.code - 'A' + 'a' : pressed.code;

        if (pressed.ctrl && !pressed.alt) {
            switch (letter) {
                case 'a':
                    select_all();
                    return true;

                case 'c':
                case 'x':
                    if (from != to) {
                        Clipboard::write(_text.substr(from, to - from));

                        if (letter == 'x') {
                            replace(from, to, "");
                        }
                    }
                    return true;

                case 'v':
                    insert(Clipboard::read());
                    return true;

                case 'z':
                    if (pressed.shift) {
                        restore(_redo, _undo);
                    } else {
                        restore(_undo, _redo);
                    }
                    return true;

                case 'y':
                    restore(_redo, _undo);
                    return true;

                default:
                    break;
            }
        }

        const size_t row = row_of(_caret);
        const size_t line = line_of(_caret);
        const auto page = static_cast<size_t>(std::max(1.0, std::floor(view_height() / _lineHeight) - 1.0));

        switch (pressed.code) {
            case Code::Left:
                move_to(from != to && !pressed.shift ? from : pressed.ctrl ? word_left(_caret) : before(_caret),
                        pressed.shift);
                return true;

            case Code::Right:
                move_to(from != to && !pressed.shift ? to : pressed.ctrl ? word_right(_caret) : after(_caret),
                        pressed.shift);
                return true;

            case Code::Up:
                move_to(row == 0 ? 0 : offset_at(row - 1, _goal), pressed.shift, row != 0);
                return true;

            case Code::Down:
                move_to(row + 1 >= row_count() ? _text.size() : offset_at(row + 1, _goal), pressed.shift,
                        row + 1 < row_count());
                return true;

            case Code::PageUp:
                move_to(offset_at(row > page ? row - page : 0, _goal), pressed.shift, true);
                return true;

            case Code::PageDown:
                move_to(offset_at(std::min(row_count() - 1, row + page), _goal), pressed.shift, true);
                return true;

            case Code::Home: {
                if (pressed.ctrl) {
                    move_to(0, pressed.shift);
                    return true;
                }

                // First to the text on the row, then to its very start.
                size_t text = _rowStarts[row];

                while (text < row_end(row) && (_text[text] == ' ' || _text[text] == '\t')) {
                    ++text;
                }

                move_to(_caret == text ? _rowStarts[row] : text, pressed.shift);
                return true;
            }

            case Code::End:
                move_to(pressed.ctrl ? _text.size() : row_end(row), pressed.shift);
                return true;

            case Code::Backspace:
                if (from != to) {
                    replace(from, to, "");
                } else if (_caret > 0) {
                    replace(pressed.ctrl ? word_left(_caret) : before(_caret), _caret, "");
                }
                return true;

            case Code::Delete:
                if (from != to) {
                    replace(from, to, "");
                } else if (_caret < _text.size()) {
                    replace(_caret, pressed.ctrl ? word_right(_caret) : after(_caret), "");
                }
                return true;

            case Code::Return: {
                if (_readOnly) {
                    return false;
                }

                // The new line keeps the indent of the one it came from.
                size_t indent = _starts[line];

                while (indent < from && (_text[indent] == ' ' || _text[indent] == '\t')) {
                    ++indent;
                }

                insert("\n" + _text.substr(_starts[line], indent - _starts[line]));
                return true;
            }

            case Code::Tab:
                if (_readOnly || pressed.ctrl) {
                    return false;
                }

                if (!pressed.shift) {
                    insert(std::string(TAB - (column_of(from) % TAB), ' '));
                }
                return true;

            default:
                return false;
        }
    }

    void TextEdit::wrote(const std::string &text) {
        if (!_readOnly) {
            insert(text, true);
        }
    }

    void TextEdit::gained_focus() {
        Widget::gained_focus();

        _showCaret = true;
        _blinked = now();

        invalidate();
        wake();
    }

    void TextEdit::lost_focus() {
        Widget::lost_focus();

        invalidate();
    }

    bool TextEdit::advance(const double now) {
        if (!focused() || _readOnly) {
            return false;
        }

        if (now - _blinked >= BLINK) {
            _blinked = now;
            _showCaret = !_showCaret;

            invalidate();
        }

        return sleep_until(_blinked + BLINK);
    }
}
