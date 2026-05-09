#!/bin/sh
# Module 7 Task 6 — Performance comparison (5 runs × inner repetitions for measurable times).
# Plain: ./minicc   vs   ./minicc --opt  on the same input file.
# Usage: ./tools/opt_bench.sh [path/to/file.mini]
set -e
SAMPLE="${1:-samples/demo.mini}"
RUNS=5
INNER=80

if ! command -v bc >/dev/null 2>&1; then
  echo "Install 'bc' for averages." >&2
  exit 1
fi

bench_plain_batch() {
  i=1
  while [ "$i" -le "$INNER" ]; do
    ./minicc "$SAMPLE" >/dev/null 2>&1
    i=$((i + 1))
  done
}

bench_opt_batch() {
  i=1
  while [ "$i" -le "$INNER" ]; do
    ./minicc --opt "$SAMPLE" >/dev/null 2>&1
    i=$((i + 1))
  done
}

echo "Sample: $SAMPLE"
echo "Each table row = wall-clock to compile ${INNER} times (then divide by ${INNER} for per-compile avg)."
echo ""

sum_plain=0
sum_opt=0
echo "| Run | Plain — total ${INNER}x (s) | Plain avg per compile (s) | Opt — total ${INNER}x (s) | Opt avg per compile (s) |"
echo "|-----|------------------------------|---------------------------|------------------------------|---------------------------|"

r=1
while [ "$r" -le "$RUNS" ]; do
  tp=$( { time -p bench_plain_batch; } 2>&1 | awk '/^real/ {print $2}')
  to=$( { time -p bench_opt_batch; } 2>&1 | awk '/^real/ {print $2}')
  ap=$(echo "scale=8; $tp / $INNER" | bc)
  ao=$(echo "scale=8; $to / $INNER" | bc)
  printf "|  %d  | %s | %s | %s | %s |\n" "$r" "$tp" "$ap" "$to" "$ao"
  sum_plain=$(echo "$sum_plain + $tp" | bc)
  sum_opt=$(echo "$sum_opt + $to" | bc)
  r=$((r + 1))
done

avg_batch_p=$(echo "scale=8; $sum_plain / $RUNS" | bc)
avg_batch_o=$(echo "scale=8; $sum_opt / $RUNS" | bc)
avg_pc=$(echo "scale=8; $avg_batch_p / $INNER" | bc)
avg_oc=$(echo "scale=8; $avg_batch_o / $INNER" | bc)
sp=$(echo "scale=4; if ($avg_oc > 0) $avg_pc / $avg_oc else 0" | bc)

echo "|-----|------------------------------|---------------------------|------------------------------|---------------------------|"
printf "| Avg | %s | %s | %s | %s |\n" "$avg_batch_p" "$avg_pc" "$avg_batch_o" "$avg_oc"
echo ""
echo "Average real time per single compile (plain): ${avg_pc} s"
echo "Average real time per single compile (--opt):   ${avg_oc} s"
echo "Speedup ratio (plain ÷ opt):                   ${sp}x"
echo ""
echo "Note: For tiny inputs, --opt often does more work; ratios near 1 or below are normal."
