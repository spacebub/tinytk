# Benchmarks

Google Benchmark suite over `ttk::system` and `ttk::ui`. Built only with
`TTK_BENCHMARKS=ON`.

## Building

```
cmake -S . -B build-bench -G Ninja -DCMAKE_BUILD_TYPE=Release -DTTK_BENCHMARKS=ON
cmake --build build-bench --target ttk_bench
./build-bench/bin/ttk_bench
```

`Release` only. A `Debug` build measures `-Og`.

## Running

```
./build-bench/bin/ttk_bench --benchmark_filter='Button_|Scroll_'
./build-bench/bin/ttk_bench --benchmark_out=before.json --benchmark_out_format=json
```

Comparing two commits, with the tool Google Benchmark ships (needs scipy):

```
python3 .download-cache/benchmark-v1.9.5/tools/compare.py benchmarks before.json after.json
```

Compare whole runs. Every file shares the process wide caches, so a filtered
run is a different measurement from a full one. Pin the clock before trusting a
small difference. `bench/isolation.sh <scratch dir>` reports how far a benchmark
drifts between running alone and in the suite. A new one that drifts more than
the rest is holding state the one before it left.

## Layout

| Directory  | Covers                                                                        |
|------------|-------------------------------------------------------------------------------|
| `support/` | The harness: a windowless canvas, a sandboxed home, deterministic fixtures    |
| `system/`  | `Text` and `Json`                                                             |
| `ui/`      | `Typeface`, `Painter`, the glyph and paint primitives, the controls, the layouts, `ReorderGrid`, `Anim` |

`support/Sandbox` points `HOME` and the XDG variables at a scratch tree before
anything reads them. `support/Canvas` is a window's pixels with no window: an
offscreen `ttk::Surface` driven through the same `ttk::compose` step as
`Shell::draw`, minus SDL. `frame()`
settles, advances and paints the damaged rectangles, `full()` paints the lot,
and `mount()` places one widget alone in the page at its natural size.

## Reading the numbers

The budget for `ui/` is one frame, 1000 ms over the refresh rate. A control's
`_paint` is its share. Sizes are swept where the cost depends on one. A time
that is flat across a sweep means the work is cached or culled.
