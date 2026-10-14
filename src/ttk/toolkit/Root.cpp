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
#include "ttk/toolkit/Root.h"

namespace ttk {
namespace {

BLRect enclose(const BLRect &one, const BLRect &two) {
    const double left = std::min(one.x, two.x);
    const double top = std::min(one.y, two.y);
    const double right = std::max(one.x + one.w, two.x + two.w);
    const double bottom = std::max(one.y + one.h, two.y + two.h);

    return BLRect{left, top, right - left, bottom - top};
}

bool under(const ttk::Widget *who, const ttk::Widget *of) {
    for (; who != nullptr; who = who->parent()) {
        if (who == of) {
            return true;
        }
    }

    return false;
}

}


Widget *Root::layer(const size_t index) {
    return &_layers[std::min(index, LAYERS - 1)];
}

void Root::Page::arrange(Typeface &type) {
    if (!_floats) {
        Widget::arrange(type);

        return;
    }

    // Left where it was placed. Only its insides are worked out again.
    for (const Ptr &child : children()) {
        const BLRect kept = child->box();

        child->place(kept, type);
    }
}

Root::Root(Typeface &type) : _type(type), _painter(type) {
    _layers[POPUPS].floats();

    _page.attach(this);

    for (Page &over : _layers) {
        over.attach(this);
    }
}

void Root::resize(const double width, const double height) {
    if (width == _width && height == _height) {
        return;
    }

    _width = width;
    _height = height;
    _dirty.resize(static_cast<int>(std::ceil(width)), static_cast<int>(std::ceil(height)));
    _relayout = true;

    // A popup hangs off a control that the new size moves.
    dismiss();
}

bool Root::settle() {
    if (_revision != Theme::revision()) {
        _revision = Theme::revision();
        _relayout = true;
    }

    if (!_relayout) {
        return false;
    }

    _relayout = false;

    const BLRect whole{0.0, 0.0, _width, _height};

    _page.attach(this);
    _page.place(whole, _type);

    for (Page &over : _layers) {
        over.attach(this);
        over.place(whole, _type);
    }

    // A relayout moves anything, so nothing short of the window is safe to keep.
    damage_all();

    // What was under the pointer has moved, and nothing else asks again: a widget
    // left hovered keeps its lit look under a pointer that is elsewhere.
    if (_grabbed == nullptr && _hovered != nullptr && !_hovered->holds(_pointer.x, _pointer.y)) {
        hover_to(pick(_pointer.x, _pointer.y), _pointer);
    }

    return true;
}

void Root::damage(const BLRect &region) {
    if (region.w <= 0.0 || region.h <= 0.0) {
        return;
    }

    // Too many to be worth keeping apart: the rest of the frame folds into one.
    if (_crowded) {
        _bound = enclose(_bound, region);

        return;
    }

    // A pointer reports many times between two frames and asks for the same
    // rectangle each time, so the one just given is the case worth taking first.
    if (!_pending.empty()) {
        const BLRect &last = _pending.back();

        if (region.x >= last.x && region.y >= last.y && region.x + region.w <= last.x + last.w
            && region.y + region.h <= last.y + last.h) {
            return;
        }
    }

    _pending.push_back(region);

    if (_pending.size() <= Damage::CROWDED) {
        return;
    }

    _bound = _pending.front();

    for (const BLRect &held : _pending) {
        _bound = enclose(_bound, held);
    }

    _pending.clear();
    _crowded = true;
}

void Root::damage_all() {
    _pending.clear();
    _crowded = false;
    _dirty.all();
}

void Root::flush() {
    if (_crowded) {
        _dirty.add(_bound);
        _crowded = false;
    }

    for (const BLRect &region : _pending) {
        _dirty.add(region);
    }

    _pending.clear();
}

bool Root::whole() {
    flush();

    return _dirty.whole();
}

const std::vector<BLRect> &Root::take() {
    flush();
    _taken.clear();

    for (const BLRectI &region : _dirty.regions()) {
        _taken.emplace_back(region.x, region.y, region.w, region.h);
    }

    _dirty.clear();

    return _taken;
}

bool Root::shift(const Widget *who, const BLRect &region, const int dy) {
    const Widget *top = who;

    for (const Widget *up = who->parent(); up != nullptr; up = up->parent()) {
        BLRect limit{};

        if (up->clips(limit)) {
            return false;
        }

        top = up;
    }

    size_t above = 0;

    for (size_t index = 0; index < LAYERS; ++index) {
        if (top == &_layers[index]) {
            above = index + 1;
        }
    }

    for (size_t index = above; index < LAYERS; ++index) {
        for (const Widget::Ptr &child : _layers[index].children()) {
            const BLRect over = child->drawn();

            if (child->visible() && over.x < region.x + region.w && over.x + over.w > region.x
                && over.y < region.y + region.h && over.y + over.h > region.y) {
                return false;
            }
        }
    }

    _shifts.push_back(Shift{
            .region =
                    BLRectI{static_cast<int>(std::ceil(region.x)),
                            static_cast<int>(std::ceil(region.y)),
                            static_cast<int>(std::floor(region.x + region.w)) - static_cast<int>(std::ceil(region.x)),
                            static_cast<int>(std::floor(region.y + region.h)) - static_cast<int>(std::ceil(region.y))},
            .dy = dy,
    });

    // Only damage already pending moves with these pixels. The caller adds the
    // newly exposed strips after this, in their final positions.
    flush();
    _dirty.shift(_shifts.back().region, dy);

    return true;
}

const std::vector<Root::Shift> &Root::take_shifts() {
    _shifted.swap(_shifts);
    _shifts.clear();

    return _shifted;
}

void Root::paint(BLContext &context, const BLRectI &clip) {
    _painter.aim(context, clip);

    _page.paint(_painter);

    for (Page &over : _layers) {
        over.paint(_painter);
    }

    if (_focusShown && _focused != nullptr && _focused->visible() && !_focused->paints_focus()) {
        _painter.outline(_focused->box(), Theme::radiusSmall, 1.0, Theme::palette().accent);
    }
}

Widget *Root::pick(const double x, const double y) {
    for (size_t index = LAYERS; index > 0; --index) {
        if (Widget *found = _layers[index - 1].at(x, y); found != nullptr) {
            return found;
        }
    }

    return _page.at(x, y);
}

void Root::hover_to(Widget *who, const Pointer &at) {
    if (_hovered == who) {
        if (who != nullptr) {
            who->hover(at);
        }

        return;
    }

    Widget *was = _hovered;

    if (was != nullptr) {
        was->leave();
    }

    _hovered = who;

    if (who != nullptr) {
        who->enter();
        who->hover(at);
    }

    // The containers hear it only where the two paths part.
    for (Widget *up = was == nullptr ? nullptr : was->parent(); up != nullptr; up = up->parent()) {
        if (!under(who, up)) {
            up->within(false);
        }
    }

    for (Widget *up = who == nullptr ? nullptr : who->parent(); up != nullptr; up = up->parent()) {
        if (!under(was, up)) {
            up->within(true);
        }
    }
}

Cursor Root::cursor() const {
    // A widget holding the pointer says what it looks like, wherever it has got to.
    if (_grabbed != nullptr) {
        return _grabbed->cursor_at(_pointer.x, _pointer.y);
    }

    return _hovered == nullptr ? Cursor::Default : _hovered->cursor_at(_pointer.x, _pointer.y);
}

void Root::motion(const double x, const double y) {
    const Pointer at{.x = x, .y = y};

    _pointer = at;

    if (_grabbed != nullptr) {
        _grabbed->drag(at);

        return;
    }

    hover_to(pick(x, y), at);
}

void Root::press(const Pointer &at) {
    _pointer = at;

    if (_focusShown) {
        _focusShown = false;

        if (_focused != nullptr) {
            _focused->invalidate();
        }
    }

    Widget *who = pick(at.x, at.y);

    _justDismissed = false;

    // A press outside an open popup closes it. The press then carries on, so the
    // dropdown next to this one opens rather than only the first closing. The
    // exception is a press on the control the popup belongs to, which would reopen it.
    if (_dismiss && (who == nullptr || _layers[POPUPS].at(at.x, at.y) == nullptr)) {
        const Widget *owner = _owner;

        dismiss();

        _justDismissed = true;

        if (who == nullptr || under(who, owner)) {
            return;
        }
    }

    hover_to(who, at);

    for (Widget *up = who; up != nullptr; up = up->parent()) {
        if (up->enabled() && up->press(at)) {
            up->_pressed = true;

            if (_grabbed == nullptr) {
                _grabbed = up;
            }

            return;
        }
    }

    // A press on nothing in particular takes the keyboard away from a field.
    focus(nullptr);
}

void Root::release(const Pointer &at) {
    Widget *who = _grabbed;

    _grabbed = nullptr;

    if (who == nullptr) {
        return;
    }

    who->_pressed = false;
    who->release(at);

    hover_to(pick(at.x, at.y), at);
}

void Root::grab(Widget *who) {
    _grabbed = who;
}

void Root::wheel(const double steps, const double x, const double y) {
    const Pointer at{.x = x, .y = y};

    for (Widget *up = pick(x, y); up != nullptr; up = up->parent()) {
        if (up->wheel(steps, at)) {
            return;
        }
    }
}

void Root::leave() {
    if (_grabbed != nullptr) {
        return;
    }

    hover_to(nullptr, Pointer{.x = -1.0, .y = -1.0});
}

bool Root::key(const Key &pressed) {
    for (Widget *up = _focused; up != nullptr; up = up->parent()) {
        if (up->key(pressed)) {
            return true;
        }
    }

    if (pressed.code == Code::Tab && !pressed.alt) {
        if (!pressed.repeat) {
            focus_next(pressed.shift);
        }

        return true;
    }

    if ((pressed.code == Code::Return || pressed.code == Code::Space) && _focused != nullptr
        && _focused->takes_focus()) {
        if (!pressed.repeat) {
            const BLRect box = _focused->box();
            const Pointer at{.x = box.x + (box.w / 2.0), .y = box.y + (box.h / 2.0)};

            if (_focused->press(at)) {
                _focused->release(at);
            }
        }

        return true;
    }

    return false;
}

void Root::wrote(const std::string &text) const {
    if (_focused != nullptr) {
        _focused->wrote(text);
    }
}

void Root::focus(Widget *who) {
    if (_focused == who) {
        return;
    }

    Widget *was = _focused;

    _focused = who;
    _focusShown = false;

    if (was != nullptr) {
        was->lost_focus();
    }

    if (_focused != nullptr) {
        _focused->gained_focus();
    }

    if (composing) {
        composing(_focused != nullptr && _focused->takes_focus());
    }
}

void Root::set_active(const bool active) {
    if (_active == active) {
        return;
    }

    _active = active;

    if (_focused != nullptr) {
        _focused->wake();
    }
}

void Root::gather(Widget *from, const int inherited, std::vector<Stop> &out) {
    if (!from->visible() || !from->enabled()) {
        return;
    }

    const int own = from->tab_index();
    int index = own > 0 ? own : inherited;

    if (own < 0 || inherited < 0) {
        index = -1;
    }

    if (from->takes_focus()) {
        out.push_back({.who = from, .index = index});
    }

    for (const Widget::Ptr &child : from->children()) {
        gather(child.get(), index, out);
    }
}

Widget *Root::next_stop(const bool backwards) {
    std::vector<Stop> order;

    // A dialog or a popup owns the keyboard while it is up.
    for (size_t index = LAYERS; index > 0 && order.empty(); --index) {
        gather(&_layers[index - 1], 0, order);
    }

    if (order.empty()) {
        gather(&_page, 0, order);
    }

    if (order.empty()) {
        return nullptr;
    }

    if (std::ranges::any_of(order, [](const Stop &stop) { return stop.index > 0; })) {
        std::ranges::stable_sort(
                order, {}, [](const Stop &stop) { return std::pair{stop.index <= 0, std::max(stop.index, 0)}; });
    }

    const size_t count = order.size();
    const auto found = std::ranges::find(order, _focused, &Stop::who);

    // Skipped widgets stay in the order so focus taken by a click on one moves on
    // from where it sits.
    size_t at = found != order.end() ? static_cast<size_t>(found - order.begin()) : (backwards ? 0 : count - 1);

    for (size_t step = 0; step < count; ++step) {
        at = backwards ? (at + count - 1) % count : (at + 1) % count;

        if (order[at].index >= 0) {
            return order[at].who;
        }
    }

    return nullptr;
}

void Root::focus_next(const bool backwards) {
    Widget *next = next_stop(backwards);

    if (next == nullptr) {
        return;
    }

    // A popup hangs off the control that opened it, so it closes when focus leaves that control.
    if (_dismiss && !under(next, _owner) && !under(next, &_layers[POPUPS])) {
        dismiss();

        // Closing may have rebuilt the tree the first pass pointed into.
        next = next_stop(backwards);

        if (next == nullptr) {
            return;
        }
    }

    focus(next);

    for (Widget *up = next->parent(); up != nullptr; up = up->parent()) {
        up->reveal(next->drawn());
    }

    _focusShown = true;
    next->invalidate();
}

void Root::forget(const Widget *who) {
    if (who == nullptr) {
        return;
    }

    std::erase(_live, who);

    std::erase_if(_sleeping, [who](const Sleeper &kept) { return kept.who == who; });

    if (_hovered == who) {
        _hovered = nullptr;
    }

    if (_grabbed == who) {
        _grabbed = nullptr;
    }

    if (_focused == who) {
        _focused = nullptr;
    }

    if (_owner == who) {
        _owner = nullptr;
    }

    for (const Widget::Ptr &child : who->children()) {
        forget(child.get());
    }
}

void Root::live(Widget *who) {
    enliven(who);

    std::erase_if(_sleeping, [who](const Sleeper &kept) { return kept.who == who; });
}

void Root::wake_at(Widget *who, const double when) {
    for (Sleeper &kept : _sleeping) {
        if (kept.who == who) {
            kept.due = std::min(kept.due, when);

            return;
        }
    }

    _sleeping.push_back(Sleeper{.who = who, .due = when});
}

double Root::waking() const {
    double soonest = -1.0;

    for (const Sleeper &kept : _sleeping) {
        if (soonest < 0.0 || kept.due < soonest) {
            soonest = kept.due;
        }
    }

    return soonest;
}

void Root::advance(const double now) {
    for (size_t at = _sleeping.size(); at > 0; --at) {
        if (const Sleeper &kept = _sleeping[at - 1]; kept.due <= now) {
            enliven(kept.who);
            _sleeping.erase(_sleeping.begin() + static_cast<ptrdiff_t>(at - 1));
        }
    }

    if (_live.empty()) {
        return;
    }

    // An advance may make widgets live or drop them, so the pass runs over a copy.
    _running.assign(_live.begin(), _live.end());

    for (Widget *who : _running) {
        if (!who->advance(now)) {
            std::erase(_live, who);
        }
    }
}

void Root::enliven(Widget *who) {
    if (std::ranges::find(_live, who) == _live.end()) {
        _live.push_back(who);
    }
}

void Root::dismiss() {
    if (!_dismiss) {
        return;
    }

    const std::function<void()> closing = _dismiss;

    _dismiss = nullptr;
    _owner = nullptr;

    closing();
}
}
