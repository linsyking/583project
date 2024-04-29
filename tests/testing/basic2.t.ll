; ModuleID = '1.c'
source_filename = "1.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@c = dso_local local_unnamed_addr global [1024 x i32] zeroinitializer, align 16
@__const.main.a = private unnamed_addr constant <{ [9 x i32], [503 x i32] }> <{ [9 x i32] [i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9], [503 x i32] zeroinitializer }>, align 16
@__const.main.b = private unnamed_addr constant <{ [10 x i32], [502 x i32] }> <{ [10 x i32] [i32 3, i32 4, i32 5, i32 6, i32 6, i32 7, i32 8, i32 3, i32 9, i32 1], [502 x i32] zeroinitializer }>, align 16

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #1

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #1

; Function Attrs: nofree nosync nounwind memory(write, argmem: none, inaccessiblemem: none) uwtable
define dso_local i32 @main() local_unnamed_addr #2 {
  %1 = alloca [1024 x i32], align 16
  call void @llvm.lifetime.start.p0(i64 4096, ptr nonnull %1) #5
  call void @llvm.memset.p0.i64(ptr noundef nonnull align 16 dereferenceable(4096) %1, i8 0, i64 4096, i1 false)
  store i32 -1, ptr %1, align 16, !tbaa !5
  br label %2

2:                                                ; preds = %2, %0
  %3 = phi i64 [ 0, %0 ], [ %14, %2 ]
  %4 = getelementptr inbounds i32, ptr %1, i64 %3
  %5 = load <8 x i32>, ptr %4, align 16, !tbaa !5
  %6 = icmp slt <8 x i32> %5, zeroinitializer
  %7 = getelementptr i32, ptr @__const.main.a, i64 %3
  %8 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %7, i32 4, <8 x i1> %6, <8 x i32> poison), !tbaa !5
  %9 = getelementptr i32, ptr @__const.main.b, i64 %3
  %10 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %9, i32 4, <8 x i1> %6, <8 x i32> poison), !tbaa !5
  %11 = add nsw <8 x i32> %10, %8
  %12 = select <8 x i1> %6, <8 x i32> %11, <8 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %13 = getelementptr inbounds [1024 x i32], ptr @c, i64 0, i64 %3
  store <8 x i32> %12, ptr %13, align 16
  %14 = add nuw i64 %3, 8
  %15 = icmp eq i64 %14, 1024
  br i1 %15, label %16, label %2, !llvm.loop !15

16:                                               ; preds = %2
  call void @llvm.lifetime.end.p0(i64 4096, ptr nonnull %1) #5
  ret i32 0
}

; Function Attrs: mustprogress nocallback nofree nounwind willreturn memory(argmem: write)
declare void @llvm.memset.p0.i64(ptr nocapture writeonly, i8, i64, i1 immarg) #3

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: read)
declare <8 x i32> @llvm.masked.load.v8i32.p0(ptr nocapture, i32 immarg, <8 x i1>, <8 x i32>) #4

attributes #0 = { nofree norecurse nosync nounwind memory(write, argmem: read, inaccessiblemem: none) uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+avx,+avx2,+cmov,+crc32,+cx8,+fxsr,+mmx,+popcnt,+sse,+sse2,+sse3,+sse4.1,+sse4.2,+ssse3,+x87,+xsave" "tune-cpu"="generic" }
attributes #1 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #2 = { nofree nosync nounwind memory(write, argmem: none, inaccessiblemem: none) uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+avx,+avx2,+cmov,+crc32,+cx8,+fxsr,+mmx,+popcnt,+sse,+sse2,+sse3,+sse4.1,+sse4.2,+ssse3,+x87,+xsave" "tune-cpu"="generic" }
attributes #3 = { mustprogress nocallback nofree nounwind willreturn memory(argmem: write) }
attributes #4 = { nocallback nofree nosync nounwind willreturn memory(argmem: read) }
attributes #5 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"clang version 17.0.6"}
!5 = !{!6, !6, i64 0}
!6 = !{!"int", !7, i64 0}
!7 = !{!"omnipotent char", !8, i64 0}
!8 = !{!"Simple C/C++ TBAA"}
!9 = distinct !{!9, !10, !11, !12, !13}
!10 = !{!"llvm.loop.mustprogress"}
!11 = !{!"llvm.loop.unroll.disable"}
!12 = !{!"llvm.loop.isvectorized", i32 1}
!13 = !{!"llvm.loop.unroll.runtime.disable"}
!14 = distinct !{!14, !10, !11, !12}
!15 = distinct !{!15, !10, !11, !12, !13}
