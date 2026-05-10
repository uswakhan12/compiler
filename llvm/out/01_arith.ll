; ModuleID = 'minicc-output'
target triple = "x86_64-pc-linux-gnu"

@.fmt_int = private constant [4 x i8] c"%d\0A\00"
@.fmt_flt = private constant [4 x i8] c"%f\0A\00"
declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %b = alloca i32
  store i32 0, i32* %b
  %a = alloca i32
  store i32 0, i32* %a
  %t0 = alloca i32
  %t1 = alloca i32
  %t2 = alloca i32
  %v.0 = mul i32 2, 3
  store i32 %v.0, i32* %t0
  %ld.1 = load i32, i32* %t0
  %v.2 = add i32 1, %ld.1
  store i32 %v.2, i32* %t1
  %ld.3 = load i32, i32* %t1
  store i32 %ld.3, i32* %a
  %ld.4 = load i32, i32* %a
  %v.5 = sub i32 %ld.4, 4
  store i32 %v.5, i32* %t2
  %ld.6 = load i32, i32* %t2
  store i32 %ld.6, i32* %b
  ret i32 0
}
