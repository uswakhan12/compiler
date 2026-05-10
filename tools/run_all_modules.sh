#!/bin/sh
# Full end-to-end demo for every module of the CS-346 mini-compiler.
# Usage:  ./tools/run_all_modules.sh           # everything to terminal
#         ./tools/run_all_modules.sh | tee log.txt
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

# Check every binary the demo needs.
for bin in minicc postfix prefix infix extended ll1 ll1_generic lab05 postfix_eval parse_tree; do
    [ -x "./$bin" ] || { echo "Missing ./$bin — run \`make\`." >&2; exit 1; }
done

TMP=$(mktemp)
trap 'rm -f "$TMP"' EXIT
list_cases >"$TMP"
echo "Test cases ($(wc -l <"$TMP" | tr -d ' ') files):"
cat "$TMP"
echo ""

# ============================================================== #
echo "================ Module 1 — Lexical Analysis ================"
echo "--- Lab 05 baseline lexer (./lab05) ---"
printf 'procedure foo\nif x then begin y = 42 end\nfunction bar\n' | ./lab05 || true

echo ""
echo "--- Lab 05 Task 2: postfix expression evaluator (./postfix_eval) ---"
echo "4 8 + 3 *" | ./postfix_eval

echo ""
echo "--- Full-project lexer token stream for every test case ---"
while read -r f; do
    echo "[$f]"
    ./minicc --tokens "$f"
done <"$TMP"

# ============================================================== #
echo ""
echo "================ Module 2 — Parsing (3 notations) ================"
echo "--- postfix calculator ---"
echo "4 8 + 7 *" | ./postfix
echo "--- prefix calculator ---"
printf '* 3 + 4 5\n' | ./prefix
echo "--- infix calculator ---"
echo "(1 + 2) * (3 + 4) - 5" | ./infix
echo "--- explicit parse tree + syntax tree + pre/in/post traversals ---"
echo "1+2*(3+4)" | ./parse_tree

# ============================================================== #
echo ""
echo "================ Module 3 — Extended grammar (^, log, exp) ================"
./extended <samples/extended_expr.txt
echo ""
echo "--- Debug mode (YYDEBUG enabled via --debug) ---"
echo "log(2.0) + exp(1.0)" | ./extended --debug 2>&1 | tail -20 || true

# ============================================================== #
echo ""
echo "================ Module 4 — First / Follow / LL(1) table ================"
echo "--- Fixed grammar (./ll1) ---"
./ll1
echo ""
echo "--- Generic grammar from file (./ll1_generic grammars/expr.txt) ---"
./ll1_generic modules/module4_first_follow/grammars/expr.txt
echo ""
echo "--- Generic grammar (statements) (./ll1_generic grammars/stmt.txt) ---"
./ll1_generic modules/module4_first_follow/grammars/stmt.txt | head -40

# ============================================================== #
echo ""
echo "================ Module 5 — Semantic analysis ================"
while read -r f; do
    echo "[$f]"
    ./minicc --check "$f"
done <"$TMP"
echo "--- bad_type.mini (expected: type-mismatch error) ---"
./minicc samples/bad_type.mini 2>&1 || true
echo "--- bad_scope.mini (expected: undeclared-variable error) ---"
./minicc samples/bad_scope.mini 2>&1 || true

# ============================================================== #
echo ""
echo "================ Module 6 — IR (TAC, before optimisation) ================"
while read -r f; do
    echo "[$f]"
    ./minicc "$f"
done <"$TMP"

# ============================================================== #
echo ""
echo "================ Module 7 — Optimisation ================"
FIRST=$(head -1 "$TMP")
echo "--- full pass listing for first test case ($FIRST) ---"
./minicc --opt "$FIRST"
echo ""
echo "--- LICM demo on samples/cases/06_licm.mini ---"
./minicc --opt samples/cases/06_licm.mini 2>&1 | sed -n '/Task 5:/,/Technique 5/p' | head -30

if [ "$(wc -l <"$TMP" | tr -d ' ')" -gt 1 ]; then
    echo ""
    echo "--- Final TAC for remaining cases ---"
    tail -n +2 "$TMP" | while read -r f; do
        echo "[$f]"
        ./minicc --opt "$f" 2>&1 | awk '/^=== Final TAC ===$/ {p=1} p'
    done
fi

# ============================================================== #
echo ""
echo "================ Module 7 Task 6 — Performance comparison ================"
if command -v bc >/dev/null 2>&1; then
    chmod +x tools/opt_bench.sh 2>/dev/null || true
    while read -r f; do
        echo "[$f]"
        ./tools/opt_bench.sh "$f"
    done <"$TMP"
else
    echo "Install bc to run the timing table."
fi

# ============================================================== #
echo ""
echo "================ Module 8 — LLVM IR generation ================"
echo "--- minicc TAC → LLVM IR backend (./minicc --emit-llvm) ---"
mkdir -p modules/module8_llvm/out
while read -r f; do
    name=$(basename "$f" .mini)
    out=modules/module8_llvm/out/${name}.ll
    ./minicc --emit-llvm="$out" "$f" >/dev/null 2>&1 || true
    echo "[$f] → $out  ($(wc -l <"$out" 2>/dev/null | tr -d ' ') lines)"
done <"$TMP"
echo "Head of one generated .ll file:"
head -25 modules/module8_llvm/out/$(basename "$FIRST" .mini).ll

if command -v clang >/dev/null 2>&1; then
    echo ""
    echo "--- Clang reference: O0 vs O3 on bundled test1.c / test2.c ---"
    clang --version | head -1
    chmod +x modules/module8_llvm/generate_ir.sh 2>/dev/null || true
    ./modules/module8_llvm/generate_ir.sh || true
    echo "Generated .ll files:"
    ls -1 modules/module8_llvm/out/*.ll
else
    echo "clang not found — install clang for the full Module 8 demo."
fi

# ============================================================== #
echo ""
echo "================ Integrated pipeline (one shot) ================"
./minicc --pipeline "$FIRST" | head -80
