// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "ttk/draw/Typeface.h"
#include "ttk/system/Text.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/util/Format.h"

namespace ttk {
void Label::set_text(const std::string_view text) {
    if (_text == text) {
        return;
    }

    _text = text;
    _fittedRoom = -1;

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

Label *Label::tone(const Theme::Tone tone) {
    _tone = tone;

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

Label *Label::elide(const int rows) {
    _rows = std::max(0, rows);

    if (_rows > 0) {
        _wrap = true;
    }

    return this;
}

Label *Label::section() {
    _weight = 600;
    _size = Theme::fontTiny;
    _tracked = true;
    _upper = Text::upper(_text);
    _tone = Theme::Tone(&Theme::Palette::faint);

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

const std::string &Label::fitted(Typeface &type, const BLFont &face) {
    // A fixed width face, so what fits is a division.
    const double unit = type.width(face, "M");
    const int room = unit > 0.0 ? std::max(1, static_cast<int>(_box.w / unit)) : 0;

    if (room != _fittedRoom) {
        _fitted = Format::fit_path(_text, room);
        _fittedRoom = room;
    }

    return _fitted;
}

double Label::reach(Typeface &type) {
    const BLFont &face = type.at(face_weight(), _size);

    if (_wrap) {
        return _box.w;
    }

    if (_path) {
        return type.width(face, fitted(type, face));
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

    return visible() && enabled() && x >= left && x < left + taken && y >= _box.y && y < _box.y + _box.h ? this
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

    const size_t lines = fold_spans(type, face, _text, width).size();
    const size_t shown = _rows > 0 ? std::min(lines, static_cast<size_t>(_rows)) : lines;

    _tall = static_cast<double>(shown) * type.line_height(face);

    return _tall;
}

void Label::paint(const Painter &painter) {
    if (_text.empty()) {
        return;
    }

    const BLFont &face = painter.font(face_weight(), _size);
    const BLRgba32 ink = _clicked && hovered() ? Theme::palette().accent : _tone.colour();

    if (_path) {
        painter.label(face, _box, _place, fitted(painter.type(), face), ink);

        return;
    }

    if (_wrap && _rows > 0) {
        paint_capped(painter, face, ink);

        return;
    }

    if (_wrap) {
        (void)painter.paragraph(face, _box, _text, ink);

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

void Label::paint_capped(const Painter &painter, const BLFont &face, const BLRgba32 ink) const {
    const double step = painter.line_height(face);
    const std::vector<Fold> &lines = fold_spans(painter.type(), face, _text, _box.w);
    const auto most = static_cast<size_t>(_rows);
    const size_t shown = std::min(lines.size(), most);

    for (size_t at = 0; at + 1 < shown; ++at) {
        painter.text(face, BLPoint{_box.x, _box.y + (static_cast<double>(at) * step)}, lines[at].text, ink);
    }

    const double y = _box.y + (static_cast<double>(shown - 1) * step);

    if (lines.size() <= most) {
        painter.text(face, BLPoint{_box.x, y}, lines[shown - 1].text, ink);

        return;
    }

    // The last line shown carries the rest of the text, cut where it runs out of room.
    std::string rest = _text.substr(lines[shown - 1].from);

    std::ranges::replace(rest, '\n', ' ');

    painter.row(face, BLRect{_box.x, y, _box.w, step}, rest, ink);
}
}
