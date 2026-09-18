// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>

#include "ttk/system/Text.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/util/Format.h"

namespace ttk {
    void Label::set_text(std::string text) {
        if (_text == text) {
            return;
        }

        _text = std::move(text);

        if (_tracked) {
            _upper = Text::upper(_text);
        }

        // Deliberately no relayout: a label changing under a fixed box is the common
        // case, and laying the window out again would repaint all of it per keystroke.
        // Only a link has to remeasure, so that the hit test follows the new text.
        if (_clicked != nullptr && root() != nullptr) {
            _reach = reach(root()->type());
        }

        // A wrapped one is the exception: its height follows its text, and whatever
        // is under it moves with the last line.
        if (_wrap && root() != nullptr && _box.w > 0.0) {
            const double was = _tall;

            if (natural_height(root()->type(), _box.w) != was) {
                root()->relayout();
            }
        }

        invalidate();
    }

    Label *Label::font(const int weight, const float size) {
        _weight = weight;
        _size = size;

        return this;
    }

    Label *Label::tone(const BLRgba32 tone) {
        _tone = tone;
        _toneDark = Theme::dark();
        _toneSet = true;

        return this;
    }

    Label *Label::align(const Align where) {
        _place = where;

        return this;
    }

    Label *Label::wrap(const bool value) {
        _wrap = value;

        return this;
    }

    Label *Label::section() {
        _weight = 600;
        _size = Theme::fontTiny;
        _tracked = true;
        _upper = Text::upper(_text);
        _tone = Theme::of().faint;
        _toneDark = Theme::dark();
        _toneSet = true;

        return this;
    }

    Label *Label::mono(const bool value) {
        _mono = value;

        return this;
    }

    Label *Label::on_click(std::function<void()> clicked) {
        _clicked = std::move(clicked);
        _takesPointer = true;
        cursor = Cursor::Pointer;

        return this;
    }

    Label *Label::path(const bool value) {
        _path = value;

        return this;
    }

    bool Label::press(const Pointer & /*at*/) {
        return _clicked != nullptr;
    }

    void Label::release(const Pointer &where) {
        if (_clicked && at(where.x, where.y) == this) {
            _clicked();
        }
    }

    void Label::enter() {
        Widget::enter();
    }

    void Label::leave() {
        Widget::leave();
    }

    double Label::reach(Typeface &type) const {
        const BLFont &face = type.at(face_weight(), _size);

        if (_wrap) {
            return _box.w;
        }

        if (_path) {
            const double unit = type.width(face, "M");
            const int room = unit > 0.0 ? std::max(1, static_cast<int>(_box.w / unit)) : 0;

            return type.width(face, Format::fit_path(_text, room));
        }

        return _tracked ? type.width_tracked(face, _upper, 0.9F) : type.width(face, _text);
    }

    void Label::arrange(Typeface &type) {
        _reach = _clicked == nullptr ? 0.0 : reach(type);
    }

    Widget *Label::at(const double x, const double y) {
        if (_clicked == nullptr) {
            return Widget::at(x, y);
        }

        const double taken = std::min(_reach, _box.w);
        double left = _box.x;

        if (_place == Align::Centre) {
            left += (_box.w - taken) / 2.0;
        } else if (_place == Align::End) {
            left += _box.w - taken;
        }

        return visible() && enabled() && x >= left && x < left + taken && y >= _box.y
                && y < _box.y + _box.h
            ? this
            : nullptr;
    }

    double Label::natural_width(Typeface &type) {
        if (fixedWidth >= 0.0) {
            return fixedWidth;
        }

        const BLFont &face = type.at(face_weight(), _size);

        return _tracked ? type.width_tracked(face, _upper, 0.9F) : type.width(face, _text);
    }

    double Label::natural_height(Typeface &type, const double width) {
        if (fixedHeight >= 0.0) {
            return fixedHeight;
        }

        const BLFont &face = type.at(face_weight(), _size);

        if (!_wrap) {
            return type.line_height(face);
        }

        _tall = _text.empty() ? type.line_height(face) : wrap_height(type, face, _text, width);

        return _tall;
    }

    void Label::paint(const Painter &painter) {
        if (_text.empty()) {
            return;
        }

        const BLFont &face = painter.font(face_weight(), _size);
        const BLRgba32 ink = _clicked && hovered() ? Theme::of().accent
                           : _toneSet              ? Theme::restated(_tone, _toneDark)
                                                   : Theme::of().text;

        if (_path) {
            // A fixed width face, so what fits is a division.
            const double unit = painter.width(face, "M");
            const int room = unit > 0.0 ? std::max(1, static_cast<int>(_box.w / unit)) : 0;

            painter.label(face, _box, _place, Format::fit_path(_text, room), ink);

            return;
        }

        if (_wrap) {
            (void) painter.paragraph(face, _box, _text, ink);

            return;
        }

        if (_tracked) {
            const double taken = painter.type().width_tracked(face, _upper, 0.9F);
            const double height = painter.line_height(face);

            double x = _box.x;

            if (_place == Align::Centre) {
                x = _box.x + ((_box.w - taken) / 2.0);
            } else if (_place == Align::End) {
                x = _box.x + _box.w - taken;
            }

            painter.tracked(face, BLPoint{x, _box.y + ((_box.h - height) / 2.0)}, _upper, ink, 0.9);

            return;
        }

        painter.label(face, _box, _place, _text, ink);
    }
}
