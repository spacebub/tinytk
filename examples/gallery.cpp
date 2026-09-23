// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ttk/dialogs/ConfirmDialog.h"
#include "ttk/dialogs/DialogLayer.h"
#include "ttk/dialogs/FilePicker.h"
#include "ttk/dialogs/FilePickerDialog.h"
#include "ttk/dialogs/PromptDialog.h"
#include "ttk/draw/Glyphs.h"
#include "ttk/draw/Theme.h"
#include "ttk/notices/Notifier.h"
#include "ttk/notices/Toasts.h"
#include "ttk/shell/Shell.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/controls/Button.h"
#include "ttk/toolkit/controls/Card.h"
#include "ttk/toolkit/controls/Check.h"
#include "ttk/toolkit/controls/Chip.h"
#include "ttk/toolkit/controls/Fact.h"
#include "ttk/toolkit/controls/Field.h"
#include "ttk/toolkit/controls/GlyphButton.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/controls/MultistateSwitch.h"
#include "ttk/toolkit/controls/Pill.h"
#include "ttk/toolkit/controls/Select.h"
#include "ttk/toolkit/controls/StatusIndicator.h"
#include "ttk/toolkit/controls/Stepper.h"
#include "ttk/toolkit/controls/TabStrip.h"
#include "ttk/toolkit/controls/TextBox.h"
#include "ttk/toolkit/controls/TextView.h"
#include "ttk/toolkit/controls/Toggle.h"
#include "ttk/toolkit/layout/Box.h"
#include "ttk/toolkit/layout/CardGrid.h"
#include "ttk/toolkit/layout/Collapsible.h"
#include "ttk/toolkit/layout/Pair.h"
#include "ttk/toolkit/layout/Panel.h"
#include "ttk/toolkit/layout/Picture.h"
#include "ttk/toolkit/layout/Rule.h"
#include "ttk/toolkit/layout/Scroll.h"
#include "ttk/toolkit/layout/Spacer.h"
#include "ttk/toolkit/layout/Wrap.h"
#include "ttk/toolkit/overlays/Menu.h"
#include "ttk/toolkit/overlays/Tips.h"

namespace {
    enum class MenuAction : std::uint8_t {
        Open,
        Copy,
        Remove,
    };

    // A panel that lights up under the pointer. Panel leaves that to its owner.
    class HoverPanel : public ttk::Panel {
    public:
        HoverPanel() {
            _takesPointer = true;
            hoverable = true;
        }

        void enter() override {
            Widget::enter();
            lit = true;
            invalidate();
        }

        void leave() override {
            Widget::leave();
            lit = false;
            invalidate();
        }
    };

    // Everything a control on the page reaches for.
    struct Gallery {
        ttk::Shell shell;
        ttk::Notifier notifier;
        ttk::FilePicker picker{&notifier};
        ttk::FilePickerDialog *picking = nullptr;
        ttk::DialogLayer *dialogs = nullptr;
        ttk::Toasts *toasts = nullptr;
        ttk::Tips *tips = nullptr;
        ttk::Box *bar = nullptr;
        ttk::Widget *menu = nullptr;

        ttk::Button *button = nullptr;
        ttk::Label *label = nullptr;
        ttk::Pill *pill = nullptr;
        ttk::StatusIndicator *status = nullptr;
        ttk::Field *field = nullptr;
        ttk::Select *select = nullptr;
        ttk::Stepper *stepper = nullptr;
        ttk::Toggle *toggle = nullptr;
        ttk::Check *check = nullptr;
        ttk::MultistateSwitch *shade = nullptr;
        ttk::TabStrip *tabs = nullptr;
        ttk::GlyphButton *cog = nullptr;
        ttk::CollapsiblePanel *folding = nullptr;
        ttk::DisclosureHeading *disclosure = nullptr;
        ttk::Label *anchor = nullptr;

        int kind = 0;
        int style = 0;
        int badge = 0;
        int state = 0;
        int severity = 0;
        bool disclosed = false;
    };

    constexpr const char *KINDS[] = {"Default", "Primary", "Danger", "Ghost"};
    constexpr const char *STATES[] = {"", "Starting", "Running", "Stopping", "Closed", "Failed"};
    constexpr const char *BADGES[] = {"Accent", "Muted", "Success", "Warning", "Danger"};

    int mode_index() {
        switch (ttk::Theme::mode()) {
            case ttk::Theme::Mode::Light:
                return 1;
            case ttk::Theme::Mode::Dark:
                return 2;
            default:
                return 0;
        }
    }

    // Every glyph on one strip, drawn the way a glyph button draws one.
    BLImage glyph_sheet() {
        constexpr float WEIGHT = 1.6F;
        constexpr int CELL = 34;
        const int count = static_cast<int>(ttk::Glyphs::Glyph::Count) - 1;

        BLImage image(CELL * count, CELL, BL_FORMAT_PRGB32);
        BLContext context(image);

        context.clear_all();

        const double inset = (CELL - ttk::Glyphs::span(WEIGHT)) / 2.0;

        for (int at = 0; at < count; ++at) {
            ttk::Glyphs::draw(context, static_cast<ttk::Glyphs::Glyph>(at + 1),
                              BLPoint{(at * CELL) + inset, inset}, WEIGHT, ttk::Theme::palette().text);
        }

        context.end();

        return image;
    }

    // One row of the page: a name on the left, the widget on the right.
    ttk::Box *entry(ttk::Box *page, const std::string &name) {
        ttk::Box *made = page->append(ttk::Box::row());

        made->spacing(ttk::Theme::gap)->cross(ttk::Box::Place::Centre);
        made->append(std::make_unique<ttk::Label>(name))
            ->font(ttk::Theme::palette().headingWeight, ttk::Theme::fontSmall)->tone(&ttk::Theme::Palette::muted)
            ->fixedWidth = 180.0;

        return made;
    }

    // A line that opens something when clicked.
    ttk::Label *link(ttk::Box *row, const std::string &text, std::function<void()> clicked) {
        return row->append(std::make_unique<ttk::Label>(text))
            ->tone(&ttk::Theme::Palette::accent)->on_click(std::move(clicked));
    }

    void restyle(Gallery &gallery) {
        ttk::Label *label = gallery.label;

        label->font(400, ttk::Theme::fontBody)->tone(&ttk::Theme::Palette::text)->mono(false)->path(false)->wrap(false);

        switch (gallery.style) {
            case 0:
                label->set_text("Body text. Click to see the next style.");
                break;
            case 1:
                label->font(ttk::Theme::palette().headingWeight, ttk::Theme::fontTitle);
                label->set_text("A title, in the heading weight.");
                break;
            case 2:
                label->font(400, ttk::Theme::fontSmall)->tone(&ttk::Theme::Palette::muted);
                label->set_text("Small and muted, for a note.");
                break;
            case 3:
                label->mono();
                label->set_text("mono, for what a process was given");
                break;
            case 4:
                label->path();
                label->set_text("/home/user/.local/share/gallery/a/path/that/is/elided/from/the/middle/when/room/runs/out.txt");
                break;
            default:
                label->wrap();
                label->set_text("A wrapped paragraph folds at spaces and keeps going for as many lines as the "
                                "width asks for, which is how a note under a control or the body of a dialog is set.");
                break;
        }

        gallery.shell.ui().relayout();
    }

    void rebadge(Gallery &gallery) {
        constexpr ttk::Pill::Kind KIND[] = {ttk::Pill::Kind::None, ttk::Pill::Kind::Muted, ttk::Pill::Kind::Success,
                                            ttk::Pill::Kind::Warning, ttk::Pill::Kind::Danger};

        gallery.badge = (gallery.badge + 1) % 5;
        gallery.pill->kind(KIND[gallery.badge])->dot(gallery.badge % 2 == 1)
            ->glyph(gallery.badge == 2 ? ttk::Glyphs::Glyph::Check : ttk::Glyphs::Glyph::Empty);
        gallery.pill->set_text(BADGES[gallery.badge]);
        gallery.shell.ui().relayout();
    }

    void show_menu(Gallery &gallery) {
        ttk::Root &root = gallery.shell.ui();

        if (root.has_dismiss()) {
            root.dismiss();

            return;
        }

        const std::vector<ttk::Menu::Row> rows = {
            ttk::Menu::item(MenuAction::Open, "Open", ttk::Glyphs::Glyph::Folder),
            ttk::Menu::item(MenuAction::Copy, "Copy", ttk::Glyphs::Glyph::Copy, false, true),
            ttk::Menu::rule(),
            ttk::Menu::item(MenuAction::Remove, "Remove", ttk::Glyphs::Glyph::Trash, true),
        };

        const double tall = ttk::Menu::height_of(rows);
        const BLRect at = gallery.anchor->box();
        const bool below = at.y + at.h + 4.0 + tall <= root.height();

        gallery.menu = root.layer(ttk::Root::POPUPS)->add(std::make_unique<ttk::Menu>(rows, [&gallery](const int action) {
            gallery.shell.ui().dismiss();

            switch (static_cast<MenuAction>(action)) {
                case MenuAction::Open:
                    gallery.notifier.info("Open picked");
                    break;
                case MenuAction::Remove:
                    gallery.notifier.error("Remove picked");
                    break;
                default:
                    break;
            }
        }));

        gallery.menu->place(BLRect{at.x, below ? at.y + at.h + 4.0 : at.y - tall - 4.0, ttk::Menu::WIDTH, tall},
                            root.type());

        root.set_dismiss([&gallery] {
            if (gallery.menu != nullptr) {
                gallery.shell.ui().layer(ttk::Root::POPUPS)->erase(gallery.menu);
                gallery.menu = nullptr;
            }
        }, gallery.anchor);
    }

    void post_notice(Gallery &gallery) {
        switch (gallery.severity++ % 4) {
            case 0:
                gallery.notifier.info("Something happened.", "Info");
                break;
            case 1:
                gallery.notifier.success("It worked.", "Success");
                break;
            case 2:
                gallery.notifier.warning("Have a look at this.", "Warning");
                break;
            default:
                gallery.notifier.error("An error stays until it is dismissed.", "Error");
                break;
        }
    }

    void controls(Gallery &gallery, ttk::Box *page) {
        gallery.button = entry(page, "Button")->append(std::make_unique<ttk::Button>("Default", [&gallery] {
            gallery.button->busy(true);

            gallery.shell.after(1.0, [&gallery] {
                gallery.kind = (gallery.kind + 1) % 4;
                gallery.button->busy(false);
                gallery.button->kind(static_cast<ttk::Button::Kind>(gallery.kind));
                gallery.button->set_text(KINDS[gallery.kind]);
            });
        }));

        gallery.button->glyph(ttk::Glyphs::Glyph::Play)->tooltip("Busy for a second, then the next kind");

        gallery.label = entry(page, "Label")->append(std::make_unique<ttk::Label>());
        gallery.label->stretch = 1.0;
        gallery.label->hint = "Click for the next style";
        gallery.label->on_click([&gallery] {
            gallery.style = (gallery.style + 1) % 6;
            restyle(gallery);
        });
        restyle(gallery);

        gallery.pill = entry(page, "Pill")->append(std::make_unique<ttk::Pill>(BADGES[0]));
        gallery.pill->hint = "Every kind in turn, on a timer";

        entry(page, "Chip")->append(std::make_unique<ttk::Chip>("1.4.2", "A version, said in full when rested on"));

        entry(page, "Fact")->append(std::make_unique<ttk::Fact>("Location", "/home/user/.local/share/gallery"))
            ->path()->on_click("Open the location", [&gallery] { gallery.notifier.info("Fact clicked"); });

        gallery.status = entry(page, "Status indicator")->append(std::make_unique<ttk::StatusIndicator>());
        gallery.status->set(ttk::StatusIndicator::Status::Empty);
        gallery.status->on_click([&gallery] {
            gallery.state = (gallery.state + 1) % 6;
            gallery.status->set(static_cast<ttk::StatusIndicator::Status>(gallery.state), STATES[gallery.state]);
            gallery.shell.ui().relayout();
        }, "Click for the next state");

        gallery.field = entry(page, "Field")->append(std::make_unique<ttk::Field>("Address", [&gallery](const std::string &text) {
            gallery.field->badge(text.empty() ? "Empty" : std::to_string(text.size()) + " chars",
                                 text.empty() ? ttk::Pill::Kind::Muted : ttk::Pill::Kind::Success);
        }));
        gallery.field->stretch = 1.0;
        gallery.field->prefix("https://")->placeholder("example.org")
            ->badge("Empty", ttk::Pill::Kind::Muted)->note("The badge follows what is typed.")
            ->icon(ttk::Glyphs::Glyph::Folder, "Browse", [&gallery] { gallery.notifier.info("Browse pressed"); });

        entry(page, "Text box")->append(std::make_unique<ttk::TextBox>([](const std::string &) {}))
            ->placeholder("A bare line of input, without a label")->stretch = 1.0;

        gallery.select = entry(page, "Select")->append(std::make_unique<ttk::Select>("Choice", [&gallery](const int index) {
            gallery.select->set_current(index);
        }));
        gallery.select->stretch = 1.0;
        gallery.select->set_options({"First", "Second", "Third"});
        gallery.select->set_badges({"", "new", ""});
        gallery.select->placeholder("Nothing picked")->clearable()->tooltip("One of a list, and clearable");

        gallery.stepper = entry(page, "Stepper")->append(std::make_unique<ttk::Stepper>("Count", [&gallery](const int value) {
            gallery.stepper->set_value(value);
        }));
        gallery.stepper->range(1, 32)->clearable(0, "Auto")->tooltip("A number in a range, with an off value");
        gallery.stepper->set_value(4);

        gallery.toggle = entry(page, "Toggle")->append(std::make_unique<ttk::Toggle>("Off", [&gallery](const bool on) {
            gallery.toggle->set_checked(on);
            gallery.toggle->set_text(on ? "On" : "Off");
        }));
        gallery.toggle->hint = "On or off, with a word for which";

        gallery.check = entry(page, "Check")->append(std::make_unique<ttk::Check>([&gallery](const bool on) {
            gallery.check->set_checked(on);
        }));
        gallery.check->hint = "A bare check";

        gallery.tabs = entry(page, "Tab strip")->append(std::make_unique<ttk::TabStrip>([&gallery](const int index) {
            gallery.tabs->set_current(index);
            gallery.tabs->set_badge(index, false);
        }));
        gallery.tabs->set_tabs({{.label = "First"}, {.label = "Second", .badge = true}, {.label = "Third"}});
        gallery.tabs->set_current(0);
        gallery.tabs->hint = "One of a row of pages. The dot marks one that wants a look.";
    }

    void layouts(Gallery &gallery, ttk::Box *page) {
        auto view = std::make_unique<ttk::TextView>();
        std::vector<std::string> rows;

        for (int at = 0; at < 8; ++at) {
            rows.push_back(std::to_string(at) + (at == 3 ? "  warning: " : "  ") + "a line of output, one row each");
        }

        view->face(ttk::Typeface::mono, ttk::Theme::fontSmall);
        view->ink([](const size_t index) { return index == 3 ? ttk::Theme::palette().warning : ttk::Theme::palette().text; });
        view->set_rows(std::move(rows));
        view->hint = "Rows with a tone each, selectable";
        entry(page, "Text view")->append(std::move(view))->stretch = 1.0;

        ttk::Pair *pair = entry(page, "Pair")->append(std::make_unique<ttk::Pair>(240.0));
        pair->stretch = 1.0;
        pair->spacing(ttk::Theme::gap);
        pair->append(std::make_unique<ttk::Label>("The left half of a pair."))->font(400, ttk::Theme::fontSmall)->wrap();
        pair->append(std::make_unique<ttk::Label>("The right half, which stacks under the left once the page is too narrow."))
            ->font(400, ttk::Theme::fontSmall)->wrap();

        ttk::Wrap *wrap = entry(page, "Wrap")->append(std::make_unique<ttk::Wrap>());
        wrap->stretch = 1.0;
        wrap->spacing(ttk::Theme::gap, 6.0);

        for (const char *word : {"a", "wrap", "starts", "another", "line", "rather", "than", "run", "past", "its", "width"}) {
            wrap->append(std::make_unique<ttk::Label>(word))->font(400, ttk::Theme::fontSmall);
        }

        const BLImage sheet = glyph_sheet();
        ttk::Box *strip = entry(page, "Picture");
        ttk::Picture *picture = strip->append(std::make_unique<ttk::Picture>(sheet));

        strip->cross(ttk::Box::Place::Start);
        picture->fixedWidth = sheet.width();
        picture->fixedHeight = sheet.height();
        picture->hint = "Every glyph in the set, drawn onto one image";

        ttk::CardGrid *grid = entry(page, "Card grid")->append(std::make_unique<ttk::CardGrid>(ttk::ReorderGrid::Metrics{
            .bleed = 0.0, .gutter = 16.0, .top = 0.0, .narrowest = 250.0, .row_height = 152.0, .step = 8.0}));

        grid->stretch = 1.0;
        grid->reordered = [&gallery](const int from, const int to) {
            gallery.notifier.info("Card " + std::to_string(from + 1) + " moved to " + std::to_string(to + 1));
        };

        for (int at = 0; at < 6; ++at) {
            ttk::Card *card = grid->add(std::make_unique<ttk::Card>());

            card->title = "Card " + std::to_string(at + 1);
            card->subtitle = "Drag it to another cell.";
            card->told = at == 2 ? "Something went wrong here" : "Ready";
            card->trouble = at == 2;
            card->tags = {{.text = at % 2 == 0 ? "even" : "odd", .kind = at % 2 == 0 ? ttk::Pill::Kind::Success : ttk::Pill::Kind::Muted,
                           .dot = at % 2 == 0, .hint = "A badge on the card"}};
            card->opened = [&gallery, at] { gallery.notifier.info("Card " + std::to_string(at + 1) + " opened"); };
        }

        HoverPanel *panel = entry(page, "Panel")->append(std::make_unique<HoverPanel>());
        panel->stretch = 1.0;
        panel->append(ttk::Box::column())->pad(ttk::Theme::gap)
            ->append(std::make_unique<ttk::Label>("A panel that lights under the pointer."))->font(400, ttk::Theme::fontSmall);

        gallery.folding = entry(page, "Collapsible panel")->append(std::make_unique<ttk::CollapsiblePanel>("Folds away", [&gallery](const bool open) {
            gallery.folding->set_open(open);
        }));
        gallery.folding->stretch = 1.0;
        gallery.folding->body()->pad(ttk::Theme::gap)->spacing(6.0);
        gallery.folding->body()->append(std::make_unique<ttk::Label>("What the panel holds while it is open."))
            ->font(400, ttk::Theme::fontSmall)->wrap();

        gallery.disclosure = entry(page, "Disclosure heading")->append(std::make_unique<ttk::DisclosureHeading>("Turns", [&gallery] {
            gallery.disclosed = !gallery.disclosed;
            gallery.disclosure->set_open(gallery.disclosed);
        }));
        gallery.disclosure->stretch = 1.0;
        gallery.disclosure->body()->pad(0.0, 6.0)->spacing(6.0);
        gallery.disclosure->body()->append(std::make_unique<ttk::Label>("What the heading hides until it is turned."))
            ->font(400, ttk::Theme::fontSmall)->wrap();
    }

    void overlays(Gallery &gallery, ttk::Box *page) {
        link(entry(page, "Confirm dialog"), "Ask a question with two answers", [&gallery] {
            gallery.dialogs->show(std::make_unique<ttk::ConfirmDialog>(
                "Throw it away?", "It cannot be brought back.", "Throw away", true,
                [&gallery] { gallery.notifier.success("Thrown away"); }));
        });

        link(entry(page, "Prompt dialog"), "Ask for one line", [&gallery] {
            gallery.dialogs->show(std::make_unique<ttk::PromptDialog>(
                "Name it", "Name", "untitled", "Keep",
                [&gallery](const std::string &named) { gallery.notifier.success("Named " + named); }));
        });

        gallery.anchor = link(entry(page, "Menu"), "Open a menu under this line", [&gallery] { show_menu(gallery); });

        link(entry(page, "Notice"), "Post a notice in the corner", [&gallery] { post_notice(gallery); });

        link(entry(page, "File picker"), "Pick a file, with a toggle of the application's own", [&gallery] {
            gallery.picker.open("Pick anything", {"*"}, false, true, true, "gallery",
                                [&gallery](const std::vector<std::string> &paths, const bool option) {
                                    gallery.notifier.success(std::to_string(paths.size()) + " picked"
                                                             + (option ? ", with the option on" : ""));
                                },
                                "Also this", "A toggle the application asked the picker to carry");
        });

        entry(page, "Tooltip")->append(std::make_unique<ttk::Label>("Rest the pointer here"))
            ->hint = "Every widget on the page carries one of these.";
    }

    void bar(Gallery &gallery, ttk::Box *into) {
        gallery.bar = into->append(ttk::Box::row());
        gallery.bar->pad(ttk::Theme::pad, 0.0)->spacing(ttk::Theme::gap)->cross(ttk::Box::Place::Centre);
        gallery.bar->fixedHeight = ttk::Theme::barHeight;

        gallery.bar->append(std::make_unique<ttk::Label>("tinytk gallery"))
            ->font(ttk::Theme::palette().headingWeight, ttk::Theme::fontTitle);
        gallery.bar->append(std::make_unique<ttk::Spacer>());

        gallery.shade = gallery.bar->append(std::make_unique<ttk::MultistateSwitch>([&gallery](const int value) {
            constexpr ttk::Theme::Mode MODES[] = {ttk::Theme::Mode::System, ttk::Theme::Mode::Light, ttk::Theme::Mode::Dark};

            ttk::Theme::set_mode(MODES[value]);
            ttk::Shell::set_outline(ttk::Theme::palette().borderStrong);
            gallery.shade->set_current(value);
            gallery.shell.ui().damage_all();
        }));

        gallery.shade->set_options({{.value = 0, .label = "System"}, {.value = 1, .label = "Light"}, {.value = 2, .label = "Dark"}});
        gallery.shade->set_current(mode_index());
        gallery.shade->hint = "Which shade the window is painted in";

        gallery.cog = gallery.bar->append(std::make_unique<ttk::GlyphButton>(ttk::Glyphs::Glyph::Cog, [&gallery] {
            gallery.cog->spin();
            gallery.notifier.info("Settings would open here");
        }));

        gallery.cog->size(34.0)->tooltip("Spins a turn when pressed, the way a settings cog does");

        gallery.bar->append(std::make_unique<ttk::GlyphButton>(ttk::Glyphs::Glyph::Close, [&gallery] { gallery.shell.stop(); }))
            ->size(34.0)->tone(&ttk::Theme::Palette::text, &ttk::Theme::Palette::danger)->tooltip("Close the window");
    }

    void build(Gallery &gallery) {
        ttk::Root &root = gallery.shell.ui();
        ttk::Box *window = root.content()->append(ttk::Box::column());

        bar(gallery, window);
        window->append(std::make_unique<ttk::Rule>());

        ttk::Scroll *scroll = window->append(std::make_unique<ttk::Scroll>());
        scroll->stretch = 1.0;

        auto column = ttk::Box::column();
        ttk::Box *page = column.get();

        page->pad(ttk::Theme::pad)->spacing(ttk::Theme::gap);
        scroll->hold(std::move(column));

        controls(gallery, page);
        layouts(gallery, page);
        overlays(gallery, page);

        gallery.dialogs = root.layer(ttk::Root::DIALOGS)->append(std::make_unique<ttk::DialogLayer>());
        gallery.toasts = root.layer(ttk::Root::NOTICES)->append(std::make_unique<ttk::Toasts>([&gallery](const int id) {
            gallery.notifier.dismiss(id);
        }));
        gallery.tips = root.layer(ttk::Root::TIPS)->append(std::make_unique<ttk::Tips>());
    }

    void wire(Gallery &gallery) {
        ttk::Shell &shell = gallery.shell;

        gallery.notifier.changed = [&gallery] {
            gallery.shell.post([&gallery] { gallery.toasts->set_messages(gallery.notifier.messages()); });
        };

        // The picker says when it opens and closes. Its dialog follows.
        gallery.picker.changed = [&gallery] {
            gallery.shell.post([&gallery] {
                const bool wants = gallery.picker.state().open;

                if (wants && gallery.picking == nullptr) {
                    gallery.picking = static_cast<ttk::FilePickerDialog *>(
                        gallery.dialogs->show(std::make_unique<ttk::FilePickerDialog>(gallery.picker)));
                } else if (!wants && gallery.picking != nullptr) {
                    gallery.dialogs->dismiss();
                }

                if (gallery.picking != nullptr) {
                    gallery.picking->sync();
                }
            });
        };

        gallery.dialogs->closed = [&gallery](const ttk::Dialog *gone) {
            if (gone == gallery.picking) {
                gallery.picking = nullptr;
                gallery.picker.dismiss();
            }
        };

        shell.every(1.5, [&gallery] { rebadge(gallery); });

        shell.draggable = [&gallery](const double x, const double y) {
            return !gallery.dialogs->covered() && !gallery.shell.ui().has_dismiss()
                && gallery.bar->holds(x, y) && gallery.bar->at(x, y) == nullptr;
        };

        shell.shortcut = [&gallery](const ttk::Key &pressed) {
            if (pressed.code != ttk::Code::Escape) {
                return false;
            }

            if (gallery.shell.ui().has_dismiss()) {
                gallery.shell.ui().dismiss();

                return true;
            }

            if (gallery.dialogs->covered()) {
                gallery.dialogs->close();

                return true;
            }

            return false;
        };

        shell.shadeChanged = [&gallery] {
            ttk::Shell::set_outline(ttk::Theme::palette().borderStrong);
            gallery.shell.ui().damage_all();
        };

        shell.resized = [&gallery](double, double) { gallery.shell.ui().relayout(); };

        shell.settle = [&gallery] {
            const ttk::Widget *over = gallery.shell.ui().hovered();
            const double x = gallery.shell.ui().pointer_x();
            const double y = gallery.shell.ui().pointer_y();

            if (over != nullptr && !over->hint.empty()) {
                gallery.tips->point(over->hint, over->box(), x, y, ttk::Shell::now());
            } else {
                gallery.tips->point({}, BLRect{}, x, y, ttk::Shell::now());
            }
        };

        shell.closing = [&gallery] { gallery.shell.stop(); };
    }
}

int main() {
    Gallery gallery;

    if (!gallery.shell.start("tinytk gallery", 1180, 800)) {
        return 1;
    }

    ttk::Shell::set_outline(ttk::Theme::palette().borderStrong);

    build(gallery);
    wire(gallery);

    gallery.shell.run();

    return 0;
}
