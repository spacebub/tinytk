# Changelog

## 1.7.3 - 2026-10-21

### Added

- A benchmark of a wrapped label's natural width at one, two and eight sentences

### Fixed

- A wrapped label measured every word of its text again each time it was asked for its natural
  width. It now keeps the width until its text or face changes

## 1.7.2 - 2026-10-20

### Fixed

- `Shell` lets the screensaver run and the machine sleep, which SDL blocks for as long as video is
  up unless told otherwise

## 1.7.1 - 2026-10-17

### Added

- `Widget::wheel_sideways` takes the sideways part of a wheel event, positive to the right, as a
  touchpad or a tilting wheel sends it. `Root::wheel(steps, across, at)` offers each part that is
  not zero to the widget under the pointer and then its ancestors, so a touchpad scrolls both ways
  at once
- `TextEdit` scrolls sideways by 3 columns per notch of a sideways wheel, and leaves it to its
  parent when there is nothing to the side

### Fixed

- `Shell` passes the wheel's sideways part on instead of dropping it, and fills in the Ctrl and
  Shift held, so Shift and the wheel scroll a `TextEdit` sideways in a window as well as in tests

## 1.7.0 - 2026-10-17

### Changed

- A dialog shown over another no longer hides it from view. The covered dialog is drawn under the
  one on top, and still takes no pointer, keys or focus
- A dialog closed with `Dialog::set_open(false)` before another covered it stays closed when the
  one on top is taken down

### Added

- Benchmarks of the dialog layer: a full paint, a relayout and a small frame with one to three
  dialogs stacked, and showing one dialog over another

## 1.6.0 - 2026-10-16

### Added

- `TextEdit::Decorator`, attached with `TextEdit::decorate`, colours runs of a line, washes a
  line, marks it in the gutter or draws it instead of its text. It is asked only about lines
  being repainted, and an editor without one does no extra work
- `TextEdit::selected`, `caret`, `on_select`, `select`, `go_to` and `insert`, and the line
  lookups `line_count`, `line_of`, `line_start` and `line_end` made public
- `Spinner`, a turning arc for work under way, with `Spinner::render` to paint one inside
  another widget
- `Painter::row` and `Typeface::draw_once` take bands that colour stretches of a line from one
  composed mask
- Benchmarks of a decorated TextEdit page and of typing into 50 000 lines
- `MultistateSwitch::fill` spreads the choices across the whole width of the box
- `TextEdit::spot` and `text_box` give where a character and the text are on screen, and
  `TextEdit::on_scroll` tells when the view scrolls, for something laid over the text to follow it
- `Root::set_modified_activation`, off, leaves Return and Space held with Ctrl or Alt to the
  window's shortcuts instead of clicking the focused control, which they still do by default
- `TextEdit::bare` draws the editor on the window's background, without its own body, border
  and gutter shading

### Fixed

- A label elided to its rows measured its text as one run but folded it word by word, so at
  exactly its natural width it could break its own text and elide it
- A widget that repainted while the root was being destroyed wrote into the root's freed damage
  lists. The root now detaches its tree first, so such a widget finds no root

## 1.5.4 - 2026-10-15

### Fixed

- `Root` attaches its page and layers on construction, so a widget added before the first
  `settle()` has a root from the start. A `wake()` or `now()` from a sync that runs before the
  first frame is no longer lost, and a tween started there runs

## 1.5.3 - 2026-10-14

### Added

- `Label::elide(rows)` caps a wrapped label at a row count, cutting the last line shown with an
  ellipsis. A positive count turns `wrap()` on and zero lifts the cap

## 1.5.2 - 2026-10-13

### Added

- `WlShm::open` takes a `wl_display` and `wl_surface` obtained elsewhere
- Tests of the `wl_shm` ring against a compositor of the suite's own, made of libwayland-server

### Fixed

- A `wl_shm` buffer taken back while the compositor holds it goes under a new `wl_buffer`, so the
  compositor's late release of the old one no longer frees it while it is attached again
- The `wl_shm` stride is computed in 64 bits, so a width past 2^29 pixels is refused instead of
  overflowing
- `WlShm::acquire` keeps `Target::fresh` until the held buffer is presented
- `Damage::shift` adds nothing for a move of at least the region height, and overflows on none

## 1.5.1 - 2026-10-13

### Changed

- The fetched SDL is patched so its Wayland driver no longer damages the whole surface after
  every frame callback. The compositor now receives only the rectangles the toolkit painted,
  which cuts its upload and GPU work while the toolkit's own paint work stays the same

## 1.5.0 - 2026-10-13

### Added

- A GoogleTest suite under `tests/`, one program per area, every test registered with ctest
  so `ctest -j` runs them in parallel. Fetched only with `TTK_TESTS=ON`
- Benchmarks for damage tracking, clip culling and the caret blink
- `Typeface::bleed` and a `Widget::invalidate(region, clip)` overload

### Changed

- Presentation happens on the CPU alone and SDL is built without its render API
- Damage is tracked as bounded pixel sets and only the strips that changed are repainted.
  A frame with many scattered regions folds them into one rectangle
- Culled fills, hit tests and the caret blink cost nothing on the paint path, and Release
  builds align functions to cache lines
- `Json::Doc` takes its text as a `std::unique_ptr<char[]>`
- The clang-format configuration is shared with qzdl and applied, with the hand laid tables
  guarded

### Fixed

- `Json::read_file` no longer reads past its buffer and refuses a directory
- `fit_path` elision, `natural_less` on unsigned numbers and `bytes` rolling over to the next unit
- `Typeface::at` before a font is loaded
- Glyph masks keep their full ink instead of clipping the outer pixels
- Scroll's columns off the grid edge
- `Code::Delete` matches SDL's key code
- A Painter with an empty clip, and Root regions damaged before its first size
- Toasts stop ticking once nothing is showing
- FilePickerDialog answers `said()` in every mode and the picker dismisses after a push
- Button and Pill labels elide instead of overflowing
- StatusIndicator restarts its blink and times it against the last beat
- TextEdit word movement to the right and rewrapping after `wrapped(false)`
- The caret blink checks in TextBox and TextEdit
- A use after free in `Dialog::release`

## 1.4.1 - 2026-10-13

### Changed

- The caret stops blinking in an inactive window and the loop idles while the window is unseen
- Neighbouring marked rows in the file picker keep a gap between them

## 1.4.0 - 2026-10-12

### Changed

- The loop blocks until something happens. `Shell::nudge` wakes it for touches made during
  a frame
- Nothing is allocated per frame in the paint path or per sync in the setters

### Fixed

- 23 clang-tidy findings

## 1.3.2 - 2026-10-11

### Fixed

- The file picker list repaints when a single pick moves to another file

## 1.3.1 - 2026-10-10

### Changed

- SDL is built without the drivers, X11 extensions and services the toolkit never uses

## 1.3.0 - 2026-10-10

### Added

- Automatic tab order across the window. Tab and Shift+Tab move focus, and a popup closes when
  focus leaves it

## 1.2.3 - 2026-10-10

### Changed

- SDL 3.2.24 to 3.4.18

## 1.2.2 - 2026-10-10

### Added

- A globe glyph

## 1.2.1 - 2026-10-03

### Added

- `Button::Kind::Success` with the `successHover` and `successText` palette slots

## 1.2.0 - 2026-10-03

### Added

- Programs can define their own Button kinds, StatusIndicator tones and palette-derived Tones
- A clang-format configuration with a 120 column limit

## 1.1.4 - 2026-10-02

### Changed

- The public headers are documented

### Fixed

- `TextView::drop_rows` on a wrapped run, Card painting over its content and Maximize leaking
  its stroke width

### Removed

- Unused code in FilePicker

## 1.1.3 - 2026-10-01

### Changed

- An optimisation level or an LTO flag given through `CMAKE_CXX_FLAGS` takes precedence over the
  toolkit's own

## 1.1.2 - 2026-09-26

### Changed

- `wl_shm` presentation is offered on Linux only

## 1.1.1 - 2026-09-26

### Changed

- Release builds at O2. The shadow blur is vectorised with AVX2 tiers, outlines are filled as
  rings, the run caches drop their string keys, and the text editor's line indexing and
  wrapping is vectorised

## 1.1.0 - 2026-09-26

### Added

- Presentation through `wl_shm` buffers of the toolkit's own on Wayland, on by default

### Fixed

- A link time error in `ttk::Json`
- Garbled output from processes run in pty mode, which now get a minimum column count

## 1.0.0 - 2026-09-25

### Fixed

- The Windows build

## 0.1.8 - 2026-09-25

### Added

- TextEdit, a multi-line editor
- A TextView benchmark

### Changed

- TextView is far faster

## 0.1.7 - 2026-09-24

### Removed

- The git revision CMake helper

## 0.1.6 - 2026-09-24

### Changed

- The sanitizer option is renamed `TTK_SANITIZE` so it does not clash with consumers

## 0.1.5 - 2026-09-24

### Changed

- Notifications talk to D-Bus directly, so libdbus is no longer a hard dependency
- The build is split into parts that can be switched off one by one

## 0.1.4 - 2026-09-24

### Added

- Desktop notifications

## 0.1.3 - 2026-09-24

### Fixed

- The Windows build

## 0.1.2 - 2026-09-24

### Fixed

- Performance regressions in the theme

## 0.1.1 - 2026-09-24

### Changed

- The theme is reworked into palette slots with a revision that restyles widgets

## 0.1.0 - 2026-09-19

### Added

- First release
