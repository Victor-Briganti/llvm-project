; ModuleID = 'omp_unroll.ll'
source_filename = "omp_unroll.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: noinline nounwind sspstrong uwtable
define dso_local i32 @main() #0 {
  br label %1

1:                                                ; preds = %30, %0
  %.01 = phi i32 [ 0, %0 ], [ %31, %30 ]
  %.0 = phi i32 [ 0, %0 ], [ %32, %30 ]
  %2 = icmp ult i32 %.0, 100
  br i1 %2, label %3, label %33

3:                                                ; preds = %1
  %4 = add nuw nsw i32 %.01, 1
  %5 = add nuw nsw i32 %.0, 1
  br label %6

6:                                                ; preds = %3
  %7 = add nuw nsw i32 %4, 1
  %8 = add nuw nsw i32 %5, 1
  br label %9

9:                                                ; preds = %6
  %10 = add nuw nsw i32 %7, 1
  %11 = add nuw nsw i32 %8, 1
  br label %12

12:                                               ; preds = %9
  %13 = add nuw nsw i32 %10, 1
  %14 = add nuw nsw i32 %11, 1
  br label %15

15:                                               ; preds = %12
  %16 = add nuw nsw i32 %13, 1
  %17 = add nuw nsw i32 %14, 1
  br label %18

18:                                               ; preds = %15
  %19 = add nuw nsw i32 %16, 1
  %20 = add nuw nsw i32 %17, 1
  br label %21

21:                                               ; preds = %18
  %22 = add nuw nsw i32 %19, 1
  %23 = add nuw nsw i32 %20, 1
  br label %24

24:                                               ; preds = %21
  %25 = add nuw nsw i32 %22, 1
  %26 = add nuw nsw i32 %23, 1
  br label %27

27:                                               ; preds = %24
  %28 = add nuw nsw i32 %25, 1
  %29 = add nuw nsw i32 %26, 1
  br label %30

30:                                               ; preds = %27
  %31 = add nuw nsw i32 %28, 1
  %32 = add nuw nsw i32 %29, 1
  br label %1, !llvm.loop !6

33:                                               ; preds = %1
  %.01.lcssa = phi i32 [ %.01, %1 ]
  %34 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %.01.lcssa)
  ret i32 0
}

declare i32 @printf(ptr noundef, ...) #1

attributes #0 = { noinline nounwind sspstrong uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 19.1.7"}
!6 = distinct !{!6, !7, !8}
!7 = !{!"llvm.loop.mustprogress"}
!8 = !{!"llvm.loop.unroll.disable"}
