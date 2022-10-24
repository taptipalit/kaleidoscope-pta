#ifndef HEAPTYPEANALYZER_H
#define HEAPTYPEANALYZER_H

#include "llvm/Analysis/CFLAndersAliasAnalysis.h"
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

    std::vector<std::string> memAllocFns;

    std::vector<std::string> L_A0_Fns;
    enum HeapTy {
        StructTy,
        ArrayTy,
        ScalarTy
    };

    void buildCallGraphs(llvm::Module&);
    bool returnsUntypedMalloc(llvm::Function*);
    void findHeapContexts(llvm::Module&);


    std::map<Function*, std::vector<Function*>> callers;
    std::map<Function*, std::vector<Function*>> callees;

    std::map<int, std::vector<Function*>, std::greater<int>> callerDistMap;

    std::vector<Function*> heapCalls;

    std::map<StringRef, Function*> clonedFunctionMap;

public:
    static char ID;

    std::vector<Function*>& getHeapCalls() { return heapCalls; }
    std::map<StringRef, Function*>& getClonedFunctionMap() { return clonedFunctionMap; }

    HeapTypeAnalyzer() : ModulePass(ID) {
        /*
        memAllocFns.push_back("ngx_alloc");
        memAllocFns.push_back("ngx_palloc");
        memAllocFns.push_back("ngx_pnalloc");
        memAllocFns.push_back("ngx_palloc_small");
        memAllocFns.push_back("ngx_palloc_block");
        memAllocFns.push_back("ngx_palloc_large");
        memAllocFns.push_back("ngx_pmemalign");
        memAllocFns.push_back("ngx_pcalloc");
        memAllocFns.push_back("ngx_array_init");
        memAllocFns.push_back("ngx_array_create");
        memAllocFns.push_back("ngx_list_init");
        memAllocFns.push_back("ngx_hash_init");
        memAllocFns.push_back("ngx_hash_keys_array_init");
        */

        memAllocFns.push_back("malloc");
        memAllocFns.push_back("mmap");
        memAllocFns.push_back("calloc");

        /*
        L_A0_Fns.push_back("ngx_array_push");
        L_A0_Fns.push_back("ngx_array_push_n");
        L_A0_Fns.push_back("ngx_list_push");
        */
    }
    StringRef getPassName() const
    {
        return "Analyze heap types";
    }

    virtual void handleVersions(Module & M);
    virtual bool runOnModule (Module & M);
    virtual void getAnalysisUsage(AnalysisUsage &AU) const
    {
        AU.addRequired<llvm::CFLAndersAAWrapperPass>();
        // This pass modifies the control-flow graph of the function
        //AU.setPreservesCFG();
    }
    CallInst* findCInstFA(llvm::Value*);
    void deriveHeapAllocationTypes(llvm::Module&);

    void deriveHeapAllocationTypesWithCloning(llvm::Module&);

    bool deepClone(llvm::Function*, llvm::Function*&, std::vector<std::string>&, 
            llvm::Type*, llvm::Type*);

    HeapTypeAnalyzer::HeapTy getSizeOfTy(llvm::Module&, llvm::LLVMContext&, llvm::MDNode*, llvm::MDNode*, llvm::MDNode*);
    void removePoolAllocatorBody(llvm::Module&);

};

}
#endif
