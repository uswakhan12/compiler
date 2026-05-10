; ModuleID = 'minicc-output'
target triple = "x86_64-pc-linux-gnu"

@.fmt_int = private constant [4 x i8] c"%d\0A\00"
@.fmt_flt = private constant [4 x i8] c"%f\0A\00"
declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %s = alloca i32
  store i32 0, i32* %s
  %i = alloca i32
  store i32 0, i32* %i
  %t0 = alloca i32
  %t1 = alloca i32
  %t2 = alloca i32
  %t3 = alloca i32
  %t4 = alloca i32
  %t5 = alloca i32
  store i32 0, i32* %i
  br label %L0
L0:
  %ld.0 = load i32, i32* %i
  %c.1 = icmp slt i32 %ld.0, 5
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
  br i1 %c.5, label %L1, label %cont.6
cont.6:
  %ld.7 = load i32, i32* %i
  %v.8 = add i32 %ld.7, 1
  store i32 %v.8, i32* %t1
  ; (skipping unsupported TAC op: id=10)
  %ld.9 = load i32, i32* %t1
  store i32 %ld.9, i32* %t2
  %ld.10 = load i32, i32* %t2
  store i32 %ld.10, i32* %i
  br label %L0
post.11:
  br label %L1
L1:
  ; (skipping unsupported TAC op: id=15)
  ; (skipping unsupported TAC op: id=16)
  %ld.12 = load i32, i32* %t3
  %ld.13 = load i32, i32* %t4
  %v.14 = add i32 %ld.12, %ld.13
  store i32 %v.14, i32* %t5
  %ld.15 = load i32, i32* %t5
  store i32 %ld.15, i32* %s
  ret i32 0
}
