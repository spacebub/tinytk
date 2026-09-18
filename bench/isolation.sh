#!/bin/bash
#
# Every benchmark reads the same alone as it does inside the whole run, or the
# suite is only good for comparing one whole run against another. Prints the
# drift between the two for a handful that cover the harness.
#
#   bench/isolation.sh /tmp/scratch [--benchmark_min_warmup_time=0.3]
set -u

OUT="${1:?usage: isolation.sh <scratch dir> [extra ttk_bench flags]}"; shift
EXTRA="$*"
BIN=./build-bench/bin/ttk_bench

PICKS="Chip_paint Button_paint/1 Scroll_paint/512 Label_paint_wrapped
       Typeface_draw_cached Painter_paragraph Glyphs_draw/16 Box_arrange_column/64"

mkdir -p "$OUT"

$BIN $EXTRA --benchmark_out="$OUT/whole.json" --benchmark_out_format=json >/dev/null 2>&1

for b in $PICKS; do
    $BIN $EXTRA --benchmark_filter="^${b}$" \
        --benchmark_out="$OUT/alone_$(echo "$b" | tr '/' '_').json" \
        --benchmark_out_format=json >/dev/null 2>&1
done

python3 - "$OUT" $PICKS <<'PY'
import json, sys

out = sys.argv[1]
whole = {b['name']: b['real_time']
         for b in json.load(open(f"{out}/whole.json"))['benchmarks']}

print(f"{'benchmark':28s} {'alone':>12s} {'in suite':>12s} {'drift':>8s}")

worst = 0.0

for name in sys.argv[2:]:
    alone = json.load(open(f"{out}/alone_{name.replace('/', '_')}.json"))['benchmarks']

    if not alone or name not in whole:
        print(f"{name:28s}  (no data)")
        continue

    a = alone[0]['real_time']
    drift = 100.0 * (whole[name] - a) / a
    worst = max(worst, abs(drift))

    print(f"{name:28s} {a:12.1f} {whole[name]:12.1f} {drift:+7.1f}%")

print(f"\nworst drift: {worst:.1f}%")
PY
