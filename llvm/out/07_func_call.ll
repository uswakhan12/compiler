; ModuleID = 'minicc-output'
target triple = "x86_64-pc-linux-gnu"

@.fmt_int = private constant [4 x i8] c"%d\0A\00"
@.fmt_flt = private constant [4 x i8] c"%f\0A\00"
declare i32 @printf(i8*, ...)
declare double @log(double)

define i32 @main() {
entry:
  %y = alloca double
  store double 0.000000e+00, double* %y
  %x = alloca double
  store double 0.000000e+00, double* %x
  %t0 = alloca double
  %flit.0 = sitofp i32 2 to double
  store double %flit.0, double* %x
  %ld.1 = load double, double* %x
  %ret.2 = call double @log(double %ld.1)
  store double %ret.2, double* %t0
  %ld.3 = load double, double* %t0
  store double %ld.3, double* %y
  %ld.4 = load double, double* %y
  %rcast.5 = fptosi double %ld.4 to i32
  ret i32 %rcast.5
post.6:
  ret i32 0     ; fallback terminator
}
