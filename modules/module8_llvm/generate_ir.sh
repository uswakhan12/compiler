#!/bin/sh
# Module 8 — generate .ll files for the bundled C tests (requires clang).
set -e
ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT="$ROOT/out"
mkdir -p "$OUT"
for f in test1.c test2.c; do
  base="${f%.c}"
  echo "=== $f (unoptimised IR) ==="
  clang "$ROOT/$f" -S -emit-llvm -o "$OUT/${base}-O0.ll"
  echo "=== $f (-O3 IR) ==="
  clang "$ROOT/$f" -S -emit-llvm -O3 -o "$OUT/${base}-O3.ll"
done
echo "IR written to $OUT"
