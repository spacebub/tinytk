// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>

#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/layout/Panel.h"
#include "ttk/toolkit/overlays/Dialog.h"

namespace ttk {
    Label *Dialog::heading(Box *into, const std::string &text) {
        Label *made = into->append(std::make_unique<Label>(text));

        made->font(Theme::of().headingWeight, Theme::fontLarge)->tone(Theme::of().text)->wrap();

        return made;
    }

    Label *Dialog::body(Box *into, const std::string &text) {
        Label *made = into->append(std::make_unique<Label>(text));

        made->font(400, Theme::fontBody)->tone(Theme::of().muted)->wrap();

        return made;
    }

    Dialog::Dialog() {
        _takesPointer = true;

        _card = append(std::make_unique<Panel>());
        _card->rounding = Theme::radius;

        set_open(true);
    }

    void Dialog::set_open(const bool open) {
        if (_open == open) {
            return;
        }

        _open = open;

        set_visible(open);

        if (open) {
            _grown.set(0.95F);
            _grow = true;
        }

        invalidate();
    }

    void Dialog::arrange(Typeface &type) {
        if (_grow) {
            _grow = false;

            _grown.run(1.0F, now(), 0.14, Anim::Curve::CubicOut);

            wake();
        }

        const double wide = std::min(wanted, _box.w - 48.0);
        const double want = tall > 0.0 ? tall : _card->natural_height(type, wide);
        const double high = std::min(want, _box.h - 48.0);

        _card->place(BLRect{_box.x + ((_box.w - wide) / 2.0), _box.y + ((_box.h - high) / 2.0), wide,
                            high},
                     type);
    }

    void Dialog::paint(const Painter &painter) {
        painter.fill(_box, Theme::of().scrim);

        // The card grows into place. Blend2D can scale, so the transform is a real one.
        const double grown = _grown.value() > 0.0 ? _grown.value() : 1.0;
        const bool growing = grown < 0.999;

        if (growing) {
            const BLRect card = _card->box();

            painter.context().save();
            painter.context().scale(grown, grown);
            painter.context().translate((card.x + (card.w / 2.0)) * ((1.0 / grown) - 1.0),
                                        (card.y + (card.h / 2.0)) * ((1.0 / grown) - 1.0));
        }

        Widget::paint(painter);
        paint_over(painter);

        if (growing) {
            painter.context().restore();
        }
    }

    Widget *Dialog::at(const double x, const double y) {
        if (!visible()) {
            return nullptr;
        }

        if (Widget *found = Widget::at(x, y); found != nullptr) {
            return found;
        }

        // Anything the card does not want is still the dialog's, so the page under it
        // never answers the pointer.
        return this;
    }

    bool Dialog::press(const Pointer &at) {
        _onScrim = !_card->holds(at.x, at.y) && (root() == nullptr || !root()->just_dismissed());

        return true;
    }

    void Dialog::release(const Pointer &at) {
        if (_onScrim && !_card->holds(at.x, at.y) && dismissed) {
            dismissed();
        }

        _onScrim = false;
    }

    bool Dialog::advance(const double now) {
        _grown.advance(now);

        invalidate();

        return _grown.live();
    }
}
