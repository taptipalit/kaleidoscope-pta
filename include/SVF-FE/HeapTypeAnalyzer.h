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

    std::vector<std::string> memAllocFns;

    std::vector<std::string> L_A0_Fns;
    /*
    enum HeapTy {
        StructTy,
        SimpleArrayTy,
        StructArrayTy,
        ScalarTy
    };
    */
public:
    static char ID;


    HeapTypeAnalyzer() : ModulePass(ID) {
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

        memAllocFns.push_back("malloc");
        memAllocFns.push_back("mmap");
        memAllocFns.push_back("calloc");

        L_A0_Fns.push_back("ngx_array_push");
        L_A0_Fns.push_back("ngx_array_push_n");
        L_A0_Fns.push_back("ngx_list_push");


        
    }
    StringRef getPassName() const
    {
        return "Analyze heap types";
    }

    virtual void handleVersions(Module & M);
    virtual bool runOnModule (Module & M);
    virtual void getAnalysisUsage(AnalysisUsage &AU) const
    {
        // This pass modifies the control-flow graph of the function
        //AU.setPreservesCFG();
    }
    CallInst* findCInstFA(llvm::Value*);
    void deriveHeapAllocationTypes(llvm::Module&);

    /*
    void deriveHeapAllocationTypesWithCloning(llvm::Module&);

    bool deepClone(llvm::Function*, llvm::Function*&, std::vector<std::string>&, 
            llvm::Type*, llvm::Type*);
            */

    llvm::Type* getSizeOfTy(llvm::Module&, llvm::LLVMContext&, llvm::MDNode*, llvm::MDNode*, llvm::MDNode*, llvm::CallInst*);
    void removePoolAllocatorBody(llvm::Module&);

};

}
#endif
