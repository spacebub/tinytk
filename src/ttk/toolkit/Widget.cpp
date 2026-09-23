// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <ranges>

#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/Widget.h"

namespace ttk {
    Widget::Widget() : _revision(Theme::revision()) {}

    Widget *Widget::add(Ptr child) {
        Widget *raw = child.get();

        raw->_parent = this;
        raw->attach(_root);

        _children.push_back(std::move(child));

        return raw;
    }

    void Widget::clear() {
        Root *root = _root;

        if (root != nullptr) {
            for (const Ptr &child : _children) {
                root->forget(child.get());
            }
        }

        _children.clear();
    }

    void Widget::erase(const Widget *child) {
        const auto found = std::ranges::find_if(_children,
                                                [child](const Ptr &held) {
                                                    return held.get() == child;
                                                });

        if (found == _children.end()) {
            return;
        }

        if (_root != nullptr) {
            _root->forget(found->get());
        }

        _children.erase(found);
    }

    void Widget::attach(Root *root) {
        _root = root;

        if (_revision != Theme::revision()) {
            _revision = Theme::revision();

            restyle();
        }

        for (const Ptr &child : _children) {
            child->attach(root);
        }
    }

    void Widget::place(const BLRect &box, Typeface &type) {
        _box = box;

        moved();
        arrange(type);
    }

    double Widget::natural_width(Typeface &type) {
        if (fixedWidth >= 0.0) {
            return fixedWidth;
        }

        double widest = 0.0;

        for (const Ptr &child : _children) {
            if (child->visible()) {
                widest = std::max(widest, child->natural_width(type));
            }
        }

        return widest;
    }

    double Widget::natural_height(Typeface &type, const double width) {
        if (fixedHeight >= 0.0) {
            return fixedHeight;
        }

        double tallest = 0.0;

        for (const Ptr &child : _children) {
            if (child->visible()) {
                tallest = std::max(tallest, child->natural_height(type, width));
            }
        }

        return tallest;
    }

    // A plain widget stacks its children on itself. Only a layout gives them places of
    // their own.
    void Widget::arrange(Typeface &type) {
        for (const Ptr &child : _children) {
            child->place(_box, type);
        }
    }

    void Widget::set_visible(const bool value) {
        if (_visible == value) {
            return;
        }

        invalidate();

        _visible = value;

        if (_root != nullptr) {
            _root->relayout();
        }
    }

    void Widget::set_enabled(const bool value) {
        if (_enabled == value) {
            return;
        }

        _enabled = value;

        invalidate();
    }

    bool Widget::focused() const {
        return _root != nullptr && _root->focused() == this;
    }

    void Widget::paint(const Painter &painter) {
        for (const Ptr &child : _children) {
            if (!child->visible() || !painter.needed(child->drawn())) {
                continue;
            }

            BLRect region{};

            if (child->clips(region)) {
                painter.push(region);
                child->paint(painter);
                painter.pop();
            } else {
                child->paint(painter);
            }
        }
    }

    bool Widget::clips(BLRect & /*unused*/) const {
        return false;
    }

    void Widget::invalidate() const {
        invalidate(drawn());
    }

    void Widget::invalidate(const BLRect &region) const {
        if (_root == nullptr) {
            return;
        }

        BLRect wanted = region;

        // Clipped by every scroller on the way up: a row scrolled out of view has a
        // box, and repainting it would scribble over whatever is there now.
        for (const Widget *above = _parent; above != nullptr; above = above->_parent) {
            BLRect limit{};

            if (!above->clips(limit)) {
                continue;
            }

            const double left = std::max(wanted.x, limit.x);
            const double top = std::max(wanted.y, limit.y);
            const double right = std::min(wanted.x + wanted.w, limit.x + limit.w);
            const double bottom = std::min(wanted.y + wanted.h, limit.y + limit.h);

            if (right <= left || bottom <= top) {
                return;
            }

            wanted = BLRect{left, top, right - left, bottom - top};
        }

        _root->damage(wanted);
    }

    bool Widget::press(const Pointer & /*unused*/) {
        return false;
    }

    void Widget::drag(const Pointer & /*unused*/) {}

    void Widget::release(const Pointer & /*unused*/) {}

    void Widget::enter() {
        _hovered = true;

        invalidate(lit_box());
    }

    void Widget::leave() {
        _hovered = false;
        _pressed = false;

        invalidate(lit_box());
    }

    void Widget::hover(const Pointer & /*unused*/) {}

    void Widget::within(bool /*unused*/) {}

    bool Widget::holds_pointer() const {
        for (const Widget *up = _root == nullptr ? nullptr : _root->hovered(); up != nullptr;
             up = up->_parent) {
            if (up == this) {
                return true;
            }
        }

        return false;
    }

    bool Widget::wheel(double /*unused*/, const Pointer & /*unused*/) {
        return false;
    }

    bool Widget::key(const Key & /*unused*/) {
        return false;
    }

    void Widget::wrote(const std::string & /*unused*/) {}

    void Widget::gained_focus() {
        invalidate();
    }

    void Widget::lost_focus() {
        invalidate();
    }

    Widget *Widget::at(const double x, const double y) {
        if (!_visible || !holds(x, y)) {
            return nullptr;
        }

        BLRect region{};

        if (clips(region)
            && (x < region.x || x >= region.x + region.w || y < region.y
                 || y >= region.y + region.h)) {
            return nullptr;
        }

        for (const auto &child : std::views::reverse(_children)) {
            if (Widget *found = child->at(x, y); found != nullptr) {
                return found;
            }
        }

        // Anything with a hint can be rested on, whether or not it answers a press.
        return (_takesPointer || !hint.empty()) && _enabled ? this : nullptr;
    }

    // A size a parent has set outright wins over the floor: the floor is what the
    // widget asks for when it is left to size itself, and a row splitting itself in
    // half has already decided.
    double Widget::wanted_width(Typeface &type) {
        return fixedWidth >= 0.0 ? fixedWidth : std::max(minWidth, natural_width(type));
    }

    double Widget::wanted_height(Typeface &type, const double width) {
        return fixedHeight >= 0.0 ? fixedHeight : std::max(minHeight, natural_height(type, width));
    }

    Cursor Widget::cursor_at(double /*unused*/, double /*unused*/) const {
        return cursor;
    }

    bool Widget::holds(const double x, const double y) const {
        return x >= _box.x && x < _box.x + _box.w && y >= _box.y && y < _box.y + _box.h;
    }

    bool Widget::advance(double /*unused*/) {
        return false;
    }

    void Widget::wake() const {
        if (_root != nullptr) {
            _root->live(const_cast<Widget *>(this));
        }
    }

    bool Widget::sleep_until(const double when) const {
        if (_root != nullptr) {
            _root->wake_at(const_cast<Widget *>(this), when);
        }

        return false;
    }

    double Widget::now() const {
        return _root != nullptr ? _root->now() : 0.0;
    }
}
