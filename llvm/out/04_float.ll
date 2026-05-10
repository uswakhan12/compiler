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
  %t0 = alloca double
  %t1 = alloca double
  store i32 2, i32* %k
  %ld.0 = load i32, i32* %k
  %p.1 = sitofp i32 %ld.0 to double
  store double %p.1, double* %t0
  %ld.2 = load double, double* %t0
  %v.3 = fmul double 1.5, %ld.2
  store double %v.3, double* %t1
  %ld.4 = load double, double* %t1
  store double %ld.4, double* %y
  ret i32 0
}
