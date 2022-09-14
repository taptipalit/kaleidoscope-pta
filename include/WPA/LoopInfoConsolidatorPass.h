#ifndef SVF_LOOP
#define SVF_LOOP
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Analysis/LoopInfo.h"
#include "Util/BasicTypes.h"
#include "llvm/Pass.h"

#include <unordered_set>

using namespace std;

class LoopInfoConsolidatorPass : public llvm::ModulePass {
    static char ID;
    unordered_set<llvm::BasicBlock*> bbInLoops;

    public:

    LoopInfoConsolidatorPass() : llvm::ModulePass(ID) {}

    virtual bool runOnModule(llvm::Module& M);

    bool bbIsLoop(llvm::BasicBlock* bb) {
        return std::find(bbInLoops.begin(), bbInLoops.end(), bb) != bbInLoops.end();
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
        AU.setPreservesCFG();
        AU.addRequired<llvm::LoopInfoWrapperPass>();
    }
};


// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerLoopInfoConsolidatorPass(const llvm::PassManagerBuilder &,
                         llvm::legacy::PassManagerBase &PM) {
  PM.add(new LoopInfoConsolidatorPass());
}

static llvm::RegisterStandardPasses
  RegisterMyPass(llvm::PassManagerBuilder::EP_EarlyAsPossible,
                 registerLoopInfoConsolidatorPass);
#endif
