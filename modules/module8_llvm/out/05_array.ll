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
  %a = alloca [5 x i32]
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
  %ld.9 = load i32, i32* %i
  %ld.10 = load i32, i32* %t1
  %gep.11 = getelementptr [5 x i32], [5 x i32]* %a, i32 0, i32 %ld.9
  store i32 %ld.10, i32* %gep.11
  %ld.12 = load i32, i32* %t1
  store i32 %ld.12, i32* %t2
  %ld.13 = load i32, i32* %t2
  store i32 %ld.13, i32* %i
  br label %L0
post.14:
  br label %L1
L1:
  %gep.15 = getelementptr [5 x i32], [5 x i32]* %a, i32 0, i32 0
  %ald.16 = load i32, i32* %gep.15
  store i32 %ald.16, i32* %t3
  %gep.17 = getelementptr [5 x i32], [5 x i32]* %a, i32 0, i32 4
  %ald.18 = load i32, i32* %gep.17
  store i32 %ald.18, i32* %t4
  %ld.19 = load i32, i32* %t3
  %ld.20 = load i32, i32* %t4
  %v.21 = add i32 %ld.19, %ld.20
  store i32 %v.21, i32* %t5
  %ld.22 = load i32, i32* %t5
  store i32 %ld.22, i32* %s
  ret i32 0
}
