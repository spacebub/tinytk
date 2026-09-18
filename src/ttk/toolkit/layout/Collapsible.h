// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_LAYOUT_COLLAPSIBLE_H
#define TTK_LAYOUT_COLLAPSIBLE_H


#include <functional>
#include <string>
#include <utility>

#include "ttk/draw/Anim.h"
#include "ttk/draw/Glyphs.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Painter.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/Widget.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/controls/Pill.h"
#include "ttk/toolkit/layout/Box.h"
#include "ttk/toolkit/layout/Panel.h"
#include "ttk/toolkit/layout/Spacer.h"

namespace ttk {
    // A panel that folds away, with a heading row things can be put on.
    class CollapsiblePanel : public Panel {
    public:
        CollapsiblePanel(std::string title, std::function<void(bool)> folded)
            : _title(std::move(title)), _folded(std::move(folded)) {
            _takesPointer = true;

            _head = append(Box::row());
            _head->spacing(10.0)->cross(Box::Place::Centre);
            _head->fixedHeight = Theme::control;

            // The title is drawn, not laid out, so the row opens with a gap its width.
            _gap = _head->append(std::make_unique<Spacer>(0.0));

            _pill = _head->append(std::make_unique<Pill>());
            _pill->set_visible(false);
            _pill->fixedHeight = 22.0;

            _said = _head->append(std::make_unique<Label>());
            _said->font(400, Theme::fontSmall)->tone(Theme::of().faint);
            _said->stretch = 1.0;

            // The whole band, so it centres in the row exactly as the drawn title does
            // rather than on a line box the layout has centred for it.
            _said->fixedHeight = Theme::control;

            _tools = _head->append(Box::row());
            _tools->spacing(10.0)->cross(Box::Place::Centre);

            _body = append(Box::column());
            _body->spacing(16.0);
        }

        [[nodiscard]] Box *tools() const { return _tools; }

        [[nodiscard]] Box *body() const { return _body; }

        [[nodiscard]] Pill *pill() const { return _pill; }

        void set_open(const bool open) {
            if (_open == open) {
                return;
            }

            _open = open;

            // What is in the body may go the moment it is switched off, so the slide
            // closes over the height it had.
            if (!open && root() != nullptr) {
                _held = _body->natural_height(root()->type(), _box.w - 32.0);
            }

            _turn.run(open ? 180.0F : 0.0F, now(), 0.22, Anim::Curve::CubicOut);
            _slide.run(open ? 1.0F : 0.0F, now(), 0.22, Anim::Curve::CubicOut);

            wake();

            if (root() != nullptr) {
                root()->relayout();
            }

            _settling = true;
        }

        void set_said(std::string text, const bool warning) const {
            _said->set_text(std::move(text));
            _said->tone(warning ? Theme::of().warning : Theme::of().faint);
        }

        double natural_height(Typeface &type, const double width) override {
            const double body = !_open && _slide.live() ? _held
                                                        : _body->natural_height(type, width - 32.0);

            return Theme::control + 32.0
                + (_slide.value() > 0.0 ? (body + 16.0) * _slide.value() : 0.0);
        }

        void arrange(Typeface &type) override {
            _gap->fixedWidth = title_width(type) + 10.0;

            _head->place(BLRect{_box.x + 16.0, _box.y + 16.0, _box.w - 32.0, Theme::control}, type);

            const double room = _box.w - 32.0;
            const double tall = _body->natural_height(type, room);

            _body->place(BLRect{_box.x + 16.0, _box.y + 16.0 + Theme::control + 16.0, room, tall},
                         type);

            _body->set_visible(_slide.value() > 0.0);
        }

        bool clips(BLRect &region) const override {
            region = _box;

            return true;
        }

        void paint(const Painter &painter) override {
            Panel::paint(painter);

            const Theme::Palette &palette = Theme::of();
            const BLRect head{_box.x + 16.0, _box.y + 16.0, _box.w - 32.0, Theme::control};
            const double side = Glyphs::span(1.0F);

            Glyphs::draw(painter.context(), Glyphs::Glyph::Down,
                         BLPoint{head.x, head.y + ((head.h - side) / 2.0)}, 1.0F,
                         _overHead ? palette.text : palette.faint,
                         _turn.value());

            painter.label(painter.font(palette.headingWeight, Theme::fontMedium),
                          BLRect{head.x + side + 10.0, head.y, 200.0, head.h}, Align::Start, _title,
                          palette.text);
        }

        bool press(const Pointer &at) override { return on_head(at.x, at.y); }

        void release(const Pointer &at) override {
            if (on_head(at.x, at.y) && _folded) {
                _folded(!_open);
            }
        }

        [[nodiscard]] Cursor cursor_at(const double x, const double y) const override {
            return on_head(x, y) ? Cursor::Pointer : Cursor::Default;
        }

        void hover(const Pointer &at) override {
            const bool over = on_head(at.x, at.y);

            if (over != _overHead) {
                _overHead = over;

                invalidate();
            }
        }

        void leave() override {
            Widget::leave();

            _overHead = false;
        }

        bool advance(const double now) override {
            _turn.advance(now);
            _slide.advance(now);

            // The panel grows, so the page under it moves: that is a relayout, but one
            // asked for per frame only while it is actually sliding.
            if (_settling && root() != nullptr) {
                root()->relayout();
            }

            invalidate();

            if (!_turn.live() && !_slide.live()) {
                _settling = false;

                return false;
            }

            return true;
        }

        // Where the heading's title ends, so the pill sits after it.
        double title_width(Typeface &type) const {
            return Glyphs::span(1.0F) + 10.0
                + type.width(type.at(Theme::of().headingWeight, Theme::fontMedium), _title);
        }

    private:
        [[nodiscard]] bool on_head(const double x, const double y) const {
            return y < _box.y + 16.0 + Theme::control && x < _said->box().x + _said->box().w;
        }

        std::string _title;
        std::function<void(bool)> _folded;

        Box *_head = nullptr;
        Spacer *_gap = nullptr;
        Box *_tools = nullptr;
        Box *_body = nullptr;
        Pill *_pill = nullptr;

        double _held = 0.0;
        Label *_said = nullptr;

        Anim::Tween _turn;
        Anim::Tween _slide;

        bool _open = false;
        bool _overHead = false;
        bool _settling = false;
    };

    // A heading that folds the row under it, without a panel of its own.
    class DisclosureHeading : public Widget {
    public:
        DisclosureHeading(const std::string &title, std::function<void()> turned)
            : _turned(std::move(turned)) {
            _takesPointer = true;

            _head = append(Box::row());
            _head->spacing(8.0)->cross(Box::Place::Centre);
            _head->fixedHeight = HEAD;

            // The chevron is drawn, not laid out, so the row opens with a gap its width.
            _head->append(std::make_unique<Spacer>(0.0))->fixedWidth = Glyphs::span(WEIGHT);

            _name = _head->append(std::make_unique<Label>(title));
            _name->section();

            _said = _head->append(std::make_unique<Label>());
            _said->font(400, Theme::fontTiny)->tone(Theme::of().faint);

            _head->append(std::make_unique<Spacer>());

            _body = append(Box::column());
        }

        [[nodiscard]] Box *body() const { return _body; }

        void set_open(const bool open) {
            if (_open == open) {
                return;
            }

            _open = open;

            // The body may empty the moment it is switched off, so the slide closes
            // over the height it had.
            if (!open && root() != nullptr) {
                _held = _body->natural_height(root()->type(), _box.w);
            }

            _turn.run(open ? 180.0F : 0.0F, now(), 0.22, Anim::Curve::CubicOut);
            _slide.run(open ? 1.0F : 0.0F, now(), 0.22, Anim::Curve::CubicOut);

            wake();

            if (root() != nullptr) {
                root()->relayout();
            }

            _settling = true;
        }

        void set_said(std::string text) const { _said->set_text(std::move(text)); }

        double natural_height(Typeface &type, const double width) override {
            const double body = !_open && _slide.live() ? _held
                                                        : _body->natural_height(type, width);

            return HEAD + (_slide.value() > 0.0 ? (body + GAP) * _slide.value() : 0.0);
        }

        void arrange(Typeface &type) override {
            _head->place(BLRect{_box.x, _box.y, _box.w, HEAD}, type);

            const double tall = _body->natural_height(type, _box.w);

            _body->place(BLRect{_box.x, _box.y + HEAD + GAP, _box.w, tall}, type);
            _body->set_visible(_slide.value() > 0.0);
        }

        bool clips(BLRect &region) const override {
            region = _box;

            return true;
        }

        void paint(const Painter &painter) override {
            Widget::paint(painter);

            const double side = Glyphs::span(WEIGHT);

            Glyphs::draw(painter.context(), Glyphs::Glyph::Down,
                         BLPoint{_box.x, _box.y + ((HEAD - side) / 2.0)}, WEIGHT,
                         _over ? Theme::of().text : Theme::of().faint, _turn.value());
        }

        bool press(const Pointer &at) override { return on_head(at.y); }

        void release(const Pointer &at) override {
            if (on_head(at.y) && _turned) {
                _turned();
            }
        }

        [[nodiscard]] Cursor cursor_at(double /*x*/, const double y) const override {
            return on_head(y) ? Cursor::Pointer : Cursor::Default;
        }

        void hover(const Pointer &at) override {
            const bool over = on_head(at.y);

            if (over == _over) {
                return;
            }

            _over = over;

            _name->tone(over ? Theme::of().text : Theme::of().faint);

            invalidate();
        }

        void leave() override {
            Widget::leave();

            _over = false;

            _name->tone(Theme::of().faint);
        }

        bool advance(const double now) override {
            _turn.advance(now);
            _slide.advance(now);

            if (_settling && root() != nullptr) {
                root()->relayout();
            }

            invalidate();

            if (!_turn.live() && !_slide.live()) {
                _settling = false;

                return false;
            }

            return true;
        }

    private:
        static constexpr double HEAD = 18.0;
        static constexpr double GAP = 12.0;
        static constexpr float WEIGHT = 0.85F;

        [[nodiscard]] bool on_head(const double y) const { return y < _box.y + HEAD; }

        std::function<void()> _turned;

        Box *_head = nullptr;
        Box *_body = nullptr;
        Label *_name = nullptr;
        Label *_said = nullptr;

        double _held = 0.0;

        Anim::Tween _turn;
        Anim::Tween _slide;

        bool _open = false;
        bool _over = false;
        bool _settling = false;
    };
}


#endif //TTK_LAYOUT_COLLAPSIBLE_H
