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
    //! Panel with a heading row that folds its body away.
    //!
    //! The heading shows a turning chevron and the title, then an optional \ref pill(), a faint status text set by
    //! \ref set_said() and a row of \ref tools() at the right. The body slides open and shut below it. The panel
    //! starts closed.
    class CollapsiblePanel : public Panel {
    public:
        //! Creates a closed panel titled `title`.
        //!
        //! `folded` is called with the open state asked for, the opposite of the current one, when a click is
        //! released on the heading left of \ref tools(). The panel does not change state by itself, so the
        //! callback calls \ref set_open() to follow the click.
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
            _said->font(400, Theme::fontSmall)->tone(&Theme::Palette::faint);
            _said->stretch = 1.0;

            // The whole band, so it centres in the row exactly as the drawn title does
            // rather than on a line box the layout has centred for it.
            _said->fixedHeight = Theme::control;

            _tools = _head->append(Box::row());
            _tools->spacing(10.0)->cross(Box::Place::Centre);

            _body = append(Box::column());
            _body->spacing(16.0);
        }

        //! Returns the row at the right end of the heading, for buttons and other controls. Clicks on it do not
        //! fold the panel.
        [[nodiscard]] Box *tools() const { return _tools; }

        //! Returns the column shown below the heading while the panel is open, inset 16 pixels on each side.
        [[nodiscard]] Box *body() const { return _body; }

        //! Returns the pill shown after the title. It is hidden until the caller shows it.
        [[nodiscard]] Pill *pill() const { return _pill; }

        //! Opens or closes the panel, turning the chevron and sliding the body over 0.22 seconds. Does nothing
        //! when the panel is already in that state.
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

        //! Sets the status text shown in the heading after the pill to `text`, in the warning colour when
        //! `warning` is true and faint otherwise.
        void set_said(std::string text, const bool warning) const {
            _said->set_text(std::move(text));
            _said->tone(warning ? &Theme::Palette::warning : &Theme::Palette::faint);
        }

        //! Returns the heading and padding height plus the body height scaled by how far the body has slid
        //! open. While closing, the body height is the one measured when the close began.
        double natural_height(Typeface &type, const double width) override {
            const double body = !_open && _slide.live() ? _held
                                                        : _body->natural_height(type, width - 32.0);

            return Theme::control + 32.0
                + (_slide.value() > 0.0 ? (body + 16.0) * _slide.value() : 0.0);
        }

        //! Places the heading row and the body at its full height. The body is hidden while fully closed.
        void arrange(Typeface &type) override {
            _gap->fixedWidth = title_width(type) + 10.0;

            _head->place(BLRect{_box.x + 16.0, _box.y + 16.0, _box.w - 32.0, Theme::control}, type);

            const double room = _box.w - 32.0;
            const double tall = _body->natural_height(type, room);

            _body->place(BLRect{_box.x + 16.0, _box.y + 16.0 + Theme::control + 16.0, room, tall},
                         type);

            _body->set_visible(_slide.value() > 0.0);
        }

        //! Stores the panel box in `region` and returns true, so the body is cut off while it slides.
        bool clips(BLRect &region) const override {
            region = _box;

            return true;
        }

        //! Paints the panel and its children, then the chevron and the title. The chevron is brighter while the
        //! pointer is over the heading.
        void paint(const Painter &painter) override {
            Panel::paint(painter);

            const Theme::Palette &palette = Theme::palette();
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

        //! Takes a press on the heading, left of \ref tools(), and returns whether it was taken.
        bool press(const Pointer &at) override { return on_head(at.x, at.y); }

        //! Calls the `folded` callback given to the constructor when the press ends over the heading.
        void release(const Pointer &at) override {
            if (on_head(at.x, at.y) && _folded) {
                _folded(!_open);
            }
        }

        //! Returns the pointing hand over the heading, left of \ref tools(), and the default cursor elsewhere.
        [[nodiscard]] Cursor cursor_at(const double x, const double y) const override {
            return on_head(x, y) ? Cursor::Pointer : Cursor::Default;
        }

        //! Repaints the panel when the pointer moves onto or off the heading.
        void hover(const Pointer &at) override {
            const bool over = on_head(at.x, at.y);

            if (over != _overHead) {
                _overHead = over;

                invalidate();
            }
        }

        //! Called when the pointer moves off the panel. Clears the heading highlight.
        void leave() override {
            Widget::leave();

            _overHead = false;
        }

        //! Steps the chevron and the slide, asking for a new layout each frame while they run, and returns
        //! whether either is still running.
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

        //! Returns the width from the left edge of the heading to the end of the title, chevron and gap included.
        double title_width(Typeface &type) const {
            return Glyphs::span(1.0F) + 10.0
                + type.width(type.at(Theme::palette().headingWeight, Theme::fontMedium), _title);
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

    //! Heading with a turning chevron that folds the content below it, without a panel around either.
    //!
    //! The heading row is 18 pixels tall and shows the chevron, the title in section style and a faint status
    //! text set by \ref set_said(). The body slides open 12 pixels below it. The heading starts closed.
    class DisclosureHeading : public Widget {
    public:
        //! Creates a closed heading titled `title`.
        //!
        //! `turned` is called when a click is released anywhere on the heading row. The heading does not change
        //! state by itself, so the caller tracks it and calls \ref set_open().
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
            _said->font(400, Theme::fontTiny)->tone(&Theme::Palette::faint);

            _head->append(std::make_unique<Spacer>());

            _body = append(Box::column());
        }

        //! Returns the column shown below the heading while it is open, as wide as the heading.
        [[nodiscard]] Box *body() const { return _body; }

        //! Opens or closes the heading, turning the chevron and sliding the body over 0.22 seconds. Does nothing
        //! when the heading is already in that state.
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

        //! Sets the faint status text shown after the title to `text`.
        void set_said(std::string text) const { _said->set_text(std::move(text)); }

        //! Returns the heading row height plus the gap and body height scaled by how far the body has slid
        //! open. While closing, the body height is the one measured when the close began.
        double natural_height(Typeface &type, const double width) override {
            const double body = !_open && _slide.live() ? _held
                                                        : _body->natural_height(type, width);

            return HEAD + (_slide.value() > 0.0 ? (body + GAP) * _slide.value() : 0.0);
        }

        //! Places the heading row and the body at its full height. The body is hidden while fully closed.
        void arrange(Typeface &type) override {
            _head->place(BLRect{_box.x, _box.y, _box.w, HEAD}, type);

            const double tall = _body->natural_height(type, _box.w);

            _body->place(BLRect{_box.x, _box.y + HEAD + GAP, _box.w, tall}, type);
            _body->set_visible(_slide.value() > 0.0);
        }

        //! Stores the heading's box in `region` and returns true, so the body is cut off while it slides.
        bool clips(BLRect &region) const override {
            region = _box;

            return true;
        }

        //! Paints the children, then the chevron. The chevron is brighter while the pointer is over the heading
        //! row.
        void paint(const Painter &painter) override {
            Widget::paint(painter);

            const double side = Glyphs::span(WEIGHT);

            Glyphs::draw(painter.context(), Glyphs::Glyph::Down,
                         BLPoint{_box.x, _box.y + ((HEAD - side) / 2.0)}, WEIGHT,
                         _over ? Theme::palette().text : Theme::palette().faint, _turn.value());
        }

        //! Takes a press on the heading row and returns whether it was taken.
        bool press(const Pointer &at) override { return on_head(at.y); }

        //! Calls the `turned` callback given to the constructor when the press ends over the heading row.
        void release(const Pointer &at) override {
            if (on_head(at.y) && _turned) {
                _turned();
            }
        }

        //! Returns the pointing hand over the heading row and the default cursor over the body.
        [[nodiscard]] Cursor cursor_at(double /*x*/, const double y) const override {
            return on_head(y) ? Cursor::Pointer : Cursor::Default;
        }

        //! Brightens the title and repaints when the pointer moves onto the heading row, and dims it again when
        //! the pointer moves off.
        void hover(const Pointer &at) override {
            const bool over = on_head(at.y);

            if (over == _over) {
                return;
            }

            _over = over;

            _name->tone(over ? &Theme::Palette::text : &Theme::Palette::faint);

            invalidate();
        }

        //! Called when the pointer moves off the heading. Clears the highlight and dims the title.
        void leave() override {
            Widget::leave();

            _over = false;

            _name->tone(&Theme::Palette::faint);
        }

        //! Steps the chevron and the slide, asking for a new layout each frame while they run, and returns
        //! whether either is still running.
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
