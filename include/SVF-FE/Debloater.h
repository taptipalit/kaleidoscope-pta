#ifndef DEBLOATER_H
#define DEBLOATER_H

#include "Util/BasicTypes.h"
#include <map>
#include <utility>
#include <vector>
#include <set>


namespace SVF
{

class Debloater : public ModulePass
{
private:
    bool hasFunctionPointer(GlobalVariable*);

    Function* getFunction(Constant*, int);

    std::map<GlobalVariable*, std::map<int, Function*>> globalFptrIndices;

    void collectFunctionPointers(GlobalVariable*);

    std::vector<Function*> accessibleFuncs;

    void buildCallGraph(Module&); 
public:
    static char ID;

    Debloater() : ModulePass(ID) {
    }

    StringRef getPassName() const
    {
        return "Debloat unneeded functions";
    }

    virtual bool runOnModule (Module & M);
    virtual void getAnalysisUsage(AnalysisUsage &AU) const
    {
        // This pass modifies the control-flow graph of the function
        //AU.setPreservesCFG();
    }


};

}
#endif
