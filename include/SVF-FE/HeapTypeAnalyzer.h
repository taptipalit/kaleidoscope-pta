#ifndef HEAPTYPEANALYZER_H
#define HEAPTYPEANALYZER_H

#include "Util/BasicTypes.h"
#include <map>
#include <utility>
#include <vector>
#include <set>


namespace SVF
{

class HeapTypeAnalyzer : public ModulePass
{
private:
    // Private methods

    // Private variables

    std::vector<std::string> poolAllocFns;
public:
    static char ID;
    HeapTypeAnalyzer() : ModulePass(ID) {
        poolAllocFns.push_back("ngx_palloc");
        poolAllocFns.push_back("ngx_pnalloc");
        poolAllocFns.push_back("ngx_palloc_small");
        poolAllocFns.push_back("ngx_palloc_block");
        poolAllocFns.push_back("ngx_palloc_large");
        poolAllocFns.push_back("ngx_pmemalign");
        poolAllocFns.push_back("ngx_pcalloc");
    }
    StringRef getPassName() const
    {
        return "Analyze heap types";
    }
    virtual bool runOnModule (Module & M);
    virtual void getAnalysisUsage(AnalysisUsage &AU) const
    {
        // This pass modifies the control-flow graph of the function
        //AU.setPreservesCFG();
    }
    CallInst* findCInstFA(llvm::Value*);
    void deriveHeapAllocationTypesWithCloning(llvm::Module&);

    bool deepClone(llvm::Function*, llvm::Function*&, std::vector<std::string>&, 
            llvm::Type*, llvm::Type*);


};

}
#endif
