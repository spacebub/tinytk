# tinytk

Tiny ui toolkit on Blend2D and SDL3. Provides several premade controls and system utilities.

## Layout

`src/ttk/system` is `ttk::system`, a static library with no window in it:
process spawning, HTTP fetches, desktop notifications, JSON over yyjson, text helpers, environment
and paths.

`src/ttk` beyond that is `ttk::ui`, a widget toolkit drawn with Blend2D
inside an SDL3 window. `draw` holds the rasteriser side, `toolkit` the
widgets, `shell` the window and frame loop, `dialogs` and `notices` the
pieces every application puts over its pages.

## Using it

Add the tree and link one of the two targets.

```cmake
add_subdirectory(tinytk)
target_link_libraries(app PRIVATE ttk::ui)
```

`TTK_UI=OFF` skips the interface library and its fetches. `TTK_DOWNLOAD_CACHE`
names where fetched sources are kept between build trees.

`TTK_GALLERY=ON` builds `examples/gallery.cpp`, a window showing every widget,
dialog, menu, tooltip and notice the library has. It is off by default.

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

On Linux and the BSDs, notifications go over D-Bus, so `libdbus-1` and its headers
must be installed.

`SANITIZE=ON` adds the address and undefined sanitizers. `tools/lint.sh`
runs clang-tidy over the sources with the checks in `.clang-tidy`.
