# tinytk

Tiny ui toolkit on Blend2D and SDL3. Provides several premade controls and system utilities.

## Layout

`src/ttk/system` is `ttk::system`, a static library with no window and no
third-party dependency in it: process spawning, text helpers, environment and paths.
What does need one sits in a target of its own, so an application links only what it uses:

| Target        | What                  | Needs                                     |
|---------------|-----------------------|-------------------------------------------|
| `ttk::json`   | JSON over yyjson      | yyjson, fetched                           |
| `ttk::http`   | HTTP fetches          | libcurl, or WinHTTP on Windows            |
| `ttk::notify` | desktop notifications | nothing, it speaks D-Bus on its own       |

`src/ttk` beyond that is `ttk::ui`, a widget toolkit drawn with Blend2D
inside an SDL3 window. `draw` holds the rasteriser side, `toolkit` the
widgets, `shell` the window and frame loop, `dialogs` and `notices` the
pieces every application puts over its pages.

## Using it

Add the tree and link the targets the application uses.

```cmake
add_subdirectory(tinytk)
target_link_libraries(app PRIVATE ttk::ui ttk::http)
```

`TTK_UI`, `TTK_JSON`, `TTK_HTTP` and `TTK_NOTIFY` are all on. Turning one off skips
its target along with its fetches and lookups, so a build machine needs only what is kept. `TTK_DOWNLOAD_CACHE`
names where fetched sources are kept between build trees.

`ttk::ui` draws and presents on the CPU alone: into SDL's native framebuffer on Windows
and X11, and into `wl_shm` buffers of its own on Wayland. SDL is built without its render
API and graphics backends, so a video driver with no framebuffer of its own, such as the
one on macOS, cannot open a window. `TTK_WLSHM` is offered on Unix other than macOS and
Android, ON by default, and needs the wayland-client headers. With it OFF, SDL is built
without its Wayland driver and windows open through X11, or XWayland in a Wayland session.

`TTK_EXAMPLES=ON` builds `examples/*`, a basic hello world app and a gallery showing every widget,
dialog, menu, tooltip and notice the library has. ON by default if ttk is compiled standalone.

Before anything asks for a path or a request, tell the library who it is:

```cpp
ttk::Paths::set_application("app");
ttk::Http::set_agent("app/1.0");
```

## Building

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

With the defaults, the libcurl headers must be installed.

`TTK_SANITIZE=ON` adds the address and undefined sanitizers. `tools/lint.sh`
runs clang-tidy over the sources with the checks in `.clang-tidy`.

`TTK_TESTS=ON` builds headless rendering correctness checks. Run them with
`ctest --test-dir build --output-on-failure`.
