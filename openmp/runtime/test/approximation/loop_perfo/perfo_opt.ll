; ModuleID = 'omp_unroll.ll'
source_filename = "omp_unroll.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: noinline nounwind sspstrong uwtable
define dso_local i32 @main() #0 {
  br label %1

1:                                                ; preds = %3, %0
  %.02 = phi i32 [ 0, %0 ], [ %5, %3 ]
  %.01 = phi i32 [ 0, %0 ], [ %4, %3 ]
  %.0 = phi i32 [ 0, %0 ], [ %6, %3 ]
  %2 = icmp ult i32 %.0, 100
  br i1 %2, label %3, label %7

3:                                                ; preds = %1
  %4 = add nuw nsw i32 %.01, 1
  %5 = add nuw nsw i32 %.02, 2
  %6 = add nuw nsw i32 %.0, 1
  br label %1, !llvm.loop !6

7:                                                ; preds = %1
  %.02.lcssa = phi i32 [ %.02, %1 ]
  %.01.lcssa = phi i32 [ %.01, %1 ]
  %8 = call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str, i32 noundef %.01.lcssa) #2
  %9 = call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str, i32 noundef %.02.lcssa) #2
  ret i32 0
}

declare i32 @printf(ptr noundef, ...) #1

attributes #0 = { noinline nounwind sspstrong uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind }

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
!8 = !{!"llvm.loop.perforation.enable"}
