// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_DIALOGS_FILEPICKERDIALOG_H
#define TTK_DIALOGS_FILEPICKERDIALOG_H


#include <utility>

#include "ttk/draw/Glyphs.h"
#include "ttk/draw/Typeface.h"
#include "ttk/dialogs/FilePicker.h"
#include "ttk/toolkit/controls/Button.h"
#include "ttk/toolkit/controls/GlyphButton.h"
#include "ttk/toolkit/controls/TextBox.h"
#include "ttk/toolkit/controls/Toggle.h"
#include "ttk/toolkit/layout/Scroll.h"
#include "ttk/toolkit/overlays/Dialog.h"

namespace ttk {
    class FilePickerDialog : public ttk::Dialog {
    public:
        explicit FilePickerDialog(FilePicker &picker);

        void sync() override;

        bool closing() override;

        void arrange(Typeface &type) override;

        [[nodiscard]] ttk::Cursor cursor_at(double x, double y) const override;

        void hover(const ttk::Pointer &at) override;

        void leave() override;

        bool press(const ttk::Pointer &at) override;

        void release(const ttk::Pointer &at) override;

    protected:
        void paint_over(const ttk::Painter &painter) override;

    private:
        // A framed strip the dialog's own controls sit inside. It is a child rather than
        // something the dialog paints, so that what goes in it is drawn over it, not under.
        class Slab : public Widget {
        public:
            BLRgba32 fill{};
            BLRgba32 edge{};
            double rounding = Theme::radiusSmall;

            void paint(const ttk::Painter &painter) override {
                painter.round(_box, rounding, fill);
                painter.outline(_box, rounding, 1.0, edge);
            }
        };

        static std::string said(const FilePicker::State &pick);

        void chose() const;

        void paint_crumbs(const ttk::Painter &painter);

        static void paint_crumb(const ttk::Painter &painter, const BLRect &box, bool lit);

        // -2 for nothing, -1 for the root, otherwise the part's index.
        [[nodiscard]] int crumb_at(double x, double y) const;

        // The directory listing, drawn straight: a folder here may hold thousands.
        class Rows : public ttk::Scroll {
        public:
            explicit Rows(FilePickerDialog *dialog) : _sheet(dialog) {
                _takesPointer = true;
            }

            static constexpr double ROW = 34.0;

            void arrange(Typeface & /*type*/) override {
                set_reach(static_cast<double>(_sheet->_picker.state().entries.size()) * ROW);
            }

            void paint(const ttk::Painter &painter) override {
                const Theme::Palette &palette = Theme::of();
                const FilePicker::State &pick = _sheet->_picker.state();

                painter.push(_box);

                const double wide = _box.w - (scrollable() ? Theme::lane : 0.0);
                double y = _box.y - offset();

                for (size_t index = 0; index < pick.entries.size(); ++index) {
                    const FilePicker::Row &entry = pick.entries[index];
                    const BLRect line{_box.x, y, wide, ROW};

                    y += ROW;

                    if (!painter.needed(line)) {
                        continue;
                    }

                    if (entry.marked) {
                        painter.round(BLRect{line.x, line.y, line.w - 6.0, line.h},
                                      Theme::radiusSmall - 2.0, palette.accentSoft);
                    } else if (std::cmp_equal(index, _over)) {
                        painter.round(BLRect{line.x, line.y, line.w - 6.0, line.h},
                                      Theme::radiusSmall - 2.0, palette.hover);
                    }

                    constexpr double side = 15.0 * 1.2;

                    Glyphs::draw(painter.context(), entry.directory ? Glyphs::Glyph::FileFolder : Glyphs::Glyph::File,
                                 BLPoint{line.x + 10.0, line.y + ((line.h - side) / 2.0)}, 1.2F,
                                 entry.directory ? palette.accent : palette.faint);

                    painter.label(painter.font(Typeface::pick(entry.marked ? 600 : 400, true),
                                               Theme::fontBody),
                                  BLRect{line.x + 10.0 + side + 9.0, line.y, line.w - 70.0, line.h},
                                  ttk::Align::Start, entry.name,
                                  entry.marked   ? palette.accent
                                  : entry.hidden ? palette.muted
                                                 : palette.text);

                    if (entry.marked) {
                        const double tick = Glyphs::span(1.2F);

                        Glyphs::draw(painter.context(), Glyphs::Glyph::Check,
                                     BLPoint{line.x + line.w - 16.0 - tick,
                                             line.y + ((line.h - tick) / 2.0)},
                                     1.2F, palette.accent);
                    } else if (pick.folders && entry.directory) {
                        // Always shown: the only sign a folder can be taken.
                        const double plus = Glyphs::span(1.2F);
                        const bool lit = std::cmp_equal(index, _over) && _onEdge;

                        Glyphs::draw(painter.context(), Glyphs::Glyph::Plus,
                                     BLPoint{line.x + line.w - 16.0 - plus,
                                             line.y + ((line.h - plus) / 2.0)},
                                     1.2F,
                                     Theme::alpha(lit ? palette.accent : palette.faint,
                                                  lit ? 1.0 : 0.5));
                    }
                }

                painter.pop();

                Scroll::paint(painter);
            }

            [[nodiscard]] ttk::Cursor cursor_at(const double x, const double /*y*/) const override {
                return over_lane(x) ? ttk::Cursor::Default : ttk::Cursor::Pointer;
            }

            void hover(const ttk::Pointer &at) override {
                const int over = over_lane(at.x) ? -1 : row_at(at.y);
                const bool edge = over >= 0 && at.x >= _box.x + _box.w - 44.0;

                if (over != _over || edge != _onEdge) {
                    _over = over;
                    _onEdge = edge;

                    invalidate();
                }
            }

            void leave() override {
                Widget::leave();

                _over = -1;
            }

            bool press(const ttk::Pointer &at) override {
                _scrolling = Scroll::press(at);

                return _scrolling || holds(at.x, at.y);
            }

            void release(const ttk::Pointer &at) override {
                Scroll::release(at);

                if (std::exchange(_scrolling, false)) {
                    return;
                }

                const FilePicker::State &pick = _sheet->_picker.state();
                const int row = row_at(at.y);

                if (row < 0 || std::cmp_greater_equal(row, pick.entries.size())) {
                    return;
                }

                const FilePicker::Row entry = pick.entries[static_cast<size_t>(row)];

                // The row enters, its edge takes: one click cannot mean both.
                if (pick.folders && entry.directory && at.x >= _box.x + _box.w - 44.0) {
                    _sheet->_picker.mark(entry.path);

                    return;
                }

                if (entry.directory) {
                    _sheet->_picker.go(entry.path);
                } else if (pick.saving) {
                    _sheet->_named->set_text(entry.name);
                    _sheet->_picker.named(entry.name);
                } else {
                    _sheet->_picker.mark(entry.path);
                }
            }

        private:
            [[nodiscard]] int row_at(const double y) const {
                const int row = static_cast<int>((y - _box.y + offset()) / ROW);

                return row >= 0 ? row : -1;
            }

            [[nodiscard]] bool over_lane(const double x) const {
                return scrollable() && x >= _box.x + _box.w - Theme::lane;
            }

            FilePickerDialog *_sheet;

            int _over = -1;
            bool _onEdge = false;

            // The press went to the bar or the track, so the release is not a click.
            bool _scrolling = false;
        };

        FilePicker &_picker;

        Slab *_whereSlab = nullptr;
        Slab *_nameSlab = nullptr;
        Slab *_listSlab = nullptr;

        ttk::GlyphButton *_shut = nullptr;
        ttk::GlyphButton *_up = nullptr;
        ttk::GlyphButton *_typer = nullptr;
        ttk::TextBox *_typed = nullptr;
        ttk::TextBox *_named = nullptr;
        Rows *_rows = nullptr;
        ttk::Toggle *_hidden = nullptr;
        ttk::Toggle *_option = nullptr;
        ttk::Button *_use = nullptr;

        BLRect _where{};
        BLRect _crumbs{};
        BLRect _naming{};
        BLRect _panel{};

        std::vector<BLRect> _crumbBoxes;

        // -2 for nothing, -1 for the root, otherwise the part under the pointer.
        int _overCrumb = -2;

        // The directory the list is showing, so only a change to it moves the view.
        std::string _shownPath;

        int _seeded = 0;

        // What the layout is worked out from, so a sync only lays out on a real change.
        struct Shape {
            std::string path;
            std::string option;
            std::string use;
            bool editing = false;
            bool drives = false;
            bool saving = false;
            bool options = false;
            size_t rows = 0;

            bool operator==(const Shape &) const = default;
        };

        Shape _shape;
    };
}


#endif //TTK_DIALOGS_FILEPICKERDIALOG_H
