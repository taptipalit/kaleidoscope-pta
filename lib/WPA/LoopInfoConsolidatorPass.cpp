#include "WPA/LoopInfoConsolidatorPass.h"

using namespace llvm;

bool LoopInfoConsolidatorPass::runOnModule(Module& M) {
	outs() << "I saw a module called " << M.getName() << "!\n";
	
    for (Function& func: M.getFunctionList()) {
        if (func.isDeclaration()) {
            continue;
        }
        LoopInfoWrapperPass& lInfoPass = getAnalysis<LoopInfoWrapperPass>(func);
        LoopInfo& lInfo = lInfoPass.getLoopInfo();
        for (Loop* loop: lInfo) {
            for (BasicBlock* bb: loop->getBlocksVector()) {
                bbInLoops.insert(bb);
            }
        }
    }
    
    /*
    outs() << "loop bbs:\n";
    for (BasicBlock* bb: bbInLoops) {
        bb->dump();
    }
    */
	return false;
}


char LoopInfoConsolidatorPass::ID = 0;
