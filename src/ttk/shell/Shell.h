// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef TTK_SHELL_SHELL_H
#define TTK_SHELL_SHELL_H


#include <functional>
#include <string>
#include <utility>
#include <map>
#include <mutex>
#include <vector>

#include <SDL3/SDL.h>
#include <blend2d/blend2d.h>

#include "ttk/draw/Surface.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"
#include "ttk/util/Clock.h"
#include "ttk/util/Window.h"

namespace ttk {
    // The window and the frame loop: everything between the desktop and the widgets.
    // With nothing in flight, due or dirty, the loop blocks in SDL_WaitEventTimeout
    // and the process costs nothing.
    class Shell : public Clock, public Window {
    public:
        Shell();
        ~Shell() override;

        Shell(const Shell &) = delete;
        Shell &operator=(const Shell &) = delete;
        Shell(Shell &&) = delete;
        Shell &operator=(Shell &&) = delete;

        // False when the platform gives no window, no surface or no font, which the
        // caller turns into a clean exit rather than a crash.
        bool start(const std::string &title, int width, int height);

        // The window icon, decoded. Set before start().
        void set_icon(BLImage icon) { _icon = std::move(icon); }

        // How small the window may be dragged. Set before start().
        void set_minimum_size(const int width, const int height) {
            _minWidth = width;
            _minHeight = height;
        }

        void run();

        void stop() override { _running = false; }

        ttk::Root &ui() const { return *_root; }

        Typeface &type() { return _type; }

        [[nodiscard]] SDL_Window *window() const { return _window; }

        void minimize() const override;
        void toggle_maximize() const override;

        [[nodiscard]] bool maximized() const override { return _maximized; }

        static void set_outline(BLRgba32 edge);

        // Where the window manager may take a press and drag the window.
        std::function<bool(double, double)> draggable;

        // The desktop asked for the window to close.
        std::function<void()> closing;

        std::function<void(double, double)> resized;

        // Mouse side buttons.
        std::function<void()> back;
        std::function<void()> forward;

        // Before anything else sees a key, for the shortcuts the whole window owns.
        std::function<bool(const ttk::Key &)> shortcut;

        // Once per turn of the loop, after the events and before anything is drawn.
        std::function<void()> settle;

        // Called when the desktop's light or dark preference changes.
        std::function<void()> shadeChanged;

        int every(double seconds, std::function<void()> what) override;
        int after(double seconds, std::function<void()> what) override;
        void cancel(int id) override;
        void post(std::function<void()> what) override;

        // Puts the pointer the widget under it asks for on the desktop.
        void set_cursor(ttk::Cursor wanted);

        // Where the window is and how big, in logical pixels.
        void geometry(int &x, int &y, int &width, int &height) const;
        void set_geometry(int x, int y, int width, int height) const;

        [[nodiscard]] static double now();

    private:
        struct Alarm {
            int id = 0;
            double due = 0.0;
            double every = 0.0;
            std::function<void()> what;
        };

        void handle(const SDL_Event &event);
        void draw();
        void relayout();

        bool alarms(double at);

        void errands();

        // Milliseconds to block for before the next alarm is due.
        [[nodiscard]] int sleep_for(double at) const;

        // True once the display has had time to show the last frame. A desktop drives
        // its resize loop as fast as we return, so unpaced it gets frames nothing shows.
        [[nodiscard]] bool due() const;

        // Reads the rate of the display the window is on.
        void read_refresh();

        SDL_Window *_window = nullptr;
        BLImage _icon;

        Surface _surface;
        Typeface _type;

        std::unique_ptr<ttk::Root> _root;

        std::vector<Alarm> _alarms;

        // Kept between turns so a frame with alarms due allocates nothing.
        std::vector<int> _due;
        int _nextAlarm = 1;

        // One of each, made on demand and kept for the life of the window.
        std::map<ttk::Cursor, SDL_Cursor *> _cursors;
        ttk::Cursor _cursor = ttk::Cursor::Default;

        // Work handed over from another thread.
        std::mutex _posted;
        std::vector<std::function<void()>> _errands;
        unsigned _wakeEvent = 0;

        double _width = 0.0;
        double _height = 0.0;

        int _minWidth = 720;
        int _minHeight = 520;

        bool _running = true;
        bool _maximized = false;
        bool _ready = false;

        Uint64 _frameGap = 0;
        Uint64 _painted = 0;

        friend struct ShellHooks;
    };
}


#endif //TTK_SHELL_SHELL_H
