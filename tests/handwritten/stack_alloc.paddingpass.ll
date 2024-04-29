; ModuleID = 'stack_alloc.ll'
source_filename = "stack_alloc.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: mustprogress nofree nosync nounwind willreturn memory(none) uwtable
define dso_local i32 @func() local_unnamed_addr #0 {
  %pad = alloca [24 x i8], align 4
  %1 = alloca [2 x i32], align 4
  %2 = alloca [8 x i32], align 16
  %pad1 = alloca [28 x i8], align 16
  %3 = alloca [9 x i32], align 16
  %4 = alloca [16 x i32], align 16
  %pad2 = alloca [28 x i8], align 16
  %5 = alloca [17 x i32], align 16
  call void @llvm.lifetime.start.p0(i64 8, ptr nonnull %1) #2
  call void @llvm.lifetime.start.p0(i64 32, ptr nonnull %2) #2
  call void @llvm.lifetime.start.p0(i64 36, ptr nonnull %3) #2
  call void @llvm.lifetime.start.p0(i64 64, ptr nonnull %4) #2
  call void @llvm.lifetime.start.p0(i64 68, ptr nonnull %5) #2
  %6 = ptrtoint ptr %1 to i64
  %7 = ptrtoint ptr %2 to i64
  %8 = add nsw i64 %7, %6
  %9 = ptrtoint ptr %3 to i64
  %10 = add nsw i64 %8, %9
  %11 = ptrtoint ptr %4 to i64
  %12 = add nsw i64 %10, %11
  %13 = ptrtoint ptr %5 to i64
  %14 = add nsw i64 %12, %13
  %15 = trunc i64 %14 to i32
  call void @llvm.lifetime.end.p0(i64 68, ptr nonnull %5) #2
  call void @llvm.lifetime.end.p0(i64 64, ptr nonnull %4) #2
  call void @llvm.lifetime.end.p0(i64 36, ptr nonnull %3) #2
  call void @llvm.lifetime.end.p0(i64 32, ptr nonnull %2) #2
  call void @llvm.lifetime.end.p0(i64 8, ptr nonnull %1) #2
  ret i32 %15
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #1

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #1

attributes #0 = { mustprogress nofree nosync nounwind willreturn memory(none) uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+avx,+avx2,+cmov,+crc32,+cx8,+fxsr,+mmx,+popcnt,+sse,+sse2,+sse3,+sse4.1,+sse4.2,+ssse3,+x87,+xsave" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #2 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"clang version 17.0.6"}
