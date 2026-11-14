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
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>
#include <blend2d/blend2d.h>

#include "ttk/draw/Surface.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"
#include "ttk/util/Clock.h"
#include "ttk/util/Window.h"

namespace ttk {
//! Window and frame loop of an application, everything between the desktop and the widgets.
//!
//! Set the icon and minimum size, call \ref start() to open the window, fill \ref ui() and the callbacks,
//! then call \ref run(). The window is borderless and draws its own title bar. While no widget animates, no
//! area needs repainting, no timer is due and nothing called \ref nudge(), the loop blocks until an event, a
//! \ref post() or the next timer, and the process stays idle. While the window is hidden, minimized or
//! occluded, nothing is drawn and animations wait until it is shown again.
//! Drawing is paced to the display's refresh rate, divided down to at most 200 frames per second.
class Shell : public Clock, public Window {
public:
    //! Creates a shell without a window. Call \ref start() to open it.
    Shell();

    //! Destroys the window and shuts SDL down.
    ~Shell() override;

    Shell(const Shell &) = delete;
    Shell &operator=(const Shell &) = delete;
    Shell(Shell &&) = delete;
    Shell &operator=(Shell &&) = delete;

    //! Opens a hidden window titled `title` of `width` by `height` logical pixels and returns true.
    //!
    //! Initialises SDL video, creates the window surface, loads the fonts, follows the desktop's light or dark
    //! preference and creates the root returned by \ref ui(). Returns false when the platform gives no window,
    //! no surface or no font, which the caller should turn into a clean exit. The window is shown by \ref run().
    bool start(const std::string &title, int width, int height);

    //! Sets the window icon to the decoded image `icon`. Call it before \ref start(), which applies it.
    void set_icon(BLImage icon) { _icon = std::move(icon); }

    //! Sets the smallest size the window can be resized to, in logical pixels. Defaults to 720 by 520. Call it
    //! before \ref start(), which applies it.
    void set_minimum_size(const int width, const int height) {
        _minWidth = width;
        _minHeight = height;
    }

    //! Runs the frame loop until \ref stop() is called or the desktop closes the window. Call it after a
    //! successful \ref start().
    //!
    //! Calls \ref settle, draws the first frame and shows the window. Each turn of the loop then handles
    //! events, runs functions queued by \ref post(), runs due timers, calls \ref settle, updates the cursor and
    //! draws whatever changed.
    void run();

    //! Makes \ref run() return once its current turn is over.
    void stop() override { _running = false; }

    //! Returns the root of the widget tree. Valid only after a successful \ref start().
    ttk::Root &ui() const { return *_root; }

    //! Returns the typeface loaded by \ref start().
    Typeface &type() { return _type; }

    //! Returns the SDL window, or null before \ref start() has created it.
    [[nodiscard]] SDL_Window *window() const { return _window; }

    //! Asks the desktop to minimize the window.
    void minimize() const override;

    //! Asks the desktop to restore the window when \ref maximized() is true, and to maximize it otherwise.
    void toggle_maximize() const override;

    //! Tests whether the window is maximized, as last reported by the desktop.
    [[nodiscard]] bool maximized() const override { return _maximized; }

    //! Sets the colour of the border the compositor draws around the window to `edge`. The alpha of `edge` is
    //! ignored. Does nothing outside Windows.
    static void set_outline(BLRgba32 edge);

    //! Called with a point `[x, y]` in window coordinates to ask whether a press there drags the window.
    //!
    //! Return true for empty parts of a title bar. Not called for the 6 pixel resize border of a window that
    //! is not maximized, nor while a widget holds the pointer grab. Null makes no point draggable.
    std::function<bool(double, double)> draggable;

    //! Called when the desktop asks for the window to close. The loop ends after it returns.
    std::function<void()> closing;

    //! Called with the new width and height, in pixels of the window surface, after the root has been resized
    //! to them.
    std::function<void(double, double)> resized;

    //! Called when the mouse back side button is released. Side buttons never reach the widgets.
    std::function<void()> back;

    //! Called when the mouse forward side button is released. Side buttons never reach the widgets.
    std::function<void()> forward;

    //! Called with each key press that neither the focused widget, its ancestors nor \ref Root::key() consumed,
    //! for the shortcuts the whole window owns, so Tab arrives only with Alt held. It returns true when it
    //! handled the key.
    std::function<bool(const ttk::Key &)> shortcut;

    //! Called once per turn of the loop, after events, posted functions and timers, and before the frame is
    //! drawn. Also called once by \ref run() before the first frame, which it can fill.
    std::function<void()> settle;

    //! Called when the desktop's light or dark preference changes, after the theme has followed it, and when
    //! the window is maximized or restored.
    std::function<void()> shadeChanged;

    //! Runs `what` every `seconds` until \ref cancel() is called with the returned id.
    //!
    //! The first run is `seconds` from now and each next one `seconds` after the turn of the loop that ran the
    //! previous one. A non-positive `seconds` runs `what` once, on the next turn. Call it on the interface
    //! thread only.
    int every(double seconds, std::function<void()> what) override;

    //! Runs `what` once, `seconds` from now, and returns an id that \ref cancel() accepts until then.
    //!
    //! Call it on the interface thread only.
    int after(double seconds, std::function<void()> what) override;

    //! Removes the timer `id` returned by \ref every() or \ref after(). Does nothing for an id that is not
    //! pending. Safe to call from inside a timer's callback. Call it on the interface thread only.
    void cancel(int id) override;

    //! Queues `what` to run on the interface thread on the next turn of the loop, and wakes the loop.
    //!
    //! Safe to call from any thread. Queued functions run in the order they were posted.
    void post(std::function<void()> what) override;

    //! Asks for one more turn of the loop, with its \ref settle, without waiting for an event.
    //!
    //! Call it when something changed that no event, timer or \ref post() will report, such as state touched
    //! while a frame was drawn. A call made before \ref settle runs in a turn is answered by that turn. Call it
    //! on the interface thread only: \ref post() is the way in from another thread.
    void nudge() { _nudged = true; }

    //! Shows `wanted` as the pointer cursor over the window. Does nothing when it is already shown.
    //!
    //! \ref run() calls it every turn with the cursor of the widget under the pointer. System cursors are
    //! created on first use and kept for the life of the shell.
    void set_cursor(ttk::Cursor wanted);

    //! Stores the position of the window in `x` and `y` and its size in `width` and `height`, all in logical
    //! pixels.
    //!
    //! While the window is maximized, minimized or fullscreen this is the last position and size it had
    //! outside those states, the ones a restore returns to. Test \ref maximized() for the state itself.
    //! Positions can be negative on a display left of or above the primary one. A Wayland compositor does not
    //! report where the window is, and the position stored there is `[0, 0]`.
    void geometry(int &x, int &y, int &width, int &height) const;

    //! Resizes the window to `width` by `height`, moves it to `at` and maximizes it when `maximized` is true, in
    //! logical pixels. Call it after \ref start() and before \ref run().
    //!
    //! The size is kept unless both `width` and `height` are positive. The position is kept when `at` is empty,
    //! which leaves it to the desktop. A position can be negative. It is moved with \ref Placement::fit() onto
    //! the usable areas of the connected displays, the primary one first, so a window saved on a display that
    //! is gone still opens where it can be reached. A Wayland compositor does not let a client place its
    //! window, and there `at` is ignored.
    //!
    //! With `maximized` true, \ref run() shows the window maximized. When the desktop restores it, it returns
    //! to the size and position given here.
    void set_geometry(std::optional<SDL_Point> at, int width, int height, bool maximized);

    //! Returns the time in seconds since SDL was initialised, the clock that frame times are measured on.
    [[nodiscard]] static double now();

private:
    struct Alarm {
        int id = 0;
        double due = 0.0;
        double every = 0.0;
        std::function<void()> what;
    };

    void handle(const SDL_Event &event);
    void keep_normal();
    void draw();
    bool relayout();

    bool alarms(double at);

    void errands();

    [[nodiscard]] int sleep_for(double at, bool unseen) const;

    // True while the desktop shows none of the window: hidden, minimized or occluded.
    [[nodiscard]] bool unseen() const;

    // True once the display has had time to show the last frame. A desktop drives
    // its resize loop as fast as we return, so unpaced it gets frames nothing shows.
    [[nodiscard]] bool due() const;

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

    std::mutex _posted;
    std::vector<std::function<void()>> _errands;
    unsigned _wakeEvent = 0;

    // The files of the drop in progress, which arrive one event each.
    std::vector<std::string> _dropping;

    double _width = 0.0;
    double _height = 0.0;

    // Position and size outside maximized, minimized and fullscreen, which SDL does not report in those states.
    SDL_Rect _normal{};

    int _minWidth = 720;
    int _minHeight = 520;

    bool _running = true;
    bool _maximized = false;
    bool _ready = false;
    bool _nudged = false;

    Uint64 _frameGap = 0;
    Uint64 _painted = 0;

    friend struct ShellHooks;
};
}


#endif //TTK_SHELL_SHELL_H
