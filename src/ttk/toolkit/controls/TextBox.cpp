// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>

#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/controls/TextBox.h"
#include "ttk/util/Clipboard.h"

namespace ttk {
    namespace {

        constexpr double BLINK = 0.53;

        bool is_word_char(const char letter) {
            return (letter >= 'a' && letter <= 'z') || (letter >= 'A' && letter <= 'Z')
                || (letter >= '0' && letter <= '9') || letter == '_'
                || static_cast<unsigned char>(letter) >= 0x80;
        }

    }


    TextBox::TextBox(std::function<void(const std::string &)> edited) : _edited(std::move(edited)) {
        _takesPointer = true;
        cursor = Cursor::Text;
    }

    void TextBox::set_text(std::string text) {
        if (_text == text) {
            return;
        }

        _text = std::move(text);
        _caret = std::min(_caret, _text.size());
        _anchor = _caret;

        invalidate();
    }

    TextBox *TextBox::placeholder(std::string text) {
        _placeholder = std::move(text);

        return this;
    }

    TextBox *TextBox::mono(const bool value) {
        _mono = value;

        return this;
    }

    TextBox *TextBox::read_only(const bool value) {
        _readOnly = value;

        return this;
    }

    TextBox *TextBox::secret(const bool value) {
        _secret = value;
        invalidate();

        return this;
    }

    std::string_view TextBox::shown(const size_t from, const size_t to) const {
        const size_t start = std::min(from, _text.size());
        const size_t end = std::min(to, _text.size());

        if (!_secret) {
            return std::string_view(_text).substr(start, end - start);
        }

        constexpr std::string_view DOT = "\u2022";

        while (_dots.size() < _text.size() * DOT.size()) {
            _dots += DOT;
        }

        size_t count = 0;

        for (size_t at = start; at < end; at = after(at)) {
            ++count;
        }

        return std::string_view(_dots).substr(0, count * DOT.size());
    }

    void TextBox::select_all() {
        _anchor = 0;
        _caret = _text.size();

        invalidate();
    }

    double TextBox::natural_width(Typeface & /*type*/) {
        return fixedWidth >= 0.0 ? fixedWidth : 120.0;
    }

    double TextBox::natural_height(Typeface &type, double /*width*/) {
        return fixedHeight >= 0.0 ? fixedHeight : type.line_height(type.at(400, Theme::fontBody));
    }

    void TextBox::span(size_t &from, size_t &to) const {
        from = std::min(_caret, _anchor);
        to = std::max(_caret, _anchor);
    }

    double TextBox::width_to(Typeface &type, const size_t offset) const {
        return type.width(type.at(Typeface::pick(400, _mono), Theme::fontBody), shown(0, offset));
    }

    size_t TextBox::offset_at(Typeface &type, const double x) const {
        const double wanted = x - _box.x + _shift;
        size_t best = 0;
        double closest = 1e9;

        for (size_t at = 0; at <= _text.size();) {
            if (const double gap = std::abs(width_to(type, at) - wanted); gap < closest) {
                closest = gap;
                best = at;
            }

            if (at == _text.size()) {
                break;
            }

            at = after(at);
        }

        return best;
    }

    size_t TextBox::before(const size_t at) const {
        if (at == 0) {
            return 0;
        }

        size_t back = at - 1;

        while (back > 0 && (static_cast<unsigned char>(_text[back]) & 0xc0) == 0x80) {
            --back;
        }

        return back;
    }

    size_t TextBox::after(const size_t at) const {
        if (at >= _text.size()) {
            return _text.size();
        }

        const auto lead = static_cast<unsigned char>(_text[at]);
        const size_t wide = lead < 0x80 ? 1 : lead < 0xe0 ? 2 : lead < 0xf0 ? 3 : 4;

        return std::min(at + wide, _text.size());
    }

    size_t TextBox::word_left(size_t at) const {
        while (at > 0 && !is_word_char(_text[at - 1])) {
            at = before(at);
        }

        while (at > 0 && is_word_char(_text[at - 1])) {
            at = before(at);
        }

        return at;
    }

    size_t TextBox::word_right(size_t at) const {
        while (at < _text.size() && !is_word_char(_text[at])) {
            at = after(at);
        }

        while (at < _text.size() && is_word_char(_text[at])) {
            at = after(at);
        }

        return at;
    }

    void TextBox::move_to(const size_t offset, const bool selecting) {
        _caret = std::min(offset, _text.size());

        if (!selecting) {
            _anchor = _caret;
        }

        _showCaret = true;
        _blinked = now();

        if (root() != nullptr) {
            keep_caret(root()->type());
        }

        invalidate();
    }

    void TextBox::keep_caret(Typeface &type) {
        const double at = width_to(type, _caret);
        const double room = std::max(0.0, _box.w - 2.0);

        if (at - _shift < 0.0) {
            _shift = at;
        } else if (at - _shift > room) {
            _shift = at - room;
        }

        const double whole = type.width(type.at(Typeface::pick(400, _mono), Theme::fontBody),
                                        shown(0, _text.size()));

        _shift = std::clamp(_shift, 0.0, std::max(0.0, whole - room));
    }

    void TextBox::erase(const size_t from, const size_t to) {
        if (_readOnly || from >= to) {
            return;
        }

        _text.erase(from, to - from);
        _caret = from;
        _anchor = from;

        if (_edited) {
            _edited(_text);
        }

        invalidate();
    }

    void TextBox::insert(const std::string &what) {
        if (_readOnly) {
            return;
        }

        size_t from = 0;
        size_t to = 0;

        span(from, to);

        if (from != to) {
            _text.erase(from, to - from);
            _caret = from;
        }

        _text.insert(_caret, what);
        _caret += what.size();
        _anchor = _caret;

        if (_edited) {
            _edited(_text);
        }

        move_to(_caret, false);
    }

    void TextBox::paint(const Painter &painter) {
        const Theme::Palette &palette = Theme::palette();
        const BLFont &face = painter.font(Typeface::pick(400, _mono), Theme::fontBody);

        painter.push(_box);

        if (_text.empty()) {
            if (!_placeholder.empty()) {
                painter.label(face, _box, Align::Start, _placeholder, palette.faint);
            }

            // The caret belongs to an empty field too, which is where it matters most.
            if (focused() && _showCaret) {
                painter.fill(BLRect{_box.x, _box.y + 3.0, 1.5, _box.h - 6.0}, palette.accent);
            }

            painter.pop();

            return;
        }

        size_t from = 0;
        size_t to = 0;

        span(from, to);

        const double left = _box.x - _shift;

        // Drawn in three runs so the selected part takes the accent's ink. Its edges are
        // measured from the start of the text so kerning across them is kept.
        if (from != to && focused()) {
            const std::string_view head = shown(0, from);
            const std::string_view marked = shown(from, to);
            const double start = painter.width(face, head);
            const double end = painter.width(face, shown(0, to));

            painter.fill(BLRect{left + start, _box.y + 2.0, end - start, _box.h - 4.0},
                         palette.accent);

            painter.label(face, BLRect{left, _box.y, start + 1.0, _box.h}, Align::Start, head,
                          palette.text);

            painter.label(face, BLRect{left + start, _box.y, painter.width(face, marked) + 1.0, _box.h},
                          Align::Start, marked, palette.accentText);

            painter.label(face, BLRect{left + end, _box.y, _box.w + _shift, _box.h}, Align::Start,
                          shown(to, _text.size()), palette.text);
        } else {
            painter.label(face, BLRect{left, _box.y, _box.w + _shift + 4.0, _box.h}, Align::Start,
                          shown(0, _text.size()), palette.text);
        }

        if (focused() && _showCaret && from == to) {
            const double at = left + painter.width(face, shown(0, _caret));

            painter.fill(BLRect{at, _box.y + 3.0, 1.5, _box.h - 6.0}, palette.accent);
        }

        painter.pop();
    }

    bool TextBox::press(const Pointer &at) {
        if (!enabled()) {
            return false;
        }

        if (root() != nullptr) {
            root()->focus(this);

            move_to(offset_at(root()->type(), at.x), at.shift);
        }

        return true;
    }

    void TextBox::drag(const Pointer &at) {
        if (root() != nullptr) {
            move_to(offset_at(root()->type(), at.x), true);
        }
    }

    bool TextBox::key(const Key &pressed) {
        size_t from = 0;
        size_t to = 0;

        span(from, to);

        if (pressed.ctrl && pressed.code == 'a') {
            select_all();

            return true;
        }

        if (pressed.ctrl && (pressed.code == 'c' || pressed.code == 'x')) {
            if (from != to) {
                Clipboard::write(_text.substr(from, to - from));

                if (pressed.code == 'x') {
                    erase(from, to);
                }
            }

            return true;
        }

        if (pressed.ctrl && pressed.code == 'v') {
            insert(Clipboard::read());

            return true;
        }

        switch (pressed.code) {
            case Code::Left:
                move_to(from != to && !pressed.shift ? from
                       : pressed.ctrl ? word_left(_caret)
                                      : before(_caret),
                       pressed.shift);

                return true;

            case Code::Right:
                move_to(from != to && !pressed.shift ? to
                       : pressed.ctrl ? word_right(_caret)
                                      : after(_caret),
                       pressed.shift);

                return true;

            case Code::Home:
                move_to(0, pressed.shift);

                return true;

            case Code::End:
                move_to(_text.size(), pressed.shift);

                return true;

            case Code::Backspace:
                if (from != to) {
                    erase(from, to);
                } else if (_caret > 0) {
                    erase(pressed.ctrl ? word_left(_caret) : before(_caret), _caret);
                }

                move_to(_caret, false);

                return true;

            case Code::Delete:
                if (from != to) {
                    erase(from, to);
                } else if (_caret < _text.size()) {
                    erase(_caret, pressed.ctrl ? word_right(_caret) : after(_caret));
                }

                move_to(_caret, false);

                return true;

            case Code::Return:
                if (accepted) {
                    accepted();
                }

                return true;

            case Code::Escape:
                if (cancelled) {
                    cancelled();

                    return true;
                }

                return false;

            default:
                break;
        }

        return false;
    }

    void TextBox::wrote(const std::string &text) {
        insert(text);
    }

    void TextBox::gained_focus() {
        Widget::gained_focus();

        // The border that says a field is live belongs to the Field around it, and a
        // damage of the run alone would never reach it.
        if (parent() != nullptr) {
            parent()->invalidate();
        }

        _showCaret = true;
        _blinked = now();

        wake();
    }

    void TextBox::lost_focus() {
        Widget::lost_focus();

        if (parent() != nullptr) {
            parent()->invalidate();
        }

        _shift = 0.0;
        _anchor = _caret;
    }

    bool TextBox::advance(const double now) {
        if (!focused()) {
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
