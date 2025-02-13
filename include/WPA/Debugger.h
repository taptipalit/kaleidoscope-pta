#include "InvariantHandler.h"
#include "MemoryModel/PointerAnalysis.h"

#ifndef DEBUGGER_H_
#define DEBUGGER_H_


using namespace SVF;
class Debugger: public InvariantHandler {
    private:

        int dbgTgtID;
        PointerAnalysis* _pta;

        llvm::Function* ptdTargetCheckFn;
        llvm::Function* recordTargetFn;

        std::map<Value*, int> recorded;

    public:
        Debugger(SVFModule* S, llvm::Module* M, PAG* p, LoopInfoWrapperPass* lInfo,
                PointerAnalysis* pta): 
                InvariantHandler(S, M, p, lInfo), _pta(pta) {
        }

        void addFunctionDefs();
        void instrumentPointer(llvm::Instruction*);
        void init();
};

#endif
