// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of tinytk
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>
#include <ranges>

#include "ttk/shell/Shell.h"
#include "ttk/draw/Chrome.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Compose.h"

namespace ttk {
    // SDL's callbacks want plain function pointers with its own signatures, and both
    // reach into the Shell they were handed.
    struct ShellHooks {
        static SDL_HitTestResult SDLCALL hit_test(SDL_Window *window, const SDL_Point *at, void *held);
        static bool SDLCALL watch(void *held, SDL_Event *event);
    };

    namespace {

        // How close to an edge a press starts a resize.
        constexpr double EDGE = 6.0;

        // Blend2D decodes to premultiplied alpha and SDL wants it straight.
        void dress_window(SDL_Window *window, const BLImage &mark) {
            BLImageData pixels{};

            if (mark.is_empty() || mark.get_data(&pixels) != BL_SUCCESS) {
                return;
            }

            std::vector<Uint32> straight(static_cast<size_t>(pixels.size.w) * pixels.size.h);

            for (int y = 0; y < pixels.size.h; ++y) {
                const auto *line = reinterpret_cast<const Uint32 *>(
                    static_cast<const std::byte *>(pixels.pixel_data) + (y * pixels.stride));

                for (int x = 0; x < pixels.size.w; ++x) {
                    const Uint32 argb = line[x];
                    const Uint32 alpha = argb >> 24U;

                    Uint32 undone = alpha << 24U;

                    if (alpha != 0) {
                        for (unsigned shift = 0; shift < 24U; shift += 8U) {
                            const Uint32 part = std::min(255U, (((argb >> shift) & 0xffU) * 255U) / alpha);

                            undone |= part << shift;
                        }
                    }

                    straight[(static_cast<size_t>(y) * pixels.size.w) + x] = undone;
                }
            }

            SDL_Surface *icon = SDL_CreateSurfaceFrom(pixels.size.w, pixels.size.h,
                                                      SDL_PIXELFORMAT_ARGB8888, straight.data(),
                                                      pixels.size.w * 4);

            if (icon != nullptr) {
                SDL_SetWindowIcon(window, icon);
                SDL_DestroySurface(icon);
            }
        }

        ttk::Click button_of(const Uint8 which) {
            switch (which) {
                case SDL_BUTTON_MIDDLE:
                    return ttk::Click::Middle;

                case SDL_BUTTON_RIGHT:
                    return ttk::Click::Right;

                case SDL_BUTTON_X1:
                    return ttk::Click::Back;

                case SDL_BUTTON_X2:
                    return ttk::Click::Forward;

                default:
                    return ttk::Click::Left;
            }
        }

    }

    Shell::Shell() = default;

    Shell::~Shell() {
        for (const auto &made: _cursors | std::views::values) {
            SDL_DestroyCursor(made);
        }

        if (_window != nullptr) {
            _surface.detach();
            SDL_DestroyWindow(_window);
        }

        SDL_Quit();
    }

    double Shell::now() {
        return static_cast<double>(SDL_GetTicksNS()) / 1000000000.0;
    }

    bool Shell::start(const std::string &title, const int width, const int height) {
        SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

        if (!SDL_Init(SDL_INIT_VIDEO)) {
            return false;
        }

        // Only the Windows and X11 drivers have a framebuffer of their own. Pinned to
        // it the window surface is plain memory and no graphics device is opened.
        if (const char *driver = SDL_GetCurrentVideoDriver();
            driver != nullptr
            && (SDL_strcmp(driver, "windows") == 0 || SDL_strcmp(driver, "x11") == 0)) {
            SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0");
        }

        _window = SDL_CreateWindow(title.c_str(), width, height,
                                   SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS
                                       | SDL_WINDOW_HIDDEN);

        if (_window == nullptr) {
            return false;
        }

        SDL_SetWindowMinimumSize(_window, _minWidth, _minHeight);

        dress_window(_window, _icon);

        if (!_surface.attach(_window) || !_type.load()) {
            return false;
        }

        Theme::set_system_dark(SDL_GetSystemTheme() != SDL_SYSTEM_THEME_LIGHT);

        _root = std::make_unique<ttk::Root>(_type);

        _root->composing = [this](const bool wanted) {
            if (wanted) {
                SDL_StartTextInput(_window);
            } else {
                SDL_StopTextInput(_window);
            }
        };

        Chrome::apply(_window);

        SDL_SetWindowHitTest(_window, ShellHooks::hit_test, this);

        _wakeEvent = SDL_RegisterEvents(1);
        _ready = true;

        return true;
    }

    SDL_HitTestResult SDLCALL ShellHooks::hit_test(SDL_Window *window, const SDL_Point *at,
                                                 void *held) {
        const auto *shell = static_cast<Shell *>(held);

        int width = 0;
        int height = 0;

        SDL_GetWindowSize(window, &width, &height);

        if (!shell->_maximized) {
            const bool left = at->x < EDGE;
            const bool right = at->x >= width - EDGE;
            const bool top = at->y < EDGE;
            const bool bottom = at->y >= height - EDGE;

            if (top && left) {
                return SDL_HITTEST_RESIZE_TOPLEFT;
            }
            if (top && right) {
                return SDL_HITTEST_RESIZE_TOPRIGHT;
            }
            if (bottom && left) {
                return SDL_HITTEST_RESIZE_BOTTOMLEFT;
            }
            if (bottom && right) {
                return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
            }
            if (left) {
                return SDL_HITTEST_RESIZE_LEFT;
            }
            if (right) {
                return SDL_HITTEST_RESIZE_RIGHT;
            }
            if (top) {
                return SDL_HITTEST_RESIZE_TOP;
            }
            if (bottom) {
                return SDL_HITTEST_RESIZE_BOTTOM;
            }
        }

        if (shell->_root != nullptr && shell->_root->grabbed() != nullptr) {
            return SDL_HITTEST_NORMAL;
        }

        if (shell->draggable && shell->draggable(at->x, at->y)) {
            return SDL_HITTEST_DRAGGABLE;
        }

        return SDL_HITTEST_NORMAL;
    }

    void Shell::set_outline(const BLRgba32 edge) {
        Chrome::outline(static_cast<std::uint8_t>(edge.r()), static_cast<std::uint8_t>(edge.g()),
                        static_cast<std::uint8_t>(edge.b()));
    }

    void Shell::minimize() const {
        SDL_MinimizeWindow(_window);
    }

    void Shell::toggle_maximize() const {
        if (_maximized) {
            SDL_RestoreWindow(_window);
        } else {
            SDL_MaximizeWindow(_window);
        }
    }

    int Shell::every(const double seconds, std::function<void()> what) {
        const int id = _nextAlarm++;

        _alarms.push_back(Alarm{
            .id = id,
            .due = now() + seconds,
            .every = seconds,
            .what = std::move(what),
        });

        return id;
    }

    int Shell::after(const double seconds, std::function<void()> what) {
        const int id = _nextAlarm++;

        _alarms.push_back(Alarm{
            .id = id,
            .due = now() + seconds,
            .every = 0.0,
            .what = std::move(what),
        });

        return id;
    }

    void Shell::cancel(const int id) {
        std::erase_if(_alarms, [id](const Alarm &alarm) { return alarm.id == id; });
    }

    void Shell::set_cursor(const ttk::Cursor wanted) {
        if (wanted == _cursor) {
            return;
        }

        _cursor = wanted;

        const auto held = _cursors.find(wanted);

        if (held != _cursors.end()) {
            SDL_SetCursor(held->second);

            return;
        }

        SDL_SystemCursor system = SDL_SYSTEM_CURSOR_DEFAULT;

        switch (wanted) {
            case ttk::Cursor::Pointer:
                system = SDL_SYSTEM_CURSOR_POINTER;

                break;

            case ttk::Cursor::Text:
                system = SDL_SYSTEM_CURSOR_TEXT;

                break;

            case ttk::Cursor::Resize:
                system = SDL_SYSTEM_CURSOR_NS_RESIZE;

                break;

            case ttk::Cursor::Grab:
            case ttk::Cursor::Grabbing:
                system = SDL_SYSTEM_CURSOR_MOVE;

                break;

            default:
                break;
        }

        SDL_Cursor *made = SDL_CreateSystemCursor(system);

        _cursors[wanted] = made;

        if (made != nullptr) {
            SDL_SetCursor(made);
        }
    }

    void Shell::post(std::function<void()> what) {
        {
            const std::scoped_lock held(_posted);

            _errands.push_back(std::move(what));
        }

        // A loop asleep in SDL_WaitEventTimeout has to be told, and only an event does
        // that from another thread.
        SDL_Event wake{};

        wake.type = _wakeEvent;

        SDL_PushEvent(&wake);
    }

    void Shell::errands() {
        std::vector<std::function<void()>> due;

        {
            const std::scoped_lock held(_posted);

            due.swap(_errands);
        }

        for (const std::function<void()> &what : due) {
            what();
        }
    }

    bool Shell::alarms(const double at) {
        // Ids only, and into a vector that is kept: an alarm may add or cancel one while
        // it runs, so each is looked up again before it is touched.
        _due.clear();

        for (const Alarm &alarm : _alarms) {
            if (alarm.due <= at) {
                _due.push_back(alarm.id);
            }
        }

        for (const int id : _due) {
            const auto held = std::ranges::find_if(_alarms, [id](const Alarm &kept) {
                return kept.id == id;
            });

            if (held == _alarms.end()) {
                continue;
            }

            const std::function<void()> what = held->what;

            if (held->every > 0.0) {
                held->due = at + held->every;
            } else {
                _alarms.erase(held);
            }

            what();
        }

        return !_due.empty();
    }

    // The fastest the window is drawn, whatever the display can do past it.
    constexpr double MOST = 200.0;

    void Shell::read_refresh() {
        // SDL_GetCurrentDisplayMode enumerates the display's whole mode list, half a
        // second of it on a high-refresh monitor. The desktop mode reads the same rate.
        const SDL_DisplayMode *mode = SDL_GetDesktopDisplayMode(SDL_GetDisplayForWindow(_window));
        const double refresh = mode != nullptr && mode->refresh_rate > 1.0F
            ? static_cast<double>(mode->refresh_rate)
            : 60.0;

        // Whole divisions of the refresh only: a rate that does not divide it is held
        // for one refresh or two in turn, which is the judder a cap is meant to avoid.
        double rate = refresh;

        for (int part = 2; rate > MOST; ++part) {
            rate = refresh / part;
        }

        _frameGap = static_cast<Uint64>(1000000000.0 / rate);
    }

    bool Shell::due() const {
        return _frameGap == 0 || SDL_GetTicksNS() - _painted >= _frameGap;
    }

    int Shell::sleep_for(const double at) const {
        double soonest = _root->waking();

        for (const Alarm &alarm : _alarms) {
            if (soonest < 0.0 || alarm.due < soonest) {
                soonest = alarm.due;
            }
        }

        if (soonest < 0.0) {
            return 1000;
        }

        return std::clamp(static_cast<int>((soonest - at) * 1000.0), 0, 1000);
    }

    void Shell::relayout() {
        if (!_surface.sync(_window)) {
            return;
        }

        const double width = _surface.width();
        const double height = _surface.height();

        if (width == _width && height == _height) {
            return;
        }

        _width = width;
        _height = height;

        _root->resize(width, height);

        if (resized) {
            resized(width, height);
        }
    }

    void Shell::draw() {
        // settle() lays the tree out and starts what that moves. A frame drawn from a
        // clock last set some other time leaves those runs where they were and the
        // pixels they were at.
        const double at = now();

        // Every attempt counts against the rate, not only the ones that reach the
        // window: a frame that finds nothing to repaint still had its turn, and
        // leaving the mark stale lets the loop spin through the wait it owes.
        _painted = SDL_GetTicksNS();

        _root->set_now(at);
        _root->advance(at);

        relayout();

        if (!_surface.ready()) {
            return;
        }

        compose(*_root, _surface);

        _surface.present(_window);
    }

    bool SDLCALL ShellHooks::watch(void *held, SDL_Event *event) {
        auto *shell = static_cast<Shell *>(held);
        SDL_Window *window = shell->_window;

        if (!shell->_ready) {
            return true;
        }

        // A desktop that runs a resize inside a modal loop of its own never comes back
        // to ours until the drag is over. A watch is called as the event is pushed,
        // which happens inside that loop, so the frame is drawn from there.
        switch (event->type) {
            case SDL_EVENT_WINDOW_RESIZED: {
                // The damage stands, so the size it settles on is drawn by the expose
                // behind it or by the loop once the drag hands control back.
                if (!shell->due()) {
                    shell->_root->damage_all();

                    return true;
                }

                shell->relayout();

                // A resize that lands on the same size still gets SDL's pixels back
                // as a fresh block, so the frame is drawn whole either way.
                shell->_surface.damage_all();
                shell->_root->damage_all();

                // SDL throws the window surface away in the handler that runs *after*
                // this event, so it is still the old one here and the expose behind it
                // draws the frame instead.
                int wide = 0;
                int tall = 0;

                SDL_GetWindowSizeInPixels(window, &wide, &tall);

                if (wide != shell->_surface.width() || tall != shell->_surface.height()) {
                    break;
                }

                shell->draw();

                break;
            }

            case SDL_EVENT_WINDOW_EXPOSED:
                shell->_surface.damage_all();

                if (!shell->due()) {
                    shell->_root->damage_all();

                    break;
                }

                shell->draw();

                break;

            default:
                break;
        }

        return true;
    }

    void Shell::handle(const SDL_Event &event) {
        // A tween this event starts is clocked from now, not from the frame the loop
        // went to sleep after. Otherwise the first step after an idle spell jumps.
        _root->set_now(now());

        switch (event.type) {
            // A window dragged to another monitor, or the one it is on given a new
            // mode: either changes the rate the frames are held to.
            case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
            case SDL_EVENT_DISPLAY_CURRENT_MODE_CHANGED:
                read_refresh();

                break;

            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                if (closing) {
                    closing();
                }

                _running = false;

                break;

            // Anything that can make SDL throw the window's pixels away and allocate
            // them again. Off the direct path only the damaged rectangles are handed
            // over, so a fresh block has to be filled whole.
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            case SDL_EVENT_WINDOW_SHOWN:
            case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
            case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
            case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
                relayout();

                _surface.damage_all();
                _root->damage_all();

                break;

            case SDL_EVENT_WINDOW_MAXIMIZED:
            case SDL_EVENT_WINDOW_RESTORED:
                _maximized = event.type == SDL_EVENT_WINDOW_MAXIMIZED;

                _surface.damage_all();
                _root->damage_all();

                if (shadeChanged) {
                    shadeChanged();
                }

                break;

            case SDL_EVENT_SYSTEM_THEME_CHANGED:
                Theme::set_system_dark(SDL_GetSystemTheme() != SDL_SYSTEM_THEME_LIGHT);

                if (shadeChanged) {
                    shadeChanged();
                }

                break;

            case SDL_EVENT_MOUSE_MOTION:
                _root->motion(event.motion.x, event.motion.y);

                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                const ttk::Click which = button_of(event.button.button);

                if (which == ttk::Click::Back || which == ttk::Click::Forward) {
                    break;
                }

                const SDL_Keymod mods = SDL_GetModState();

                _root->press(ttk::Pointer{
                    .x = event.button.x,
                    .y = event.button.y,
                    .button = which,
                    .ctrl = (mods & SDL_KMOD_CTRL) != 0,
                    .shift = (mods & SDL_KMOD_SHIFT) != 0,
                });

                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_UP: {
                const ttk::Click which = button_of(event.button.button);

                if (which == ttk::Click::Back) {
                    if (back) {
                        back();
                    }

                    break;
                }

                if (which == ttk::Click::Forward) {
                    if (forward) {
                        forward();
                    }

                    break;
                }

                _root->release(ttk::Pointer{
                    .x = event.button.x,
                    .y = event.button.y,
                    .button = which,
                });

                break;
            }

            case SDL_EVENT_MOUSE_WHEEL:
                _root->wheel(event.wheel.y, event.wheel.mouse_x, event.wheel.mouse_y);

                break;

            case SDL_EVENT_WINDOW_MOUSE_LEAVE:
                _root->leave();

                break;

            case SDL_EVENT_KEY_DOWN: {
                const ttk::Key pressed{
                    .code = static_cast<int>(event.key.key),
                    .ctrl = (event.key.mod & SDL_KMOD_CTRL) != 0,
                    .shift = (event.key.mod & SDL_KMOD_SHIFT) != 0,
                    .alt = (event.key.mod & SDL_KMOD_ALT) != 0,
                };

                if (_root->key(pressed)) {
                    break;
                }

                if (shortcut && shortcut(pressed)) {
                    break;
                }

                break;
            }

            case SDL_EVENT_TEXT_INPUT:
                _root->wrote(event.text.text);

                break;

            default:
                break;
        }
    }

    void Shell::run() {
        SDL_AddEventWatch(ShellHooks::watch, this);

        _root->set_now(now());

        // The page holds nothing until settle fills it, and the first frame is drawn
        // here.
        if (settle) {
            settle();
        }

        _root->damage_all();

        draw();

        SDL_ShowWindow(_window);

        // The frame above went into a hidden window, where the blit goes nowhere. This
        // is the one the desktop actually shows.
        _surface.damage_all();
        draw();

        read_refresh();

        _painted = SDL_GetTicksNS();

        while (_running) {
            SDL_Event event;

            // Nothing in flight: block until the desktop or an alarm has something to
            // say. This is where the idle cost goes to nothing at all.
            if (!_root->busy() && !_root->dirty()) {
                if (SDL_WaitEventTimeout(&event, sleep_for(now()))) {
                    handle(event);
                }
            }

            while (SDL_PollEvent(&event)) {
                handle(event);
            }

            const double at = now();

            _root->set_now(at);

            errands();
            alarms(at);

            if (settle) {
                settle();
            }

            set_cursor(_root->cursor());

            // Waking on an event is not a reason to draw sooner than the display can
            // show it: a pointer reports many times a frame, and a turn that comes
            // back with nothing to repaint has cost a frame's work to find that out.
            if (const Uint64 since = SDL_GetTicksNS() - _painted; since < _frameGap) {
                SDL_DelayNS(_frameGap - since);
            }

            draw();
        }
    }

    void Shell::geometry(int &x, int &y, int &width, int &height) const {
        SDL_GetWindowPosition(_window, &x, &y);
        SDL_GetWindowSize(_window, &width, &height);
    }

    void Shell::set_geometry(const int x, const int y, const int width, const int height) const {
        if (width > 0 && height > 0) {
            SDL_SetWindowSize(_window, width, height);
        }

        if (x >= 0 && y >= 0) {
            SDL_SetWindowPosition(_window, x, y);
        }
    }
}
