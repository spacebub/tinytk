// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_TESTS_PROBE_H
#define TTK_TESTS_PROBE_H

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <blend2d/blend2d.h>

#include "ttk/toolkit/Painter.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/Widget.h"

namespace tests {
    class Probe : public ttk::Widget {
    public:
        explicit Probe(std::string name = "probe", std::vector<std::string> *journal = nullptr,
                       const bool pointer = true)
            : name(std::move(name)), journal(journal) {
            _takesPointer = pointer;
        }

        std::string name;
        std::vector<std::string> *journal;

        bool takesPress = true;
        bool takesKey = false;
        bool takesWheel = false;
        bool focusPainted = false;

        bool clipping = false;
        BLRect clipRegion{};

        bool spread = false;
        BLRect spreadBox{};

        BLRgba32 tone{0};

        int frames = 0;
        double advancedTo = -1.0;
        double sleepTo = -1.0;

        std::vector<ttk::Pointer> pressedAt;
        std::vector<ttk::Pointer> releasedAt;
        std::vector<ttk::Pointer> draggedAt;
        std::vector<ttk::Pointer> hoveredAt;
        std::vector<ttk::Key> keys;
        std::vector<bool> inside;
        std::vector<BLRect> revealed;
        std::string written;
        double wheeled = 0.0;
        int enters = 0;
        int leaves = 0;
        int gained = 0;
        int lost = 0;
        int moves = 0;
        int arranged = 0;
        int painted = 0;

        bool press(const ttk::Pointer &at) override {
            note("press");
            pressedAt.push_back(at);

            return takesPress;
        }

        void drag(const ttk::Pointer &at) override {
            note("drag");
            draggedAt.push_back(at);
        }

        void release(const ttk::Pointer &at) override {
            note("release");
            releasedAt.push_back(at);
        }

        void enter() override {
            note("enter");
            ++enters;
            Widget::enter();
        }

        void leave() override {
            note("leave");
            ++leaves;
            Widget::leave();
        }

        void hover(const ttk::Pointer &at) override {
            note("hover");
            hoveredAt.push_back(at);
        }

        void within(const bool in) override {
            note(in ? "within" : "without");
            inside.push_back(in);
        }

        bool wheel(const double steps, const ttk::Pointer & /*at*/) override {
            note("wheel");

            if (takesWheel) {
                wheeled += steps;
            }

            return takesWheel;
        }

        bool key(const ttk::Key &pressed) override {
            note("key");
            keys.push_back(pressed);

            return takesKey;
        }

        void wrote(const std::string &text) override {
            note("wrote");
            written += text;
        }

        void gained_focus() override {
            note("gained");
            ++gained;
            Widget::gained_focus();
        }

        void lost_focus() override {
            note("lost");
            ++lost;
            Widget::lost_focus();
        }

        bool advance(const double now) override {
            note("advance");
            advancedTo = now;

            if (frames > 0) {
                --frames;

                return true;
            }

            if (sleepTo >= 0.0) {
                const double when = sleepTo;

                sleepTo = -1.0;

                return sleep_until(when);
            }

            return false;
        }

        [[nodiscard]] bool clips(BLRect &region) const override {
            if (clipping) {
                region = clipRegion;
            }

            return clipping;
        }

        [[nodiscard]] BLRect drawn() const override { return spread ? spreadBox : _box; }

        [[nodiscard]] bool paints_focus() const override { return focusPainted; }

        void reveal(const BLRect &wanted) override { revealed.push_back(wanted); }

        void arrange(ttk::Typeface &type) override {
            ++arranged;
            Widget::arrange(type);
        }

        void paint(const ttk::Painter &painter) override {
            ++painted;

            if (tone.a() != 0) {
                painter.fill(_box, tone);
            }

            Widget::paint(painter);
        }

        void set_pointer(const bool value) { _takesPointer = value; }

    protected:
        void moved() override { ++moves; }

    private:
        void note(const char *what) const {
            if (journal != nullptr) {
                journal->push_back(name + ":" + what);
            }
        }
    };

    inline Probe *mount(ttk::Widget *parent, const BLRect &box, ttk::Typeface &type, std::string name = "probe",
                        std::vector<std::string> *journal = nullptr) {
        auto *made = parent->append(std::make_unique<Probe>(std::move(name), journal));

        made->place(box, type);

        return made;
    }

    inline bool same(const BLRect &one, const BLRect &two) {
        return one.x == two.x && one.y == two.y && one.w == two.w && one.h == two.h;
    }

    inline bool inside(const BLRect &inner, const BLRect &outer) {
        return inner.x >= outer.x && inner.y >= outer.y && inner.x + inner.w <= outer.x + outer.w
               && inner.y + inner.h <= outer.y + outer.h;
    }

    inline bool covers(const std::vector<BLRect> &regions, const double x, const double y) {
        return std::ranges::any_of(
                regions, [x, y](const BLRect &r) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; });
    }

    inline double area(const std::vector<BLRect> &regions) {
        double total = 0.0;

        for (const BLRect &r : regions) {
            total += r.w * r.h;
        }

        return total;
    }
}

#endif //TTK_TESTS_PROBE_H
