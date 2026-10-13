// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "ttk/draw/Damage.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Compose.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/controls/TextBox.h"
#include "ttk/toolkit/controls/TextEdit.h"
#include "ttk/toolkit/layout/Scroll.h"
#include "ttk/util/Cache.h"

namespace {
    void require(const bool good, const char *why) {
        if (!good) {
            std::fprintf(stderr, "%s\n", why);
            std::exit(1);
        }
    }

    std::size_t area(const ttk::Damage &damage) {
        std::size_t total = 0;
        for (const BLRectI &r : damage.regions()) {
            total += static_cast<std::size_t>(r.w) * static_cast<std::size_t>(r.h);
        }
        return total;
    }

    void damage_checks() {
        ttk::Damage damage;
        damage.resize(800, 600);
        damage.add({10, 10, 2, 20});
        damage.add({700, 500, 2, 20});
        for (int at = 0; at < 100; ++at) {
            damage.add({10, 10, 2, 20});
        }
        require(damage.regions().size() == 2 && area(damage) == 80, "distant/repeated damage expanded");
        damage.add({0, 0, 800, 600});
        require(damage.regions().size() == 1 && damage.whole(), "containing damage kept redundant regions");
        damage.clear();
        damage.add({0, 0, 800, 3});
        damage.add({0, 599, 800, 1});
        damage.add({790, 0, 8, 600});
        require(area(damage) < 10000, "scroll strips expanded into the viewport");
        damage.clear();
        damage.add({-0.5, -0.5, 2, 2});
        require(area(damage) == 4, "fractional damage was not rounded outward");
        damage.add({1e100, 0, 1, 1});
        damage.add({1, 1, -0.1, 2});
        require(area(damage) == 4, "empty or offscreen damage survived");

        damage.resize(64, 64);
        std::vector<bool> expected(static_cast<std::size_t>(64) * 64);
        std::mt19937 random(7);
        for (int at = 0; at < 160; ++at) {
            const int x = static_cast<int>(random() % 64);
            const int y = static_cast<int>(random() % 64);
            damage.add({static_cast<double>(x), static_cast<double>(y), 1, 1});
            expected[(static_cast<std::size_t>(y) * 64) + static_cast<std::size_t>(x)] = true;
            require(damage.regions().size() <= ttk::Damage::CROWDED, "damage bookkeeping exceeded its bound");
            for (int py = 0; py < 64; ++py) {
                for (int px = 0; px < 64; ++px) {
                    if (!expected[(static_cast<std::size_t>(py) * 64) + static_cast<std::size_t>(px)]) {
                        continue;
                    }
                    require(std::ranges::any_of(damage.regions(),
                                                [&](const BLRectI &r) {
                                                    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
                                                }),
                            "coalescing lost damaged pixels");
                }
            }
        }
        damage.resize(100, 100);
        damage.add({10, 50, 10, 10});
        damage.shift({0, 20, 100, 60}, -20);
        const auto covered = [&](const int x, const int y) {
            return std::ranges::any_of(damage.regions(), [&](const BLRectI &r) {
                return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
            });
        };
        require(covered(10, 30) && covered(19, 39) && covered(10, 50) && area(damage) <= 300,
                "shift did not move pending damage with the pixels");
        const std::size_t moved = area(damage);
        damage.shift({0, 0, 100, 40}, 30);
        require(area(damage) == moved, "shift damaged pixels moved out of its region");
        damage.resize(0, 0);
        damage.all();
        require(damage.empty(), "zero-size damage was retained");
    }

    void root_checks(ttk::Typeface &type) {
        ttk::Root root(type);
        root.damage({0, 0, 10, 10});
        require(root.dirty(), "damage before the first resize did not wake rendering");
        root.resize(400, 240);
        root.settle();
        require(root.whole(), "first layout did not damage the whole window");
    }

    // Antialiased corners cut by a strip's edge can round one level apart.
    bool near(const std::vector<std::uint32_t> &one, const std::vector<std::uint32_t> &two) {
        return std::ranges::equal(one, two, [](const std::uint32_t a, const std::uint32_t b) {
            for (unsigned shift = 0; shift < 32; shift += 8) {
                if (std::abs(static_cast<int>((a >> shift) & 0xffU) - static_cast<int>((b >> shift) & 0xffU)) > 1) {
                    return false;
                }
            }
            return true;
        });
    }

    struct Canvas {
        ttk::Surface surface;
        ttk::Root root;
        explicit Canvas(ttk::Typeface &type) : root(type) {
            require(surface.attach(400, 240), "cannot create raster surface");
            root.resize(400, 240);
        }
        ~Canvas() { surface.detach(); }
        Canvas(const Canvas &) = delete;
        Canvas &operator=(const Canvas &) = delete;
        Canvas(Canvas &&) = delete;
        Canvas &operator=(Canvas &&) = delete;
        std::size_t frame() {
            const auto pixels = ttk::compose(root, surface);
            surface.present();
            return pixels;
        }
        [[nodiscard]] std::vector<std::uint32_t> pixels() const {
            BLImageData data{};
            require(surface.image().get_data(&data) == BL_SUCCESS, "cannot read raster surface");
            std::vector<std::uint32_t> copy(static_cast<std::size_t>(400) * 240);
            for (int y = 0; y < 240; ++y) {
                std::memcpy(copy.data() + (static_cast<std::ptrdiff_t>(y) * 400),
                            static_cast<const std::uint8_t *>(data.pixel_data)
                                    + (static_cast<std::ptrdiff_t>(y) * data.stride),
                            static_cast<std::size_t>(400) * 4);
            }
            return copy;
        }
        void matches_full(const char *why) {
            const auto partial = pixels();
            root.damage_all();
            frame();
            require(partial == pixels(), why);
        }
    };

    void caret_checks(ttk::Typeface &type) {
        for (const bool editor : {false, true}) {
            Canvas canvas(type);
            ttk::Widget *widget = nullptr;
            if (editor) {
                auto edit = std::make_unique<ttk::TextEdit>();
                edit->set_text("first line\nsecond line with text\nthird line\nfourth line\nfifth line\nsixth line");
                edit->numbered();
                widget = canvas.root.layer(ttk::Root::POPUPS)->add(std::move(edit));
            } else {
                auto edit = std::make_unique<ttk::TextBox>(nullptr);
                edit->set_text("A narrow caret repaint: й é");
                widget = canvas.root.layer(ttk::Root::POPUPS)->add(std::move(edit));
            }
            widget->place({13.25, 17.5, 350, editor ? 190.0 : 28.0}, type);
            canvas.frame();
            canvas.root.focus(widget);
            canvas.frame();
            widget->key(ttk::Key{.code = ttk::Code::Right});
            canvas.frame();
            for (int blink = 1; blink <= 4; ++blink) {
                canvas.root.set_now(static_cast<double>(blink));
                canvas.root.advance(static_cast<double>(blink));
                const auto painted = canvas.frame();
                require(painted > 0 && painted < 150, "caret blink repainted more than its narrow strip");
                canvas.matches_full(editor ? "editor caret partial/full mismatch"
                                           : "textbox caret partial/full mismatch");
            }
            canvas.root.set_active(false);
            canvas.root.advance(5);
            canvas.frame();
            canvas.matches_full("inactive caret partial/full mismatch");
        }
    }

    void emptied_caret_checks(ttk::Typeface &type) {
        Canvas canvas(type);
        auto edit = std::make_unique<ttk::TextBox>(nullptr);
        edit->set_text("A field holding far more text than its width shows");
        ttk::Widget *widget = canvas.root.layer(ttk::Root::POPUPS)->add(std::move(edit));
        widget->place({13.25, 17.5, 120, 28}, type);
        canvas.frame();
        canvas.root.focus(widget);
        canvas.frame();
        widget->key(ttk::Key{.code = ttk::Code::End});
        canvas.frame();
        widget->key(ttk::Key{.code = 'a', .ctrl = true});
        widget->key(ttk::Key{.code = 'x', .ctrl = true});
        canvas.frame();
        for (int blink = 1; blink <= 2; ++blink) {
            canvas.root.set_now(static_cast<double>(blink));
            canvas.root.advance(static_cast<double>(blink));
            canvas.frame();
            canvas.matches_full("emptied textbox caret partial/full mismatch");
        }
    }

    class Rows final : public ttk::Widget {
    public:
        Rows() { fixedHeight = 1200; }
        bool marked = false;
        void paint(const ttk::Painter &painter) override {
            for (int row = 0; row < 60; ++row) {
                const BLRect box{_box.x, _box.y + (row * 20), _box.w, 20};
                if (!painter.needed(box)) {
                    continue;
                }
                painter.fill(box, BLRgba32(marked && row == 10 ? 0xffa03040 : row % 2 != 0 ? 0xff405060 : 0xff203040));
                painter.text(painter.font(400, 12), {box.x + 8, box.y + 2}, "Scrolling row", BLRgba32(0xffffffff));
            }
        }
    };

    void scroll_checks(ttk::Typeface &type) {
        Canvas canvas(type);
        auto scroller = std::make_unique<ttk::Scroll>();
        auto *scroll = scroller.get();
        auto rows = std::make_unique<Rows>();
        auto *content = rows.get();
        scroll->hold(std::move(rows));
        canvas.root.content()->add(std::move(scroller));
        canvas.frame();
        for (const double offset : {3, 10, 7, 55, 44}) {
            scroll->scroll_to(offset);
            require(canvas.frame() < 400 * 240 / 3, "scroll repainted the viewport instead of strips");
            canvas.matches_full("scroll partial/full mismatch");
        }
        content->marked = true;
        content->invalidate({0, content->box().y + 200, 400, 20});
        scroll->scroll_to(60);
        scroll->scroll_to(65);
        scroll->scroll_to(62);
        canvas.frame();
        canvas.matches_full("batched scroll partial/full mismatch");
        canvas.root.damage_all();
        scroll->scroll_to(75);
        canvas.frame();
        canvas.matches_full("full damage plus scroll mismatch");
    }

    void paragraph_checks(ttk::Typeface &type) {
        Canvas canvas(type);
        auto label = std::make_unique<ttk::Label>("First line\nSecond line\nThird line with accents é й\nFourth "
                                                  "line\nFifth line\nSixth line\nSeventh line");
        label->wrap();
        canvas.root.content()->add(std::move(label));
        canvas.frame();
        const auto full = canvas.pixels();
        for (int y = 0; y < 160; y += 7) {
            canvas.root.damage({0, static_cast<double>(y), 400, 3});
            canvas.frame();
            require(full == canvas.pixels(), "clipped paragraph lost ink outside a line box");
        }
        canvas.root.relayout();
        require(canvas.root.dirty(), "pending layout did not wake rendering");
    }

    void editor_row_checks(ttk::Typeface &type) {
        Canvas canvas(type);
        auto edit = std::make_unique<ttk::TextEdit>();
        edit->set_text("first line\nÉjy accents й\nthird\nfourth\nfifth\nsixth");
        edit->numbered();
        ttk::Widget *widget = canvas.root.layer(ttk::Root::POPUPS)->add(std::move(edit));
        widget->place({13.25, 17.5, 350, 190}, type);
        canvas.frame();
        const auto full = canvas.pixels();
        for (int y = 0; y <= 237; y += 3) {
            canvas.root.damage({0, static_cast<double>(y), 400, 3});
            canvas.frame();
            require(near(full, canvas.pixels()), "clipped editor rows lost ink outside a row");
        }
    }

    void transform_checks(ttk::Typeface &type) {
        Canvas canvas(type);
        auto &context = canvas.surface.context();
        context.clear_all();
        context.save();
        context.clip_to_rect(BLRectI{20, 20, 30, 30});
        context.translate(30, 30);
        context.fill_round_rect(BLRect{-10, -10, 20, 20}, 3, 3, BLRgba32(0xffabcdef));
        context.restore();
        canvas.surface.present();
        const auto expected = canvas.pixels();
        context.clear_all();
        context.save();
        context.clip_to_rect(BLRectI{20, 20, 30, 30});
        context.translate(30, 30);
        const ttk::Painter painter(context, type, {20, 20, 30, 30});
        painter.round({-10, -10, 20, 20}, 3, BLRgba32(0xffabcdef));
        context.restore();
        canvas.surface.present();
        require(expected == canvas.pixels(), "clip rejection discarded transformed shapes");
    }

    void cache_checks() {
        struct Entry {
            std::size_t used = 0;
        };
        ttk::Cache::RunMap<Entry> entries;
        for (int at = 0; at < 8; ++at) {
            ttk::Cache::run_entry(entries, {.at = at, .run = "same"}, 8).first.used = static_cast<std::size_t>(at) + 1;
        }
        auto [hit, fresh] = ttk::Cache::run_entry(entries, {.at = 0, .run = "same"}, 8);
        require(!fresh && entries.size() == 8 && hit.used == 1, "cache hit evicted entries");
        ttk::Cache::run_entry(entries, {.at = 8, .run = "new"}, 8);
        require(entries.size() < 8, "cache miss failed to bound entries");
    }
}

int main() {
    damage_checks();
    cache_checks();
    ttk::Typeface type;
    require(type.load(), "rendering checks require an installed font");
    root_checks(type);
    caret_checks(type);
    scroll_checks(type);
    paragraph_checks(type);
    editor_row_checks(type);
    emptied_caret_checks(type);
    transform_checks(type);
    std::puts("Paint correctness checks passed (no benchmarks).");
}
