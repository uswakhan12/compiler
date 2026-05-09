#!/bin/sh
# One command: print outputs for all lab modules. Test cases = every samples/cases/*.mini
# (falls back to samples/*.mini except bad_*). Redirect if needed:  ./tools/run_all_modules.sh | tee out.txt
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

list_cases() {
	if ls samples/cases/*.mini >/dev/null 2>&1; then
		ls samples/cases/*.mini | sort
	else
		for g in samples/*.mini; do
			[ -f "$g" ] || continue
			case "$g" in *bad_*) continue ;; esac
			echo "$g"
		done | sort
	fi
}

[ -x ./minicc ] || { echo "Run: make" >&2; exit 1; }
[ -x ./postfix ] && [ -x ./prefix ] && [ -x ./infix ] || { echo "Run: make calc" >&2; exit 1; }
[ -x ./extended ] || { echo "Run: make extended" >&2; exit 1; }
[ -x ./ll1 ] || { echo "Run: make ll1" >&2; exit 1; }

TMP=$(mktemp)
trap 'rm -f "$TMP"' EXIT
list_cases >"$TMP"
if [ ! -s "$TMP" ]; then
	echo "No test cases: add samples/cases/*.mini" >&2
	exit 1
fi

echo "Test cases ($(wc -l <"$TMP" | tr -d ' ') files):"
cat "$TMP"
echo ""

echo "======== Module 1 — Lexer (tokens) ========"
while read -r f; do
	echo "--- $f ---"
	./minicc --tokens "$f"
done <"$TMP"

echo ""
echo "======== Module 2 — Calculators ========"
echo "--- postfix ---"
echo "4 8 +" | ./postfix
echo "--- prefix ---"
printf '%s\n' "+ 4 8" | ./prefix
echo "--- infix ---"
echo "4 + 8 * 2" | ./infix

echo ""
echo "======== Module 3 — Extended grammar ========"
./extended <samples/extended_expr.txt

echo ""
echo "======== Module 4 — FIRST / FOLLOW / LL(1) ========"
./ll1

echo ""
echo "======== Module 5 — Semantic (check) ========"
while read -r f; do
	echo "--- $f ---"
	./minicc --check "$f"
done <"$TMP"
echo "--- bad_type.mini ---"
./minicc samples/bad_type.mini 2>&1 || true
echo "--- bad_scope.mini ---"
./minicc samples/bad_scope.mini 2>&1 || true

echo ""
echo "======== Module 6 — IR (TAC, before opt) ========"
while read -r f; do
	echo "--- $f ---"
	./minicc "$f"
done <"$TMP"

echo ""
echo "======== Module 7 — Optimisation ========"
FIRST=$(head -1 "$TMP")
echo "--- full pass listing (first test case: $FIRST) ---"
./minicc --opt "$FIRST"
if [ "$(wc -l <"$TMP" | tr -d ' ')" -gt 1 ]; then
	echo "--- other cases: Final TAC only ---"
	tail -n +2 "$TMP" | while read -r f; do
		echo "--- ./minicc --opt $f ---"
		./minicc --opt "$f" 2>&1 | awk '/^=== Final TAC ===$/ {p=1} p'
	done
fi

echo ""
echo "======== Module 7 Task 6 — Timing (per test case, needs bc) ========"
if command -v bc >/dev/null 2>&1; then
	chmod +x tools/opt_bench.sh 2>/dev/null || true
	while read -r f; do
		echo "--- opt_bench $f ---"
		./tools/opt_bench.sh "$f"
	done <"$TMP"
else
	echo "Install bc for timing table."
fi

echo ""
echo "======== Module 8 — LLVM (clang + emit .ll) ========"
if command -v clang >/dev/null 2>&1; then
	clang --version
	chmod +x llvm/generate_ir.sh 2>/dev/null || true
	./llvm/generate_ir.sh || true
else
	echo "clang not found — install for Module 8."
fi

echo ""
echo "======== Pipeline (first case only; tokens → optimised TAC) ========"
./minicc --pipeline "$FIRST"
