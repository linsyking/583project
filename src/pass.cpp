#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/Twine.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Use.h>
#include <llvm/IR/User.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/Casting.h>
#include <vector>
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include <iostream>

using namespace llvm;

namespace {
struct VECPass : public PassInfoMixin<VECPass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
        llvm::LoopAnalysis::Result &li = FAM.getResult<LoopAnalysis>(F);
        IRBuilder<>                 builder(F.getContext());
        std::vector<CallInst *>     MaskedLoadInstructions;

        for (auto &B : F) {
            for (auto &I : B) {
                if (auto *CI = dyn_cast<CallInst>(&I)) {
                    auto *maskedFunction = CI->getCalledFunction();
                    if (maskedFunction &&
                        maskedFunction->getName().starts_with("llvm.masked.load")) {
                        MaskedLoadInstructions.push_back(CI);
                    }
                }
            }
        }

        for (auto *CI : MaskedLoadInstructions) {
            // Step 1: Generate unmasked load
            BasicBlock *BB         = CI->getParent();
            Type       *VectorType = CI->getCalledFunction()->getReturnType();
            builder.SetInsertPoint(CI);
            // Step 2.1: Cast mask into integer

            auto  DL      = DataLayout(F.getParent());
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
            auto *unmaskedLoad = builder.CreateLoad(VectorType, CI->getArgOperand(0));
            // Get alignment from the original masked load
            // Problem: This is not accurate, we should get the alignment from the original
            unmaskedLoad->setAlignment(CI->getArgOperand(0)->getPointerAlignment(DL));

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
        return PreservedAnalyses::none();
    }
};

// In order to prevent exceptions, we avoid out-of-bound memory access using padding. Each memory
// allocation is padded to the next multiple of the an allocation is padded up to the next 32 bytes.

// Hardcoded hardware's vectorization factor, in the number of bits
const int vecFact = 256;
struct PaddingPass : public PassInfoMixin<PaddingPass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
        std::cout << "running...\n"
                    << std::endl;
        // static array
        // stack array
        for (auto &B : F)
        {
            for (auto &I : B)
            {
                if (auto *AI = dyn_cast<AllocaInst>(&I))
                {
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

                    if (ArrayType *arrayType = dyn_cast<ArrayType>(AI->getAllocatedType()))
                    {
                        // Now we can get the number of elements in the array.
                        uint64_t numElements = arrayType->getNumElements();
                        std::cout << "alloca array of " << numElements << " elements " << std::endl;

                        Type *elementType = arrayType->getElementType();
                        DataLayout DL = F.getParent()->getDataLayout();
                        uint64_t elementSize = DL.getTypeAllocSize(elementType) * 8;
                        std::cout << "element size " << elementSize << " bits " << std::endl;

                        uint64_t arraySizeInBits = elementSize * numElements;
                        std::cout << "array size " << arraySizeInBits << " bits " << std::endl;

                        if (arraySizeInBits % vecFact != 0)
                        {
                            uint64_t paddingSize = vecFact - (arraySizeInBits % vecFact);
                            std::cout << "padding of size " << paddingSize << " bits needed" << std::endl;

                            auto &context = AI->getContext();
                            auto paddingType = ArrayType::get(Type::getInt8Ty(context), paddingSize / 8);

                            paddingType->print(llvm::errs());
                            llvm::errs() << "\n";

                            // 创建IRBuilder，它可以帮助我们构建新的指令
                            IRBuilder<> builder(context);

                            // 将插入点设置为现有alloca指令的位置
                            builder.SetInsertPoint(AI->getParent(), AI->getIterator());

                            // 现在创建一个新的alloca指令
                            AllocaInst *newAlloca = builder.CreateAlloca(paddingType, nullptr, "pad");

                            // 设置新alloca的对齐
                            newAlloca->setAlignment(AI->getAlign());
                            // if (paddingSize % elementSize == 0)
                            // {
                            //     // calculate Value *ArraySize
                            //     uint64_t newArrayLength = numElements + (paddingSize / elementSize);
                            //     std::cout << "extend array to " << newArrayLength << " elements " << std::endl;

                            //     Type *int64Type = Type::getInt64Ty(F.getContext());
                            //     Value *newArrayLengthVal =
                            //         ConstantInt::get(int64Type, newArrayLength, false);
                            //     newArrayLengthVal->print(llvm::errs()); // 将其打印到stderr，也可以选择其他输出流
                            //     llvm::errs() << "\n";
                            //     // build new alloc
                            //     IRBuilder<> builder(AI);
                            //     // AllocaInst *newAI = builder.CreateAlloca(elementType, nullptr, arraysize,
                            //     // "");
                            // }
                        }
                        else
                            std::cout << "padding not needed" << std::endl;
                    }
                    else
                    {
                        // This alloca is for a single element, not an array.
                        if (AI->isArrayAllocation())
                        {
                            std::cout << "alloca array of variable elements " << std::endl;
                            llvm::Value *sizeValue = AI->getArraySize(); // 获取数组大小参数
                            // 输出该值的详细信息，或者进一步分析
                            if (sizeValue)
                            {
                                sizeValue->print(llvm::errs()); // 将其打印到stderr，也可以选择其他输出流
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
                    return false;
                });
            }};
}
