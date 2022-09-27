#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include <Util/SVFBasicTypes.h>
#include <Util/SVFUtil.h>
#include <Graphs/PAG.h>
#include "llvm/Analysis/LoopInfo.h"



#include<vector>

#ifndef IHANDLER_H_
#define IHANDLER_H_


using namespace SVF;
class InvariantHandler {
    protected:

        PAG* pag;

        llvm::Module* mod;

        SVFModule* svfMod;

    private:
        std::map<llvm::Value*, int> valueToKaliIdMap;
        std::map<int, llvm::Value*> kaliIdToValueMap;
        int kaliInvariantId;

        llvm::Function* vgepPtdRecordFn;
        llvm::Function* ptdTargetCheckFn;
        llvm::Function* updatePWCFn;
        llvm::Function* checkPWCFn;

        LoopInfoWrapperPass* loopInfo;

    public:
        void handleVGEPInvariants();
        void handlePWCInvariants();
        void instrumentVGEPInvariant(llvm::GetElementPtrInst*, std::vector<llvm::Value*>&,
                std::map<Value*, std::vector<int>>&);
        void recordTarget(std::vector<int>&, llvm::Value*, llvm::Function*);

        InvariantHandler(SVFModule* S, llvm::Module* M, PAG* p, LoopInfoWrapperPass* lInfo): kaliInvariantId(0), ptdTargetCheckFn(nullptr), mod(M), svfMod(S), pag(p), loopInfo(lInfo) {
        }

        void initVGEPInvariants();
        void initPWCInvariants();
        void init();
        int computeOffsetInPWC(std::vector<llvm::GetElementPtrInst*>&, llvm::GetElementPtrInst*);
};

#endif
