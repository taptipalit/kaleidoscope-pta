#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include <Util/SVFBasicTypes.h>
#include <Util/SVFUtil.h>
#include <Graphs/PAG.h>
#include "llvm/Analysis/LoopInfo.h"



#include<vector>

using namespace SVF;
class InvariantHandler {
    private:
        std::map<llvm::Value*, int> valueToKaliIdMap;
        std::map<int, llvm::Value*> kaliIdToValueMap;
        int kaliInvariantId;

        llvm::Function* vgepPtdRecordFn;
        llvm::Function* ptdTargetCheckFn;
        llvm::Function* updatePWCFn;
        llvm::Function* checkPWCFn;
        llvm::Module* mod;
        SVFModule* svfMod;

        PAG* pag;
        LoopInfo* loopInfo;

    public:
        void handleVGEPInvariants();
        void handlePWCInvariants();
        void instrumentVGEPInvariant(llvm::GetElementPtrInst*, std::vector<llvm::Value*>&);
        void recordTarget(int, llvm::Value*);

        InvariantHandler(SVFModule* S, llvm::Module* M, PAG* p, LoopInfo* lInfo): kaliInvariantId(0), ptdTargetCheckFn(nullptr), mod(M), svfMod(S), pag(p), loopInfo(lInfo) {
            init();
        }

        void initVGEPInvariants();
        void initPWCInvariants();
        void init();
};
