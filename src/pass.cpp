//===-- Frequent Path Loop Invariant Code Motion Pass --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//
//
// EECS583 W24 - This pass can be used as a template for your FPLICM homework
//               assignment.
//               The passes get registered as "fplicm-correctness" and
//               "fplicm-performance".
//
//
////===-------------------------------------------------------------------===//
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

using namespace llvm;

namespace {
struct VECPass : public PassInfoMixin<VECPass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
        llvm::LoopAnalysis::Result &li = FAM.getResult<LoopAnalysis>(F);
        
        for (auto &B : F) {
            for (auto &I : B) {
                if (auto *CI = dyn_cast<CallInst>(&I)) {
                    auto* maskedFunction = CI->getCalledFunction();
                    if (maskedFunction && maskedFunction->getName().starts_with("llvm.masked.load")) {
                        // Step 1: Generate unmasked load
                        auto* unmaskedLoad = new LoadInst(maskedFunction->getReturnType(), CI->getArgOperand(0), "", CI);
                        CI->replaceAllUsesWith(unmaskedLoad);

                        // Step 2.1: Cast mask into integer
                        auto *DL = new DataLayout(F.getParent());
                        auto *mask = CI->getArgOperand(2);
                        auto *maskTy = dyn_cast<VectorType>(mask->getType());
                        auto numBits = DL->getTypeAllocSizeInBits(maskTy);
                        auto *intTy = IntegerType::get(maskTy->getContext(), numBits);
                        // outs() << *CI->getArgOperand(2)->getType() << " TO " << *intTy << '\n';
                        auto *castInst = new BitCastInst(CI->getArgOperand(2), intTy, "", unmaskedLoad);
                    
                        // Step 2.2: Guard the load with branch
                        auto cmpInst = new ICmpInst(unmaskedLoad, CmpInst::ICMP_EQ, castInst, ConstantInt::get(castInst->getType(), 0), "");
                        auto *newBB = B.splitBasicBlock(cmpInst);
                        auto *trueBB = BasicBlock::Create(F.getContext(), "", &F, newBB);
                        auto brInst = BranchInst::Create(trueBB, newBB, cmpInst, &B);
                        B.getTerminator()->eraseFromParent();

                        // Step 3: Create select instruction
                        auto selectInst = SelectInst::Create(mask, unmaskedLoad, UndefValue::get(unmaskedLoad->getType()), "", CI);

                        // Step 4: Remove masked load
                        CI->removeFromParent();
                    }
                }
            }
        }

        return PreservedAnalyses::none();
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
                    return false;
                });
            }};
}
