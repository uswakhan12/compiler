# Module 8 — LLVM and Code Generation

Two complementary deliverables:

| Component | Purpose |
|-----------|---------|
| `llvm_emit.c` (linked into `minicc`) | A **TAC → LLVM IR backend** written from scratch. Lowers our Three-Address Code to textual LLVM IR (`define i32 @main()`), preserving control flow, type promotions, and array layout. The output is a `.ll` file that Clang compiles to a native executable, completing the full pipeline: **source.mini → tokens → AST → TAC → optimised TAC → LLVM IR → native exe**. |
| `test1.c`, `test2.c`, `generate_ir.sh` | Clang reference programs used to demonstrate the **O0 vs O3** comparison required by §9.3. |

## Build / run our backend
```
make minicc
./minicc --emit-llvm=build/sample.ll samples/cases/01_arith.mini
clang -Wno-override-module build/sample.ll -o build/sample_exe
./build/sample_exe
```

## Clang reference (O0 vs O3)
```
chmod +x generate_ir.sh
./generate_ir.sh           # writes test{1,2}-{O0,O3}.ll into out/
```
Then in the report, annotate at least three transformations visible in `*-O3.ll` (e.g. constant folding, loop-invariant motion, dead-code elimination, scalar replacement, SSA form, register allocation) and explain `alloca`, `load`, `store`, `ret`, `add`, `call`.

## Supported lowerings
* `add / sub / mul / sdiv` for int, `fadd / fsub / fmul / fdiv` for float
* `sitofp` for `int → float` promotion (and `fptosi` for the reverse)
* `icmp <pred>` + `br i1` for conditional jumps
* `getelementptr` + load/store for `a[i]` access
* `alloca` for both scalars and `[N x i32]` arrays

Run `./tools/run_all_modules.sh` to see every test case lowered to `.ll` and compared against Clang's own output.
