; ModuleID = 'minicc-output'
target triple = "x86_64-pc-linux-gnu"

@.fmt_int = private constant [4 x i8] c"%d\0A\00"
@.fmt_flt = private constant [4 x i8] c"%f\0A\00"
declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %k = alloca i32
  store i32 0, i32* %k
  %s = alloca i32
  store i32 0, i32* %s
  %i = alloca i32
  store i32 0, i32* %i
  %n = alloca i32
  store i32 0, i32* %n
  %t1 = alloca i32
  %t0 = alloca i32
  %t2 = alloca i32
  %t3 = alloca i32
  store i32 10, i32* %n
  store i32 3, i32* %k
  store i32 0, i32* %i
  store i32 0, i32* %s
  %ld.0 = load i32, i32* %k
  %v.1 = mul i32 %ld.0, 4
  store i32 %v.1, i32* %t1
  br label %L0
L0:
  %ld.2 = load i32, i32* %i
  %ld.3 = load i32, i32* %n
  %c.4 = icmp slt i32 %ld.2, %ld.3
  br i1 %c.4, label %L2, label %cont.5
cont.5:
  store i32 0, i32* %t0
  br label %L3
post.6:
  br label %L2
L2:
  store i32 1, i32* %t0
  br label %L3
L3:
  %ld.7 = load i32, i32* %t0
  %c.8 = icmp eq i32 %ld.7, 0
  br i1 %c.8, label %L1, label %cont.9
cont.9:
  %ld.10 = load i32, i32* %s
  %ld.11 = load i32, i32* %t1
  %v.12 = add i32 %ld.10, %ld.11
  store i32 %v.12, i32* %t2
  %ld.13 = load i32, i32* %t2
  store i32 %ld.13, i32* %s
  %ld.14 = load i32, i32* %i
  %v.15 = add i32 %ld.14, 1
  store i32 %v.15, i32* %t3
  %ld.16 = load i32, i32* %t3
  store i32 %ld.16, i32* %i
  br label %L0
post.17:
  br label %L1
L1:
  ret i32 0
}
