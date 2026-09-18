// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <memory>
#include <vector>

#include "support/Canvas.h"
#include "ttk/draw/Damage.h"
#include "ttk/toolkit/Compose.h"
#include "ttk/toolkit/Painter.h"

namespace bench {
    namespace {
        bool &loaded() {
            static bool value = false;

            return value;
        }
    }

    ttk::Typeface &fonts() {
        static ttk::Typeface type = [] {
            ttk::Typeface made;
            loaded() = made.load();

            return made;
        }();

        return type;
    }

    bool fonts_loaded() {
        fonts();

        return loaded();
    }

    Canvas::Canvas(const int width, const int height) {
        _surface.attach(width, height);
        _root = std::make_unique<ttk::Root>(fonts());
        _root->resize(width, height);
        _root->set_now(_now);
    }

    Canvas::~Canvas() {
        _surface.detach();
    }

    void Canvas::resize(const int width, const int height) {
        _surface.attach(width, height);
        _root->resize(width, height);
        _root->damage_all();
    }

    double Canvas::tick(const double hz) {
        _now += 1.0 / hz;

        return _now;
    }

    void Canvas::settle() {
        _root->settle();
    }

    void Canvas::bare() {
        _root->leave();

        for (size_t at = 0; at < ttk::Root::LAYERS; ++at) {
            _root->layer(at)->clear();
        }

        _root->content()->clear();
        _root->relayout();
        _root->settle();

        // settle() damages the window. Nothing is owed to a caller starting over.
        _root->take();
        _root->take_shifts();
        _surface.present();

        _now = 0.0;
        _root->set_now(_now);

        // Runs every tween down, so nothing is left on the live list.
        for (int at = 0; at < 4; ++at) {
            _root->advance(_now);
        }
    }

    std::size_t Canvas::frame() {
        return frame_at(_now);
    }

    std::size_t Canvas::full() {
        _root->damage_all();

        return frame_at(_now);
    }

    std::size_t Canvas::frame_at(const double now) {
        _root->set_now(now);
        _root->advance(now);

        return pending();
    }

    std::size_t Canvas::pending() {
        const std::size_t painted = ttk::compose(*_root, _surface);

        _surface.present();

        return painted;
    }

    std::size_t paint_once(Canvas &canvas, ttk::Widget &widget) {
        const BLRect drawn = widget.drawn();

        ttk::Damage frame;
        frame.resize(canvas.width(), canvas.height());

        const BLRectI clip = frame.clamp_to(drawn);
        const ttk::Painter painter(canvas.context(), canvas.type(), clip);

        canvas.context().save();
        canvas.context().clip_to_rect(clip);
        widget.paint(painter);
        canvas.context().restore();
        canvas.context().flush(BL_CONTEXT_FLUSH_SYNC);

        return static_cast<std::size_t>(clip.w) * static_cast<std::size_t>(clip.h);
    }
}
