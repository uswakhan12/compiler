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
  store i32 6, i32* %t0
  %ld.0 = load i32, i32* %t0
  %v.1 = add i32 1, %ld.0
  store i32 %v.1, i32* %t1
  %ld.2 = load i32, i32* %t1
  store i32 %ld.2, i32* %a
  %ld.3 = load i32, i32* %a
  %v.4 = sub i32 %ld.3, 4
  store i32 %v.4, i32* %t2
  %ld.5 = load i32, i32* %t2
  store i32 %ld.5, i32* %b
  ret i32 0
}
