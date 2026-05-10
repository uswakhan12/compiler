; ModuleID = 'minicc-output'
target triple = "x86_64-pc-linux-gnu"

@.fmt_int = private constant [4 x i8] c"%d\0A\00"
@.fmt_flt = private constant [4 x i8] c"%f\0A\00"
declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %k = alloca i32
  store i32 0, i32* %k
  %y = alloca double
  store double 0.000000e+00, double* %y
  %t0 = alloca i32
  %t1 = alloca i32
  store i32 2, i32* %k
  %ld.0 = load i32, i32* %k
  %p.1 = sitofp i32 %ld.0 to double
  %p.2 = fptosi double %p.1 to i32
  store i32 %p.2, i32* %t0
  %ld.3 = load i32, i32* %t0
  %p.4 = sitofp i32 %ld.3 to double
  %v.5 = fmul double 1.5, %p.4
  %p.6 = fptosi double %v.5 to i32
  store i32 %p.6, i32* %t1
  %ld.7 = load i32, i32* %t1
  %p.8 = sitofp i32 %ld.7 to double
  store double %p.8, double* %y
  ret i32 0
}
