#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/Debug.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"

#include "llvm/ADT/SmallVector.h"
#include <vector>
#include <set>
#include <algorithm>
#include <map>
#include <cstring>
#include <cmath>

#include <iostream>
#include <fstream>
#include <string>

using namespace llvm;

llvm::cl::list<std::string> KmemAllocFuncs("kmem-alloc-funcs", llvm::cl::value_desc("Kmem alloc functions"), llvm::cl::CommaSeparated);

namespace {
  struct SkeletonPass : public ModulePass {
    static char ID;
    SkeletonPass() : ModulePass(ID) {}

    void removeMemAllocDefs(Module* module) {
        for (Function& func: module->getFunctionList()) {
            for (std::string funcName: KmemAllocFuncs) {
                if (func.getName() == funcName) {
                    func.deleteBody();
                }
            }
        }
    }

    virtual bool runOnModule(Module &M) {
        removeMemAllocDefs(&M);
        return false;
    }
  };
}

char SkeletonPass::ID = 0;

// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerSkeletonPass(const PassManagerBuilder &,
                         legacy::PassManagerBase &PM) {
  PM.add(new SkeletonPass());
}

static RegisterStandardPasses
  RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                 registerSkeletonPass);


static RegisterPass<SkeletonPass> X("nuller", "Null the bodies of the provided functions",
        false /* Only looks at CFG */,
        false /* Analysis Pass */);

