#!/bin/sh
# Module 7 Task 6 — Performance comparison (binary runtime).
#
# Builds the SAME mini program two ways:
#   1. plain : minicc → LLVM IR → clang -O0 → binary  (no minicc optimisations)
#   2. opt   : minicc --opt → LLVM IR → clang -O0 → binary  (LICM + folding + DCE etc. applied)
#
# Then runs each binary 5 times, records wall-clock time, and reports a
# speedup ratio so the report can include a real table.
#
# Usage: ./tools/opt_bench.sh [path/to/file.mini]
# Default sample: samples/cases/06_licm.mini  (loop-heavy, LICM-friendly).
set -e

SAMPLE="${1:-samples/cases/06_licm.mini}"
RUNS=5
TMPDIR="${TMPDIR:-/tmp}"
PLAIN_LL="$TMPDIR/_bench_plain.ll"
OPT_LL="$TMPDIR/_bench_opt.ll"
PLAIN_BIN="$TMPDIR/_bench_plain_bin"
OPT_BIN="$TMPDIR/_bench_opt_bin"

if ! command -v bc >/dev/null 2>&1; then
  echo "Install 'bc' for averages." >&2
  exit 1
fi
if ! command -v clang >/dev/null 2>&1; then
  echo "clang not found — install LLVM/Clang to use this benchmark." >&2
  exit 1
fi
if [ ! -f "$SAMPLE" ]; then
  echo "sample not found: $SAMPLE" >&2
  exit 1
fi
if [ ! -x ./minicc ]; then
  echo "./minicc missing — run 'make' first." >&2
  exit 1
fi

echo "Sample: $SAMPLE"
echo "Building two LLVM IRs..."
./minicc        "$SAMPLE" --emit-llvm="$PLAIN_LL" >/dev/null 2>&1
./minicc --opt  "$SAMPLE" --emit-llvm="$OPT_LL"   >/dev/null 2>&1
clang -w -O0 "$PLAIN_LL" -o "$PLAIN_BIN" 2>/dev/null
clang -w -O0 "$OPT_LL"   -o "$OPT_BIN"   2>/dev/null

echo "Running each binary $RUNS times..."
echo ""
echo "| Run | Plain runtime (s) | --opt runtime (s) |"
echo "|-----|-------------------|-------------------|"

sum_plain=0
sum_opt=0
r=1
while [ "$r" -le "$RUNS" ]; do
  tp=$( { time -p "$PLAIN_BIN"; } 2>&1 | awk '/^real/ {print $2}')
  to=$( { time -p "$OPT_BIN";   } 2>&1 | awk '/^real/ {print $2}')
  printf "|  %d  |       %s         |       %s         |\n" "$r" "$tp" "$to"
  sum_plain=$(echo "$sum_plain + $tp" | bc)
  sum_opt=$(echo "$sum_opt + $to" | bc)
  r=$((r + 1))
done

avg_p=$(echo "scale=6; $sum_plain / $RUNS" | bc)
avg_o=$(echo "scale=6; $sum_opt   / $RUNS" | bc)
sp=$(echo "scale=4; if ($avg_o > 0) $avg_p / $avg_o else 0" | bc)

echo "|-----|-------------------|-------------------|"
printf "| Avg |       %s         |       %s         |\n" "$avg_p" "$avg_o"
echo ""
echo "Average runtime, plain : ${avg_p} s"
echo "Average runtime, --opt : ${avg_o} s"
echo "Speedup (plain ÷ opt)  : ${sp}x"
echo ""
echo "Note: timings are wall-clock; results vary by load. clang is invoked"
echo "with -O0 so the LLVM optimiser does not erase our work. The only"
echo "difference between the two binaries is which TAC passes minicc ran."
