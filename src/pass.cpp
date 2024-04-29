#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Analysis/DomTreeUpdater.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Transforms/Scalar.h"
#include <cassert>
#include <optional>
#include <iostream>
#include <vector>
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Use.h"
#include "llvm/IR/User.h"

using namespace llvm;

static bool runImpl(Function &F, const TargetTransformInfo &TTI, DominatorTree *DT);

namespace {
struct VECPass : public PassInfoMixin<VECPass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
        llvm::LoopAnalysis::Result &li = FAM.getResult<LoopAnalysis>(F);
        IRBuilder<>                 builder(F.getContext());
        std::vector<CallInst *>     MaskedLoadInstructions;
        std::vector<CallInst *>     MaskedStoreInstructions;

        for (auto &B : F) {
            for (auto &I : B) {
                if (auto *CI = dyn_cast<CallInst>(&I)) {
                    auto *maskedFunction = CI->getCalledFunction();
                    if (maskedFunction &&
                        maskedFunction->getName().starts_with("llvm.masked.load")) {
                        MaskedLoadInstructions.push_back(CI);
                    }
                    if (maskedFunction &&
                        maskedFunction->getName().starts_with("llvm.masked.store")) {
                        MaskedStoreInstructions.push_back(CI);
                    }
                }
            }
        }
        auto DL = DataLayout(F.getParent());

        for (auto *CI : MaskedLoadInstructions) {
            outs() << "Optimizing 1 masked load\n";
            BasicBlock *BB         = CI->getParent();
            Type       *VectorType = CI->getCalledFunction()->getReturnType();
            builder.SetInsertPoint(CI);
            // Step 2.1: Cast mask into integer

            auto *mask    = CI->getArgOperand(2);
            auto *maskTy  = mask->getType();
            auto  numBits = DL.getTypeSizeInBits(maskTy);

            // I think we should use getTypeSizeInBits instead of getTypeAllocSizeInBits
            // outs() << "Bits: " << DL.getTypeAllocSizeInBits(maskTy) << '\n';
            // outs() << "Bits: " << DL.getTypeSizeInBits(maskTy) << '\n';

            auto *intTy     = IntegerType::get(maskTy->getContext(), numBits);
            auto *castInstr = builder.CreateBitCast(mask, intTy);
            // Step 2.2: Guard the load with branch

            auto *cmpInst =
                builder.CreateICmpEQ(castInstr, ConstantInt::get(castInstr->getType(), 0));
            auto *branchIns = llvm::SplitBlockAndInsertIfElse(cmpInst, CI, false);
            builder.SetInsertPoint(branchIns);
            auto *unmaskedLoad = builder.CreateAlignedLoad(
                VectorType, CI->getArgOperand(0),
                MaybeAlign(cast<ConstantInt>(CI->getArgOperand(1))->getAlignValue()));
            // Get alignment from the original masked load
            // Problem: This is not accurate, we should get the alignment from the original
            // unmaskedLoad->setAlignment(CI->getArgOperand(0)->getPointerAlignment(DL));

            builder.SetInsertPoint(CI);
            // Use phi node to merge the results
            auto *phi = builder.CreatePHI(VectorType, 2);
            phi->addIncoming(unmaskedLoad, unmaskedLoad->getParent());
            // Undef value
            phi->addIncoming(UndefValue::get(VectorType), BB);
            // Create select instruction
            auto *selectInst = builder.CreateSelect(mask, phi, UndefValue::get(VectorType));
            CI->replaceAllUsesWith(selectInst);
            CI->eraseFromParent();
        }

        for (auto *CI : MaskedStoreInstructions) {
            outs() << "Optimizing 1 masked store\n";
            BasicBlock *BB         = CI->getParent();
            auto       *storeFrom  = CI->getArgOperand(0);
            auto       *storeTo    = CI->getArgOperand(1);
            Type       *VectorType = storeFrom->getType();
            builder.SetInsertPoint(CI);
            auto *mask    = CI->getArgOperand(3);
            auto *maskTy  = mask->getType();
            auto  numBits = DL.getTypeSizeInBits(maskTy);

            auto *storeToLoad = builder.CreateAlignedLoad(
                VectorType, storeTo,
                MaybeAlign(cast<ConstantInt>(CI->getArgOperand(2))->getAlignValue()));
            // Use phi node to merge the results
            auto *selectInst = builder.CreateSelect(mask, storeFrom, storeToLoad);
            builder.CreateAlignedStore(selectInst, storeTo,
                                       MaybeAlign(cast<ConstantInt>(CI->getArgOperand(2))->getAlignValue()));
            CI->eraseFromParent();
        }
        return PreservedAnalyses::none();
    }
};

// In order to prevent exceptions, we avoid out-of-bound memory access using padding. Each memory
// allocation is padded to the next multiple of the an allocation is padded up to the next 32 bytes.

// Hardcoded hardware's vectorization factor, in the number of bits
const int vecFact = 256;
struct PaddingPass : public PassInfoMixin<PaddingPass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
        std::cout << "running...\n" << std::endl;
        // static array
        // stack array
        for (auto &B : F) {
            for (auto &I : B) {
                if (auto *AI = dyn_cast<AllocaInst>(&I)) {
                    std::cout << "found alloca" << std::endl;

                    // auto type = AI->getAllocatedType();
                    // std::string typeStr;
                    // llvm::raw_string_ostream rso(typeStr);
                    // type->print(rso);
                    // std::cout << rso.str() << std::endl;

                    // if (isa<ArrayType>(AI->getAllocatedType())){
                    //     std::cout << "alloca array" << std::endl;
                    //     Value *arraySize = AI->getArraySize();

                    //     // Convert Value* to ConstantInt* to access the actual value
                    //     if (ConstantInt *arraySizeCI = dyn_cast<ConstantInt>(arraySize)) {
                    //         uint64_t arraySize = arraySizeCI->getZExtValue(); // Gets the size as
                    //         an integer

                    //         // Now you have the size as an integer and can use it as needed.
                    //         std::cout << "The arraySize is: " << arraySize << std::endl;
                    //     }

                    //     Type *type = AI->getAllocatedType();
                    // }

                    if (ArrayType *arrayType = dyn_cast<ArrayType>(AI->getAllocatedType())) {
                        // Now we can get the number of elements in the array.
                        uint64_t numElements = arrayType->getNumElements();
                        std::cout << "alloca array of " << numElements << " elements " << std::endl;

                        Type      *elementType = arrayType->getElementType();
                        DataLayout DL          = F.getParent()->getDataLayout();
                        uint64_t   elementSize = DL.getTypeAllocSize(elementType) * 8;
                        std::cout << "element size " << elementSize << " bits " << std::endl;

                        uint64_t arraySizeInBits = elementSize * numElements;
                        std::cout << "array size " << arraySizeInBits << " bits " << std::endl;

                        if (arraySizeInBits % vecFact != 0) {
                            uint64_t paddingSize = vecFact - (arraySizeInBits % vecFact);
                            std::cout << "padding of size " << paddingSize << " bits needed"
                                      << std::endl;

                            auto &context = AI->getContext();
                            auto  paddingType =
                                ArrayType::get(Type::getInt8Ty(context), paddingSize / 8);

                            paddingType->print(llvm::errs());
                            llvm::errs() << "\n";

                            // 创建IRBuilder，它可以帮助我们构建新的指令
                            IRBuilder<> builder(context);

                            // 将插入点设置为现有alloca指令的位置
                            builder.SetInsertPoint(AI->getParent(), AI->getIterator());

                            // 现在创建一个新的alloca指令
                            AllocaInst *newAlloca =
                                builder.CreateAlloca(paddingType, nullptr, "pad");

                            // 设置新alloca的对齐
                            newAlloca->setAlignment(AI->getAlign());
                            // if (paddingSize % elementSize == 0)
                            // {
                            //     // calculate Value *ArraySize
                            //     uint64_t newArrayLength = numElements + (paddingSize /
                            //     elementSize); std::cout << "extend array to " << newArrayLength
                            //     << " elements " << std::endl;

                            //     Type *int64Type = Type::getInt64Ty(F.getContext());
                            //     Value *newArrayLengthVal =
                            //         ConstantInt::get(int64Type, newArrayLength, false);
                            //     newArrayLengthVal->print(llvm::errs()); //
                            //     将其打印到stderr，也可以选择其他输出流 llvm::errs() << "\n";
                            //     // build new alloc
                            //     IRBuilder<> builder(AI);
                            //     // AllocaInst *newAI = builder.CreateAlloca(elementType, nullptr,
                            //     arraysize,
                            //     // "");
                            // }
                        } else
                            std::cout << "padding not needed" << std::endl;
                    } else {
                        // This alloca is for a single element, not an array.
                        if (AI->isArrayAllocation()) {
                            std::cout << "alloca array of variable elements " << std::endl;
                            llvm::Value *sizeValue = AI->getArraySize();  // 获取数组大小参数
                            // 输出该值的详细信息，或者进一步分析
                            if (sizeValue) {
                                sizeValue->print(
                                    llvm::errs());  // 将其打印到stderr，也可以选择其他输出流
                                llvm::errs() << "\n";
                            }
                        }
                    }
                    std::cout << std::endl;
                }
            }
        }

        // getArraySize () const
        // Get the number of elements allocated.

        // heap array
        return PreservedAnalyses::none();
    };
};

struct ScalarizePass : public PassInfoMixin<ScalarizePass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
        auto &TTI = AM.getResult<TargetIRAnalysis>(F);
        auto *DT  = AM.getCachedResult<DominatorTreeAnalysis>(F);
        if (!runImpl(F, TTI, DT))
            return PreservedAnalyses::all();
        PreservedAnalyses PA;
        PA.preserve<TargetIRAnalysis>();
        PA.preserve<DominatorTreeAnalysis>();
        return PA;
    }
};
}  // namespace

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "VECPass", "v0.1", [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback([](StringRef Name, FunctionPassManager &FPM,
                                                      ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "vecpass") {
                        FPM.addPass(VECPass());
                        return true;
                    }
                    if (Name == "paddingpass") {
                        FPM.addPass(PaddingPass());
                        return true;
                    }
                    if (Name == "scalarize") {
                        FPM.addPass(ScalarizePass());
                        return true;
                    }
                    return false;
                });
            }};
}

static bool isConstantIntVector(Value *Mask) {
    Constant *C = dyn_cast<Constant>(Mask);
    if (!C)
        return false;

    unsigned NumElts = cast<FixedVectorType>(Mask->getType())->getNumElements();
    for (unsigned i = 0; i != NumElts; ++i) {
        Constant *CElt = C->getAggregateElement(i);
        if (!CElt || !isa<ConstantInt>(CElt))
            return false;
    }

    return true;
}

static unsigned adjustForEndian(const DataLayout &DL, unsigned VectorWidth, unsigned Idx) {
    return DL.isBigEndian() ? VectorWidth - 1 - Idx : Idx;
}

// Translate a masked load intrinsic like
// <16 x i32 > @llvm.masked.load( <16 x i32>* %addr, i32 align,
//                               <16 x i1> %mask, <16 x i32> %passthru)
// to a chain of basic blocks, with loading element one-by-one if
// the appropriate mask bit is set
//
//  %1 = bitcast i8* %addr to i32*
//  %2 = extractelement <16 x i1> %mask, i32 0
//  br i1 %2, label %cond.load, label %else
//
// cond.load:                                        ; preds = %0
//  %3 = getelementptr i32* %1, i32 0
//  %4 = load i32* %3
//  %5 = insertelement <16 x i32> %passthru, i32 %4, i32 0
//  br label %else
//
// else:                                             ; preds = %0, %cond.load
//  %res.phi.else = phi <16 x i32> [ %5, %cond.load ], [ poison, %0 ]
//  %6 = extractelement <16 x i1> %mask, i32 1
//  br i1 %6, label %cond.load1, label %else2
//
// cond.load1:                                       ; preds = %else
//  %7 = getelementptr i32* %1, i32 1
//  %8 = load i32* %7
//  %9 = insertelement <16 x i32> %res.phi.else, i32 %8, i32 1
//  br label %else2
//
// else2:                                          ; preds = %else, %cond.load1
//  %res.phi.else3 = phi <16 x i32> [ %9, %cond.load1 ], [ %res.phi.else, %else ]
//  %10 = extractelement <16 x i1> %mask, i32 2
//  br i1 %10, label %cond.load4, label %else5
//
static void scalarizeMaskedLoad(const DataLayout &DL, CallInst *CI, DomTreeUpdater *DTU,
                                bool &ModifiedDT) {
    Value *Ptr       = CI->getArgOperand(0);
    Value *Alignment = CI->getArgOperand(1);
    Value *Mask      = CI->getArgOperand(2);
    Value *Src0      = CI->getArgOperand(3);

    const Align AlignVal = cast<ConstantInt>(Alignment)->getAlignValue();
    VectorType *VecType  = cast<FixedVectorType>(CI->getType());

    Type *EltTy = VecType->getElementType();

    IRBuilder<>  Builder(CI->getContext());
    Instruction *InsertPt = CI;
    BasicBlock  *IfBlock  = CI->getParent();

    Builder.SetInsertPoint(InsertPt);
    Builder.SetCurrentDebugLocation(CI->getDebugLoc());

    // Short-cut if the mask is all-true.
    if (isa<Constant>(Mask) && cast<Constant>(Mask)->isAllOnesValue()) {
        Value *NewI = Builder.CreateAlignedLoad(VecType, Ptr, AlignVal);
        CI->replaceAllUsesWith(NewI);
        CI->eraseFromParent();
        return;
    }

    // Adjust alignment for the scalar instruction.
    const Align AdjustedAlignVal = commonAlignment(AlignVal, EltTy->getPrimitiveSizeInBits() / 8);
    unsigned    VectorWidth      = cast<FixedVectorType>(VecType)->getNumElements();

    // The result vector
    Value *VResult = Src0;

    if (isConstantIntVector(Mask)) {
        for (unsigned Idx = 0; Idx < VectorWidth; ++Idx) {
            if (cast<Constant>(Mask)->getAggregateElement(Idx)->isNullValue())
                continue;
            Value    *Gep  = Builder.CreateConstInBoundsGEP1_32(EltTy, Ptr, Idx);
            LoadInst *Load = Builder.CreateAlignedLoad(EltTy, Gep, AdjustedAlignVal);
            VResult        = Builder.CreateInsertElement(VResult, Load, Idx);
        }
        CI->replaceAllUsesWith(VResult);
        CI->eraseFromParent();
        return;
    }

    // If the mask is not v1i1, use scalar bit test operations. This generates
    // better results on X86 at least.
    Value *SclrMask;
    if (VectorWidth != 1) {
        Type *SclrMaskTy = Builder.getIntNTy(VectorWidth);
        SclrMask         = Builder.CreateBitCast(Mask, SclrMaskTy, "scalar_mask");
    }

    for (unsigned Idx = 0; Idx < VectorWidth; ++Idx) {
        // Fill the "else" block, created in the previous iteration
        //
        //  %res.phi.else3 = phi <16 x i32> [ %11, %cond.load1 ], [ %res.phi.else, %else ]
        //  %mask_1 = and i16 %scalar_mask, i32 1 << Idx
        //  %cond = icmp ne i16 %mask_1, 0
        //  br i1 %mask_1, label %cond.load, label %else
        //
        Value *Predicate;
        if (VectorWidth != 1) {
            Value *Mask = Builder.getInt(
                APInt::getOneBitSet(VectorWidth, adjustForEndian(DL, VectorWidth, Idx)));
            Predicate = Builder.CreateICmpNE(Builder.CreateAnd(SclrMask, Mask),
                                             Builder.getIntN(VectorWidth, 0));
        } else {
            Predicate = Builder.CreateExtractElement(Mask, Idx);
        }

        // Create "cond" block
        //
        //  %EltAddr = getelementptr i32* %1, i32 0
        //  %Elt = load i32* %EltAddr
        //  VResult = insertelement <16 x i32> VResult, i32 %Elt, i32 Idx
        //
        Instruction *ThenTerm =
            SplitBlockAndInsertIfThen(Predicate, InsertPt, /*Unreachable=*/false,
                                      /*BranchWeights=*/nullptr, DTU);

        BasicBlock *CondBlock = ThenTerm->getParent();
        CondBlock->setName("cond.load");

        Builder.SetInsertPoint(CondBlock->getTerminator());
        Value    *Gep        = Builder.CreateConstInBoundsGEP1_32(EltTy, Ptr, Idx);
        LoadInst *Load       = Builder.CreateAlignedLoad(EltTy, Gep, AdjustedAlignVal);
        Value    *NewVResult = Builder.CreateInsertElement(VResult, Load, Idx);

        // Create "else" block, fill it in the next iteration
        BasicBlock *NewIfBlock = ThenTerm->getSuccessor(0);
        NewIfBlock->setName("else");
        BasicBlock *PrevIfBlock = IfBlock;
        IfBlock                 = NewIfBlock;

        // Create the phi to join the new and previous value.
        Builder.SetInsertPoint(NewIfBlock, NewIfBlock->begin());
        PHINode *Phi = Builder.CreatePHI(VecType, 2, "res.phi.else");
        Phi->addIncoming(NewVResult, CondBlock);
        Phi->addIncoming(VResult, PrevIfBlock);
        VResult = Phi;
    }

    CI->replaceAllUsesWith(VResult);
    CI->eraseFromParent();

    ModifiedDT = true;
}

// Translate a masked store intrinsic, like
// void @llvm.masked.store(<16 x i32> %src, <16 x i32>* %addr, i32 align,
//                               <16 x i1> %mask)
// to a chain of basic blocks, that stores element one-by-one if
// the appropriate mask bit is set
//
//   %1 = bitcast i8* %addr to i32*
//   %2 = extractelement <16 x i1> %mask, i32 0
//   br i1 %2, label %cond.store, label %else
//
// cond.store:                                       ; preds = %0
//   %3 = extractelement <16 x i32> %val, i32 0
//   %4 = getelementptr i32* %1, i32 0
//   store i32 %3, i32* %4
//   br label %else
//
// else:                                             ; preds = %0, %cond.store
//   %5 = extractelement <16 x i1> %mask, i32 1
//   br i1 %5, label %cond.store1, label %else2
//
// cond.store1:                                      ; preds = %else
//   %6 = extractelement <16 x i32> %val, i32 1
//   %7 = getelementptr i32* %1, i32 1
//   store i32 %6, i32* %7
//   br label %else2
//   . . .
static void scalarizeMaskedStore(const DataLayout &DL, CallInst *CI, DomTreeUpdater *DTU,
                                 bool &ModifiedDT) {
    Value *Src       = CI->getArgOperand(0);
    Value *Ptr       = CI->getArgOperand(1);
    Value *Alignment = CI->getArgOperand(2);
    Value *Mask      = CI->getArgOperand(3);

    const Align AlignVal = cast<ConstantInt>(Alignment)->getAlignValue();
    auto       *VecType  = cast<VectorType>(Src->getType());

    Type *EltTy = VecType->getElementType();

    IRBuilder<>  Builder(CI->getContext());
    Instruction *InsertPt = CI;
    Builder.SetInsertPoint(InsertPt);
    Builder.SetCurrentDebugLocation(CI->getDebugLoc());

    // Short-cut if the mask is all-true.
    if (isa<Constant>(Mask) && cast<Constant>(Mask)->isAllOnesValue()) {
        Builder.CreateAlignedStore(Src, Ptr, AlignVal);
        CI->eraseFromParent();
        return;
    }

    // Adjust alignment for the scalar instruction.
    const Align AdjustedAlignVal = commonAlignment(AlignVal, EltTy->getPrimitiveSizeInBits() / 8);
    unsigned    VectorWidth      = cast<FixedVectorType>(VecType)->getNumElements();

    if (isConstantIntVector(Mask)) {
        for (unsigned Idx = 0; Idx < VectorWidth; ++Idx) {
            if (cast<Constant>(Mask)->getAggregateElement(Idx)->isNullValue())
                continue;
            Value *OneElt = Builder.CreateExtractElement(Src, Idx);
            Value *Gep    = Builder.CreateConstInBoundsGEP1_32(EltTy, Ptr, Idx);
            Builder.CreateAlignedStore(OneElt, Gep, AdjustedAlignVal);
        }
        CI->eraseFromParent();
        return;
    }

    // If the mask is not v1i1, use scalar bit test operations. This generates
    // better results on X86 at least.
    Value *SclrMask;
    if (VectorWidth != 1) {
        Type *SclrMaskTy = Builder.getIntNTy(VectorWidth);
        SclrMask         = Builder.CreateBitCast(Mask, SclrMaskTy, "scalar_mask");
    }

    for (unsigned Idx = 0; Idx < VectorWidth; ++Idx) {
        // Fill the "else" block, created in the previous iteration
        //
        //  %mask_1 = and i16 %scalar_mask, i32 1 << Idx
        //  %cond = icmp ne i16 %mask_1, 0
        //  br i1 %mask_1, label %cond.store, label %else
        //
        Value *Predicate;
        if (VectorWidth != 1) {
            Value *Mask = Builder.getInt(
                APInt::getOneBitSet(VectorWidth, adjustForEndian(DL, VectorWidth, Idx)));
            Predicate = Builder.CreateICmpNE(Builder.CreateAnd(SclrMask, Mask),
                                             Builder.getIntN(VectorWidth, 0));
        } else {
            Predicate = Builder.CreateExtractElement(Mask, Idx);
        }

        // Create "cond" block
        //
        //  %OneElt = extractelement <16 x i32> %Src, i32 Idx
        //  %EltAddr = getelementptr i32* %1, i32 0
        //  %store i32 %OneElt, i32* %EltAddr
        //
        Instruction *ThenTerm =
            SplitBlockAndInsertIfThen(Predicate, InsertPt, /*Unreachable=*/false,
                                      /*BranchWeights=*/nullptr, DTU);

        BasicBlock *CondBlock = ThenTerm->getParent();
        CondBlock->setName("cond.store");

        Builder.SetInsertPoint(CondBlock->getTerminator());
        Value *OneElt = Builder.CreateExtractElement(Src, Idx);
        Value *Gep    = Builder.CreateConstInBoundsGEP1_32(EltTy, Ptr, Idx);
        Builder.CreateAlignedStore(OneElt, Gep, AdjustedAlignVal);

        // Create "else" block, fill it in the next iteration
        BasicBlock *NewIfBlock = ThenTerm->getSuccessor(0);
        NewIfBlock->setName("else");

        Builder.SetInsertPoint(NewIfBlock, NewIfBlock->begin());
    }
    CI->eraseFromParent();

    ModifiedDT = true;
}

// Translate a masked gather intrinsic like
// <16 x i32 > @llvm.masked.gather.v16i32( <16 x i32*> %Ptrs, i32 4,
//                               <16 x i1> %Mask, <16 x i32> %Src)
// to a chain of basic blocks, with loading element one-by-one if
// the appropriate mask bit is set
//
// %Ptrs = getelementptr i32, i32* %base, <16 x i64> %ind
// %Mask0 = extractelement <16 x i1> %Mask, i32 0
// br i1 %Mask0, label %cond.load, label %else
//
// cond.load:
// %Ptr0 = extractelement <16 x i32*> %Ptrs, i32 0
// %Load0 = load i32, i32* %Ptr0, align 4
// %Res0 = insertelement <16 x i32> poison, i32 %Load0, i32 0
// br label %else
//
// else:
// %res.phi.else = phi <16 x i32>[%Res0, %cond.load], [poison, %0]
// %Mask1 = extractelement <16 x i1> %Mask, i32 1
// br i1 %Mask1, label %cond.load1, label %else2
//
// cond.load1:
// %Ptr1 = extractelement <16 x i32*> %Ptrs, i32 1
// %Load1 = load i32, i32* %Ptr1, align 4
// %Res1 = insertelement <16 x i32> %res.phi.else, i32 %Load1, i32 1
// br label %else2
// . . .
// %Result = select <16 x i1> %Mask, <16 x i32> %res.phi.select, <16 x i32> %Src
// ret <16 x i32> %Result
static void scalarizeMaskedGather(const DataLayout &DL, CallInst *CI, DomTreeUpdater *DTU,
                                  bool &ModifiedDT) {
    Value *Ptrs      = CI->getArgOperand(0);
    Value *Alignment = CI->getArgOperand(1);
    Value *Mask      = CI->getArgOperand(2);
    Value *Src0      = CI->getArgOperand(3);

    auto *VecType = cast<FixedVectorType>(CI->getType());
    Type *EltTy   = VecType->getElementType();

    IRBuilder<>  Builder(CI->getContext());
    Instruction *InsertPt = CI;
    BasicBlock  *IfBlock  = CI->getParent();
    Builder.SetInsertPoint(InsertPt);
    MaybeAlign AlignVal = cast<ConstantInt>(Alignment)->getMaybeAlignValue();

    Builder.SetCurrentDebugLocation(CI->getDebugLoc());

    // The result vector
    Value   *VResult     = Src0;
    unsigned VectorWidth = VecType->getNumElements();

    // Shorten the way if the mask is a vector of constants.
    if (isConstantIntVector(Mask)) {
        for (unsigned Idx = 0; Idx < VectorWidth; ++Idx) {
            if (cast<Constant>(Mask)->getAggregateElement(Idx)->isNullValue())
                continue;
            Value    *Ptr  = Builder.CreateExtractElement(Ptrs, Idx, "Ptr" + Twine(Idx));
            LoadInst *Load = Builder.CreateAlignedLoad(EltTy, Ptr, AlignVal, "Load" + Twine(Idx));
            VResult        = Builder.CreateInsertElement(VResult, Load, Idx, "Res" + Twine(Idx));
        }
        CI->replaceAllUsesWith(VResult);
        CI->eraseFromParent();
        return;
    }

    // If the mask is not v1i1, use scalar bit test operations. This generates
    // better results on X86 at least.
    Value *SclrMask;
    if (VectorWidth != 1) {
        Type *SclrMaskTy = Builder.getIntNTy(VectorWidth);
        SclrMask         = Builder.CreateBitCast(Mask, SclrMaskTy, "scalar_mask");
    }

    for (unsigned Idx = 0; Idx < VectorWidth; ++Idx) {
        // Fill the "else" block, created in the previous iteration
        //
        //  %Mask1 = and i16 %scalar_mask, i32 1 << Idx
        //  %cond = icmp ne i16 %mask_1, 0
        //  br i1 %Mask1, label %cond.load, label %else
        //

        Value *Predicate;
        if (VectorWidth != 1) {
            Value *Mask = Builder.getInt(
                APInt::getOneBitSet(VectorWidth, adjustForEndian(DL, VectorWidth, Idx)));
            Predicate = Builder.CreateICmpNE(Builder.CreateAnd(SclrMask, Mask),
                                             Builder.getIntN(VectorWidth, 0));
        } else {
            Predicate = Builder.CreateExtractElement(Mask, Idx, "Mask" + Twine(Idx));
        }

        // Create "cond" block
        //
        //  %EltAddr = getelementptr i32* %1, i32 0
        //  %Elt = load i32* %EltAddr
        //  VResult = insertelement <16 x i32> VResult, i32 %Elt, i32 Idx
        //
        Instruction *ThenTerm =
            SplitBlockAndInsertIfThen(Predicate, InsertPt, /*Unreachable=*/false,
                                      /*BranchWeights=*/nullptr, DTU);

        BasicBlock *CondBlock = ThenTerm->getParent();
        CondBlock->setName("cond.load");

        Builder.SetInsertPoint(CondBlock->getTerminator());
        Value    *Ptr        = Builder.CreateExtractElement(Ptrs, Idx, "Ptr" + Twine(Idx));
        LoadInst *Load       = Builder.CreateAlignedLoad(EltTy, Ptr, AlignVal, "Load" + Twine(Idx));
        Value    *NewVResult = Builder.CreateInsertElement(VResult, Load, Idx, "Res" + Twine(Idx));

        // Create "else" block, fill it in the next iteration
        BasicBlock *NewIfBlock = ThenTerm->getSuccessor(0);
        NewIfBlock->setName("else");
        BasicBlock *PrevIfBlock = IfBlock;
        IfBlock                 = NewIfBlock;

        // Create the phi to join the new and previous value.
        Builder.SetInsertPoint(NewIfBlock, NewIfBlock->begin());
        PHINode *Phi = Builder.CreatePHI(VecType, 2, "res.phi.else");
        Phi->addIncoming(NewVResult, CondBlock);
        Phi->addIncoming(VResult, PrevIfBlock);
        VResult = Phi;
    }

    CI->replaceAllUsesWith(VResult);
    CI->eraseFromParent();

    ModifiedDT = true;
}

// Translate a masked scatter intrinsic, like
// void @llvm.masked.scatter.v16i32(<16 x i32> %Src, <16 x i32*>* %Ptrs, i32 4,
//                                  <16 x i1> %Mask)
// to a chain of basic blocks, that stores element one-by-one if
// the appropriate mask bit is set.
//
// %Ptrs = getelementptr i32, i32* %ptr, <16 x i64> %ind
// %Mask0 = extractelement <16 x i1> %Mask, i32 0
// br i1 %Mask0, label %cond.store, label %else
//
// cond.store:
// %Elt0 = extractelement <16 x i32> %Src, i32 0
// %Ptr0 = extractelement <16 x i32*> %Ptrs, i32 0
// store i32 %Elt0, i32* %Ptr0, align 4
// br label %else
//
// else:
// %Mask1 = extractelement <16 x i1> %Mask, i32 1
// br i1 %Mask1, label %cond.store1, label %else2
//
// cond.store1:
// %Elt1 = extractelement <16 x i32> %Src, i32 1
// %Ptr1 = extractelement <16 x i32*> %Ptrs, i32 1
// store i32 %Elt1, i32* %Ptr1, align 4
// br label %else2
//   . . .
static void scalarizeMaskedScatter(const DataLayout &DL, CallInst *CI, DomTreeUpdater *DTU,
                                   bool &ModifiedDT) {
    Value *Src       = CI->getArgOperand(0);
    Value *Ptrs      = CI->getArgOperand(1);
    Value *Alignment = CI->getArgOperand(2);
    Value *Mask      = CI->getArgOperand(3);

    auto *SrcFVTy = cast<FixedVectorType>(Src->getType());

    assert(isa<VectorType>(Ptrs->getType()) &&
           isa<PointerType>(cast<VectorType>(Ptrs->getType())->getElementType()) &&
           "Vector of pointers is expected in masked scatter intrinsic");

    IRBuilder<>  Builder(CI->getContext());
    Instruction *InsertPt = CI;
    Builder.SetInsertPoint(InsertPt);
    Builder.SetCurrentDebugLocation(CI->getDebugLoc());

    MaybeAlign AlignVal    = cast<ConstantInt>(Alignment)->getMaybeAlignValue();
    unsigned   VectorWidth = SrcFVTy->getNumElements();

    // Shorten the way if the mask is a vector of constants.
    if (isConstantIntVector(Mask)) {
        for (unsigned Idx = 0; Idx < VectorWidth; ++Idx) {
            if (cast<Constant>(Mask)->getAggregateElement(Idx)->isNullValue())
                continue;
            Value *OneElt = Builder.CreateExtractElement(Src, Idx, "Elt" + Twine(Idx));
            Value *Ptr    = Builder.CreateExtractElement(Ptrs, Idx, "Ptr" + Twine(Idx));
            Builder.CreateAlignedStore(OneElt, Ptr, AlignVal);
        }
        CI->eraseFromParent();
        return;
    }

    // If the mask is not v1i1, use scalar bit test operations. This generates
    // better results on X86 at least.
    Value *SclrMask;
    if (VectorWidth != 1) {
        Type *SclrMaskTy = Builder.getIntNTy(VectorWidth);
        SclrMask         = Builder.CreateBitCast(Mask, SclrMaskTy, "scalar_mask");
    }

    for (unsigned Idx = 0; Idx < VectorWidth; ++Idx) {
        // Fill the "else" block, created in the previous iteration
        //
        //  %Mask1 = and i16 %scalar_mask, i32 1 << Idx
        //  %cond = icmp ne i16 %mask_1, 0
        //  br i1 %Mask1, label %cond.store, label %else
        //
        Value *Predicate;
        if (VectorWidth != 1) {
            Value *Mask = Builder.getInt(
                APInt::getOneBitSet(VectorWidth, adjustForEndian(DL, VectorWidth, Idx)));
            Predicate = Builder.CreateICmpNE(Builder.CreateAnd(SclrMask, Mask),
                                             Builder.getIntN(VectorWidth, 0));
        } else {
            Predicate = Builder.CreateExtractElement(Mask, Idx, "Mask" + Twine(Idx));
        }

        // Create "cond" block
        //
        //  %Elt1 = extractelement <16 x i32> %Src, i32 1
        //  %Ptr1 = extractelement <16 x i32*> %Ptrs, i32 1
        //  %store i32 %Elt1, i32* %Ptr1
        //
        Instruction *ThenTerm =
            SplitBlockAndInsertIfThen(Predicate, InsertPt, /*Unreachable=*/false,
                                      /*BranchWeights=*/nullptr, DTU);

        BasicBlock *CondBlock = ThenTerm->getParent();
        CondBlock->setName("cond.store");

        Builder.SetInsertPoint(CondBlock->getTerminator());
        Value *OneElt = Builder.CreateExtractElement(Src, Idx, "Elt" + Twine(Idx));
        Value *Ptr    = Builder.CreateExtractElement(Ptrs, Idx, "Ptr" + Twine(Idx));
        Builder.CreateAlignedStore(OneElt, Ptr, AlignVal);

        // Create "else" block, fill it in the next iteration
        BasicBlock *NewIfBlock = ThenTerm->getSuccessor(0);
        NewIfBlock->setName("else");

        Builder.SetInsertPoint(NewIfBlock, NewIfBlock->begin());
    }
    CI->eraseFromParent();

    ModifiedDT = true;
}

static void scalarizeMaskedExpandLoad(const DataLayout &DL, CallInst *CI, DomTreeUpdater *DTU,
                                      bool &ModifiedDT) {
    Value *Ptr       = CI->getArgOperand(0);
    Value *Mask      = CI->getArgOperand(1);
    Value *PassThru  = CI->getArgOperand(2);
    Align  Alignment = CI->getParamAlign(0).valueOrOne();

    auto *VecType = cast<FixedVectorType>(CI->getType());

    Type *EltTy = VecType->getElementType();

    IRBuilder<>  Builder(CI->getContext());
    Instruction *InsertPt = CI;
    BasicBlock  *IfBlock  = CI->getParent();

    Builder.SetInsertPoint(InsertPt);
    Builder.SetCurrentDebugLocation(CI->getDebugLoc());

    unsigned VectorWidth = VecType->getNumElements();

    // The result vector
    Value *VResult = PassThru;

    // Adjust alignment for the scalar instruction.
    const Align AdjustedAlignment = commonAlignment(Alignment, EltTy->getPrimitiveSizeInBits() / 8);

    // Shorten the way if the mask is a vector of constants.
    // Create a build_vector pattern, with loads/poisons as necessary and then
    // shuffle blend with the pass through value.
    if (isConstantIntVector(Mask)) {
        unsigned MemIndex = 0;
        VResult           = PoisonValue::get(VecType);
        SmallVector<int, 16> ShuffleMask(VectorWidth, PoisonMaskElem);
        for (unsigned Idx = 0; Idx < VectorWidth; ++Idx) {
            Value *InsertElt;
            if (cast<Constant>(Mask)->getAggregateElement(Idx)->isNullValue()) {
                InsertElt        = PoisonValue::get(EltTy);
                ShuffleMask[Idx] = Idx + VectorWidth;
            } else {
                Value *NewPtr    = Builder.CreateConstInBoundsGEP1_32(EltTy, Ptr, MemIndex);
                InsertElt        = Builder.CreateAlignedLoad(EltTy, NewPtr, AdjustedAlignment,
                                                             "Load" + Twine(Idx));
                ShuffleMask[Idx] = Idx;
                ++MemIndex;
            }
            VResult = Builder.CreateInsertElement(VResult, InsertElt, Idx, "Res" + Twine(Idx));
        }
        VResult = Builder.CreateShuffleVector(VResult, PassThru, ShuffleMask);
        CI->replaceAllUsesWith(VResult);
        CI->eraseFromParent();
        return;
    }

    // If the mask is not v1i1, use scalar bit test operations. This generates
    // better results on X86 at least.
    Value *SclrMask;
    if (VectorWidth != 1) {
        Type *SclrMaskTy = Builder.getIntNTy(VectorWidth);
        SclrMask         = Builder.CreateBitCast(Mask, SclrMaskTy, "scalar_mask");
    }

    for (unsigned Idx = 0; Idx < VectorWidth; ++Idx) {
        // Fill the "else" block, created in the previous iteration
        //
        //  %res.phi.else3 = phi <16 x i32> [ %11, %cond.load1 ], [ %res.phi.else, %else ]
        //  %mask_1 = extractelement <16 x i1> %mask, i32 Idx
        //  br i1 %mask_1, label %cond.load, label %else
        //

        Value *Predicate;
        if (VectorWidth != 1) {
            Value *Mask = Builder.getInt(
                APInt::getOneBitSet(VectorWidth, adjustForEndian(DL, VectorWidth, Idx)));
            Predicate = Builder.CreateICmpNE(Builder.CreateAnd(SclrMask, Mask),
                                             Builder.getIntN(VectorWidth, 0));
        } else {
            Predicate = Builder.CreateExtractElement(Mask, Idx, "Mask" + Twine(Idx));
        }

        // Create "cond" block
        //
        //  %EltAddr = getelementptr i32* %1, i32 0
        //  %Elt = load i32* %EltAddr
        //  VResult = insertelement <16 x i32> VResult, i32 %Elt, i32 Idx
        //
        Instruction *ThenTerm =
            SplitBlockAndInsertIfThen(Predicate, InsertPt, /*Unreachable=*/false,
                                      /*BranchWeights=*/nullptr, DTU);

        BasicBlock *CondBlock = ThenTerm->getParent();
        CondBlock->setName("cond.load");

        Builder.SetInsertPoint(CondBlock->getTerminator());
        LoadInst *Load       = Builder.CreateAlignedLoad(EltTy, Ptr, AdjustedAlignment);
        Value    *NewVResult = Builder.CreateInsertElement(VResult, Load, Idx);

        // Move the pointer if there are more blocks to come.
        Value *NewPtr;
        if ((Idx + 1) != VectorWidth)
            NewPtr = Builder.CreateConstInBoundsGEP1_32(EltTy, Ptr, 1);

        // Create "else" block, fill it in the next iteration
        BasicBlock *NewIfBlock = ThenTerm->getSuccessor(0);
        NewIfBlock->setName("else");
        BasicBlock *PrevIfBlock = IfBlock;
        IfBlock                 = NewIfBlock;

        // Create the phi to join the new and previous value.
        Builder.SetInsertPoint(NewIfBlock, NewIfBlock->begin());
        PHINode *ResultPhi = Builder.CreatePHI(VecType, 2, "res.phi.else");
        ResultPhi->addIncoming(NewVResult, CondBlock);
        ResultPhi->addIncoming(VResult, PrevIfBlock);
        VResult = ResultPhi;

        // Add a PHI for the pointer if this isn't the last iteration.
        if ((Idx + 1) != VectorWidth) {
            PHINode *PtrPhi = Builder.CreatePHI(Ptr->getType(), 2, "ptr.phi.else");
            PtrPhi->addIncoming(NewPtr, CondBlock);
            PtrPhi->addIncoming(Ptr, PrevIfBlock);
            Ptr = PtrPhi;
        }
    }

    CI->replaceAllUsesWith(VResult);
    CI->eraseFromParent();

    ModifiedDT = true;
}

static void scalarizeMaskedCompressStore(const DataLayout &DL, CallInst *CI, DomTreeUpdater *DTU,
                                         bool &ModifiedDT) {
    Value *Src       = CI->getArgOperand(0);
    Value *Ptr       = CI->getArgOperand(1);
    Value *Mask      = CI->getArgOperand(2);
    Align  Alignment = CI->getParamAlign(1).valueOrOne();

    auto *VecType = cast<FixedVectorType>(Src->getType());

    IRBuilder<>  Builder(CI->getContext());
    Instruction *InsertPt = CI;
    BasicBlock  *IfBlock  = CI->getParent();

    Builder.SetInsertPoint(InsertPt);
    Builder.SetCurrentDebugLocation(CI->getDebugLoc());

    Type *EltTy = VecType->getElementType();

    // Adjust alignment for the scalar instruction.
    const Align AdjustedAlignment = commonAlignment(Alignment, EltTy->getPrimitiveSizeInBits() / 8);

    unsigned VectorWidth = VecType->getNumElements();

    // Shorten the way if the mask is a vector of constants.
    if (isConstantIntVector(Mask)) {
        unsigned MemIndex = 0;
        for (unsigned Idx = 0; Idx < VectorWidth; ++Idx) {
            if (cast<Constant>(Mask)->getAggregateElement(Idx)->isNullValue())
                continue;
            Value *OneElt = Builder.CreateExtractElement(Src, Idx, "Elt" + Twine(Idx));
            Value *NewPtr = Builder.CreateConstInBoundsGEP1_32(EltTy, Ptr, MemIndex);
            Builder.CreateAlignedStore(OneElt, NewPtr, AdjustedAlignment);
            ++MemIndex;
        }
        CI->eraseFromParent();
        return;
    }

    // If the mask is not v1i1, use scalar bit test operations. This generates
    // better results on X86 at least.
    Value *SclrMask;
    if (VectorWidth != 1) {
        Type *SclrMaskTy = Builder.getIntNTy(VectorWidth);
        SclrMask         = Builder.CreateBitCast(Mask, SclrMaskTy, "scalar_mask");
    }

    for (unsigned Idx = 0; Idx < VectorWidth; ++Idx) {
        // Fill the "else" block, created in the previous iteration
        //
        //  %mask_1 = extractelement <16 x i1> %mask, i32 Idx
        //  br i1 %mask_1, label %cond.store, label %else
        //
        Value *Predicate;
        if (VectorWidth != 1) {
            Value *Mask = Builder.getInt(
                APInt::getOneBitSet(VectorWidth, adjustForEndian(DL, VectorWidth, Idx)));
            Predicate = Builder.CreateICmpNE(Builder.CreateAnd(SclrMask, Mask),
                                             Builder.getIntN(VectorWidth, 0));
        } else {
            Predicate = Builder.CreateExtractElement(Mask, Idx, "Mask" + Twine(Idx));
        }

        // Create "cond" block
        //
        //  %OneElt = extractelement <16 x i32> %Src, i32 Idx
        //  %EltAddr = getelementptr i32* %1, i32 0
        //  %store i32 %OneElt, i32* %EltAddr
        //
        Instruction *ThenTerm =
            SplitBlockAndInsertIfThen(Predicate, InsertPt, /*Unreachable=*/false,
                                      /*BranchWeights=*/nullptr, DTU);

        BasicBlock *CondBlock = ThenTerm->getParent();
        CondBlock->setName("cond.store");

        Builder.SetInsertPoint(CondBlock->getTerminator());
        Value *OneElt = Builder.CreateExtractElement(Src, Idx);
        Builder.CreateAlignedStore(OneElt, Ptr, AdjustedAlignment);

        // Move the pointer if there are more blocks to come.
        Value *NewPtr;
        if ((Idx + 1) != VectorWidth)
            NewPtr = Builder.CreateConstInBoundsGEP1_32(EltTy, Ptr, 1);

        // Create "else" block, fill it in the next iteration
        BasicBlock *NewIfBlock = ThenTerm->getSuccessor(0);
        NewIfBlock->setName("else");
        BasicBlock *PrevIfBlock = IfBlock;
        IfBlock                 = NewIfBlock;

        Builder.SetInsertPoint(NewIfBlock, NewIfBlock->begin());

        // Add a PHI for the pointer if this isn't the last iteration.
        if ((Idx + 1) != VectorWidth) {
            PHINode *PtrPhi = Builder.CreatePHI(Ptr->getType(), 2, "ptr.phi.else");
            PtrPhi->addIncoming(NewPtr, CondBlock);
            PtrPhi->addIncoming(Ptr, PrevIfBlock);
            Ptr = PtrPhi;
        }
    }
    CI->eraseFromParent();

    ModifiedDT = true;
}

static bool optimizeCallInst(CallInst *CI, bool &ModifiedDT, const TargetTransformInfo &TTI,
                             const DataLayout &DL, DomTreeUpdater *DTU) {
    IntrinsicInst *II = dyn_cast<IntrinsicInst>(CI);
    if (II) {
        // The scalarization code below does not work for scalable vectors.
        if (isa<ScalableVectorType>(II->getType()) ||
            any_of(II->args(), [](Value *V) { return isa<ScalableVectorType>(V->getType()); }))
            return false;

        switch (II->getIntrinsicID()) {
            case Intrinsic::masked_load:
                // Scalarize unsupported vector masked load
                // if (TTI.isLegalMaskedLoad(
                //         CI->getType(),
                //         cast<ConstantInt>(CI->getArgOperand(1))->getAlignValue()))
                //     return false;
                scalarizeMaskedLoad(DL, CI, DTU, ModifiedDT);
                outs() << "scalarized 1 masked load\n";
                return true;
            case Intrinsic::masked_store:
                // if (TTI.isLegalMaskedStore(
                //         CI->getArgOperand(0)->getType(),
                //         cast<ConstantInt>(CI->getArgOperand(2))->getAlignValue()))
                //     return false;
                scalarizeMaskedStore(DL, CI, DTU, ModifiedDT);
                outs() << "scalarized 1 masked store\n";
                return true;
            default:
                break;
        }
    }

    return false;
}

static bool optimizeBlock(BasicBlock &BB, bool &ModifiedDT, const TargetTransformInfo &TTI,
                          const DataLayout &DL, DomTreeUpdater *DTU) {
    bool MadeChange = false;

    BasicBlock::iterator CurInstIterator = BB.begin();
    while (CurInstIterator != BB.end()) {
        if (CallInst *CI = dyn_cast<CallInst>(&*CurInstIterator++))
            MadeChange |= optimizeCallInst(CI, ModifiedDT, TTI, DL, DTU);
        if (ModifiedDT)
            return true;
    }

    return MadeChange;
}

static bool runImpl(Function &F, const TargetTransformInfo &TTI, DominatorTree *DT) {
    std::optional<DomTreeUpdater> DTU;
    if (DT)
        DTU.emplace(DT, DomTreeUpdater::UpdateStrategy::Lazy);

    bool  EverMadeChange = false;
    bool  MadeChange     = true;
    auto &DL             = F.getParent()->getDataLayout();
    while (MadeChange) {
        MadeChange = false;
        for (BasicBlock &BB : llvm::make_early_inc_range(F)) {
            bool ModifiedDTOnIteration = false;
            MadeChange |= optimizeBlock(BB, ModifiedDTOnIteration, TTI, DL, DTU ? &*DTU : nullptr);

            // Restart BB iteration if the dominator tree of the Function was changed
            if (ModifiedDTOnIteration)
                break;
        }

        EverMadeChange |= MadeChange;
    }
    return EverMadeChange;
}
