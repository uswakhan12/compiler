; ModuleID = 'minicc-output'
target triple = "x86_64-pc-linux-gnu"

@.fmt_int = private constant [4 x i8] c"%d\0A\00"
@.fmt_flt = private constant [4 x i8] c"%f\0A\00"
declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %i = alloca i32
  store i32 0, i32* %i
  %t1 = alloca i32
  store i32 0, i32* %i
  br label %L0
L0:
  %ld.0 = load i32, i32* %i
  %c.1 = icmp slt i32 %ld.0, 3
  br i1 %c.1, label %L2, label %cont.2
cont.2:
  br label %L3
post.3:
  br label %L2
L2:
  br label %L3
L3:
  %c.4 = icmp eq i32 1, 0
  br i1 %c.4, label %L1, label %cont.5
cont.5:
  %ld.6 = load i32, i32* %i
  %v.7 = add i32 %ld.6, 1
  store i32 %v.7, i32* %t1
  %ld.8 = load i32, i32* %t1
  store i32 %ld.8, i32* %i
  br label %L0
post.9:
  br label %L1
L1:
  ret i32 0
}
