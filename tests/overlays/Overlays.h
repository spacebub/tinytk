// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_TESTS_OVERLAYS_H
#define TTK_TESTS_OVERLAYS_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include <blend2d/blend2d.h>
#include <gtest/gtest.h>

#include "support/Canvas.h"
#include "ttk/dialogs/DialogLayer.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/Widget.h"
#include "ttk/toolkit/overlays/Dialog.h"

namespace overlays {
    class Painted : public ::testing::Test {
    protected:
        void SetUp() override {
            if (!tests::fonts_loaded()) {
                GTEST_SKIP() << "no font installed";
            }
        }
    };

    class Taker final : public ttk::Widget {
    public:
        Taker() { _takesPointer = true; }

        int presses = 0;
        int releases = 0;
        BLRgba32 tone{0xff3080c0};

        bool press(const ttk::Pointer &at) override {
            ++presses;

            return holds(at.x, at.y);
        }

        void release(const ttk::Pointer & /*at*/) override { ++releases; }

        void paint(const ttk::Painter &painter) override { painter.fill(_box, tone); }
    };

    class Probe : public ttk::Dialog {
    public:
        explicit Probe(const double height = 120.0) { tall = height; }

        ~Probe() override {
            if (destroyed != nullptr) {
                ++*destroyed;
            }
        }

        Probe(const Probe &) = delete;
        Probe &operator=(const Probe &) = delete;
        Probe(Probe &&) = delete;
        Probe &operator=(Probe &&) = delete;

        using Dialog::body;
        using Dialog::heading;
        using Dialog::set_open;

        int asked = 0;
        int opens = 0;
        int syncs = 0;
        int arranged = 0;
        int overs = 0;
        bool stay = false;
        bool marker = false;
        int *destroyed = nullptr;
        std::function<void()> on_opened;

        ttk::Widget *corner = nullptr;
        BLRect cornerAt{};

        bool closing() override {
            ++asked;

            return !stay;
        }

        void opened() override {
            ++opens;

            if (on_opened) {
                on_opened();
            }
        }

        void sync() override { ++syncs; }

        void arrange(ttk::Typeface &type) override {
            ++arranged;

            ttk::Dialog::arrange(type);

            if (corner != nullptr) {
                const BLRect card = this->card()->box();

                corner->place(BLRect{card.x + cornerAt.x, card.y + cornerAt.y, cornerAt.w, cornerAt.h}, type);
            }
        }

        static constexpr double MARK = 8.0;

    protected:
        void paint_over(const ttk::Painter &painter) override {
            ++overs;

            if (marker) {
                painter.fill(BLRect{card()->box().x, card()->box().y, MARK, MARK}, BLRgba32(0xff00ff00));
            }
        }
    };

    template <typename Kind>
    void gather(ttk::Widget *from, std::vector<Kind *> &out) {
        if (from == nullptr) {
            return;
        }

        if (auto *hit = dynamic_cast<Kind *>(from); hit != nullptr) {
            out.push_back(hit);
        }

        for (const ttk::Widget::Ptr &child : from->children()) {
            gather(child.get(), out);
        }
    }

    template <typename Kind>
    std::vector<Kind *> all(ttk::Widget *from) {
        std::vector<Kind *> out;

        gather(from, out);

        return out;
    }

    inline double mid_x(const BLRect &box) {
        return box.x + (box.w / 2.0);
    }

    inline double mid_y(const BLRect &box) {
        return box.y + (box.h / 2.0);
    }

    inline void press(ttk::Root &root, const double x, const double y) {
        root.motion(x, y);
        root.press(ttk::Pointer{.x = x, .y = y});
    }

    inline void release(ttk::Root &root, const double x, const double y) {
        root.motion(x, y);
        root.release(ttk::Pointer{.x = x, .y = y});
    }

    inline void click(ttk::Root &root, const double x, const double y) {
        press(root, x, y);
        release(root, x, y);
    }

    inline void click(ttk::Root &root, const ttk::Widget *widget) {
        click(root, mid_x(widget->box()), mid_y(widget->box()));
    }

    inline BLRectI changed(const std::vector<std::uint32_t> &one, const std::vector<std::uint32_t> &two,
                           const int width) {
        int left = width;
        int top = -1;
        int right = -1;
        int bottom = -1;

        for (std::size_t at = 0; at < std::min(one.size(), two.size()); ++at) {
            if (one[at] == two[at]) {
                continue;
            }

            const int x = static_cast<int>(at % static_cast<std::size_t>(width));
            const int y = static_cast<int>(at / static_cast<std::size_t>(width));

            left = std::min(left, x);
            right = std::max(right, x);
            top = top < 0 ? y : top;
            bottom = y;
        }

        if (right < 0) {
            return BLRectI{0, 0, 0, 0};
        }

        return BLRectI{left, top, right - left + 1, bottom - top + 1};
    }

    inline ttk::DialogLayer *dialog_layer(tests::Canvas &canvas) {
        canvas.frame();

        auto *layer = canvas.root().layer(ttk::Root::DIALOGS)->append(std::make_unique<ttk::DialogLayer>());

        canvas.root().relayout();
        canvas.frame();

        return layer;
    }
}

#endif //TTK_TESTS_OVERLAYS_H
