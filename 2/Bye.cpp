#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

// Our pass called Bye
struct Bye : PassInfoMixin<Bye> {

  void processLoop(
    Loop* L, const FunctionCallee& loop_start, const FunctionCallee& loop_end) {
    // Processing loop
    IRBuilder<> Builder(L->getLoopPreheader()->getTerminator());
    Builder.CreateCall(loop_start);
      
    Builder.SetInsertPoint(L->getExitBlock()->getTerminator());
    Builder.CreateCall(loop_end);

    // Recursive call to find loop subloops and process them
    for (auto *SubL : L->getSubLoops()) {
      processLoop(SubL, loop_start, loop_end);
    }
  }

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    // Creating empty functions loop_start() and loop_end() in IR
    FunctionCallee loop_start = F.getParent()->getOrInsertFunction(
        "loop_start", Type::getVoidTy(F.getContext()));
    FunctionCallee loop_end = F.getParent()->getOrInsertFunction(
        "loop_end", Type::getVoidTy(F.getContext()));

    // Calling processLoop for every top-level loop in function F
    auto &LInfo = FAM.getResult<LoopAnalysis>(F);
    for (auto *L : LInfo) {
      processLoop(L, loop_start, loop_end);
    }

    return PreservedAnalyses::all();
  }
};

} // namespace

/* New PM Registration */
llvm::PassPluginLibraryInfo getByePluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "Bye", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerVectorizerStartEPCallback(
                [](llvm::FunctionPassManager &PM, OptimizationLevel Level) {
                  PM.addPass(Bye());
                });
            PB.registerPipelineParsingCallback(
                [](StringRef Name, llvm::FunctionPassManager &PM,
                   ArrayRef<llvm::PassBuilder::PipelineElement>) {
                  if (Name == "goodbye") {
                    PM.addPass(Bye());
                    return true;
                  }
                  return false;
                });
          }};
}

#ifndef LLVM_BYE_LINK_INTO_TOOLS
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getByePluginInfo();
}
#endif
