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
  %t0 = alloca i32
  %t1 = alloca i32
  %t2 = alloca i32
  %t3 = alloca i32
  store i32 200000000, i32* %n
  store i32 3, i32* %k
  store i32 0, i32* %i
  store i32 0, i32* %s
  br label %L0
L0:
  %ld.0 = load i32, i32* %i
  %ld.1 = load i32, i32* %n
  %c.2 = icmp slt i32 %ld.0, %ld.1
  br i1 %c.2, label %L2, label %cont.3
cont.3:
  store i32 0, i32* %t0
  br label %L3
post.4:
  br label %L2
L2:
  store i32 1, i32* %t0
  br label %L3
L3:
  %ld.5 = load i32, i32* %t0
  %c.6 = icmp eq i32 %ld.5, 0
  br i1 %c.6, label %L1, label %cont.7
cont.7:
  %ld.8 = load i32, i32* %k
  %v.9 = mul i32 %ld.8, 4
  store i32 %v.9, i32* %t1
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
