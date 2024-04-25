; ModuleID = 'maskedLoadExample'
source_filename = "maskedLoadExample.ll"

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@array = global [4 x i32] [i32 1, i32 2, i32 3, i32 4]
@mask = global [4 x i1] [i1 true, i1 false, i1 true, i1 false]

; Function to demonstrate masked load
define void @useMaskedLoad(i32* %base, i1* %mask) {
entry:
  %vec_ptr = bitcast i32* %base to <4 x i32>*
  %mask_vec = bitcast i1* %mask to <4 x i1>*
  %true_mask = load <4 x i1>, <4 x i1>* %mask_vec, align 1
  %loaded_values = call <4 x i32> @llvm.masked.load.v4i32(<4 x i32>* %vec_ptr, i32 4, <4 x i1> %true_mask, <4 x i32> undef)
  ret void
}

; Declare the llvm.masked.load intrinsic
declare <4 x i32> @llvm.masked.load.v4i32(<4 x i32>*, i32, <4 x i1>, <4 x i32>) nounwind readnone

; Main function as the entry point
define i32 @main() {
entry:
  %base = getelementptr inbounds [4 x i32], [4 x i32]* @array, i64 0, i64 0
  %mask = getelementptr inbounds [4 x i1], [4 x i1]* @mask, i64 0, i64 0
  call void @useMaskedLoad(i32* %base, i1* %mask)
  ret i32 0
}
