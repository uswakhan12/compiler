; ModuleID = 'minicc-output'
target triple = "x86_64-pc-linux-gnu"

@.fmt_int = private constant [4 x i8] c"%d\0A\00"
@.fmt_flt = private constant [4 x i8] c"%f\0A\00"
declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %h = alloca i32
  store i32 0, i32* %h
  %g = alloca i32
  store i32 0, i32* %g
  %f = alloca i32
  store i32 0, i32* %f
  %e = alloca i32
  store i32 0, i32* %e
  %d = alloca i32
  store i32 0, i32* %d
  %c = alloca i32
  store i32 0, i32* %c
  %b = alloca i32
  store i32 0, i32* %b
  %a = alloca i32
  store i32 0, i32* %a
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
  %t4 = alloca i32
  %t5 = alloca i32
  %t6 = alloca i32
  %t7 = alloca i32
  %t8 = alloca i32
  %t9 = alloca i32
  store i32 200000000, i32* %n
  store i32 2, i32* %a
  store i32 3, i32* %b
  store i32 4, i32* %c
  store i32 5, i32* %d
  store i32 6, i32* %e
  store i32 7, i32* %f
  store i32 8, i32* %g
  store i32 9, i32* %h
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
  %ld.8 = load i32, i32* %a
  %ld.9 = load i32, i32* %b
  %v.10 = mul i32 %ld.8, %ld.9
  store i32 %v.10, i32* %t1
  %ld.11 = load i32, i32* %t1
  %ld.12 = load i32, i32* %c
  %v.13 = mul i32 %ld.11, %ld.12
  store i32 %v.13, i32* %t2
  %ld.14 = load i32, i32* %t2
  %ld.15 = load i32, i32* %d
  %v.16 = mul i32 %ld.14, %ld.15
  store i32 %v.16, i32* %t3
  %ld.17 = load i32, i32* %t3
  %ld.18 = load i32, i32* %e
  %v.19 = mul i32 %ld.17, %ld.18
  store i32 %v.19, i32* %t4
  %ld.20 = load i32, i32* %t4
  %ld.21 = load i32, i32* %f
  %v.22 = mul i32 %ld.20, %ld.21
  store i32 %v.22, i32* %t5
  %ld.23 = load i32, i32* %t5
  %ld.24 = load i32, i32* %g
  %v.25 = mul i32 %ld.23, %ld.24
  store i32 %v.25, i32* %t6
  %ld.26 = load i32, i32* %t6
  %ld.27 = load i32, i32* %h
  %v.28 = mul i32 %ld.26, %ld.27
  store i32 %v.28, i32* %t7
  %ld.29 = load i32, i32* %s
  %ld.30 = load i32, i32* %t7
  %v.31 = add i32 %ld.29, %ld.30
  store i32 %v.31, i32* %t8
  %ld.32 = load i32, i32* %t8
  store i32 %ld.32, i32* %s
  %ld.33 = load i32, i32* %i
  %v.34 = add i32 %ld.33, 1
  store i32 %v.34, i32* %t9
  %ld.35 = load i32, i32* %t9
  store i32 %ld.35, i32* %i
  br label %L0
post.36:
  br label %L1
L1:
  ret i32 0
}
