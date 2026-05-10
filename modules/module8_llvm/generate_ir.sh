#!/bin/sh
# Module 8 — generate Clang reference IR for the bundled C tests.
# Requires `clang` on PATH.
set -e
ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT="$ROOT/out"
mkdir -p "$OUT"
for f in test1.c test2.c; do
    base="${f%.c}"
    echo "=== $f → ${base}-O0.ll (unoptimised) ==="
    clang "$ROOT/$f" -S -emit-llvm -o "$OUT/${base}-O0.ll"
    echo "=== $f → ${base}-O3.ll (optimised) ==="
    clang "$ROOT/$f" -S -emit-llvm -O3 -o "$OUT/${base}-O3.ll"
done
echo "IR written to $OUT"
