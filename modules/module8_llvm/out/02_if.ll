; ModuleID = 'minicc-output'
target triple = "x86_64-pc-linux-gnu"

@.fmt_int = private constant [4 x i8] c"%d\0A\00"
@.fmt_flt = private constant [4 x i8] c"%f\0A\00"
declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %x = alloca i32
  store i32 0, i32* %x
  %t0 = alloca i32
  %t1 = alloca i32
  store i32 3, i32* %x
  %ld.0 = load i32, i32* %x
  %c.1 = icmp sgt i32 %ld.0, 2
  br i1 %c.1, label %L2, label %cont.2
cont.2:
  store i32 0, i32* %t0
  br label %L3
post.3:
  br label %L2
L2:
  store i32 1, i32* %t0
  br label %L3
L3:
  %ld.4 = load i32, i32* %t0
  %c.5 = icmp eq i32 %ld.4, 0
  br i1 %c.5, label %L0, label %cont.6
cont.6:
  %ld.7 = load i32, i32* %x
  %v.8 = add i32 %ld.7, 10
  store i32 %v.8, i32* %t1
  %ld.9 = load i32, i32* %t1
  store i32 %ld.9, i32* %x
  br label %L1
post.10:
  br label %L0
L0:
  store i32 0, i32* %x
  br label %L1
L1:
  ret i32 0
}
