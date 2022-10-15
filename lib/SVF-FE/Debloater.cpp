/* SVF - Static Value-Flow Analysis Framework
Copyright (C) 2015 Yulei Sui
Copyright (C) 2015 Jingling Xue

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
 * HeapTypeAnalyzer.cpp
 *
 *  Created on: Oct 8, 2013
 */



#define DEBUG_TYPE "debloater"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/InstIterator.h"


#include "SVF-FE/Debloater.h"

using namespace SVF;

// Identifier variable for the pass
char Debloater::ID = 0;

// Register the pass
static llvm::RegisterPass<Debloater> DEB ("debloater",
        "Debloater");

Function* Debloater::getFunction(Constant* cons, int i) {
    ConstantStruct* consStruct = SVFUtil::dyn_cast<ConstantStruct>(cons);
    if (!consStruct) return nullptr;

    Value* val_I = consStruct->getOperand(i);
    if (Function* function = SVFUtil::dyn_cast<Function>(val_I)) {
        return function;
    } else {
        return nullptr;
    }
}

void Debloater::collectFunctionPointers(GlobalVariable* gvar) {
    Type* ty = gvar->getType();
    PointerType* pty = SVFUtil::dyn_cast<PointerType>(ty);
    if (!pty) return;
    StructType* stTy = SVFUtil::dyn_cast<StructType>(pty->getPointerElementType());
    if (!stTy) return;

    Constant* init = gvar->getInitializer();

    for (int i = 0; i < stTy->getNumElements(); i++) {
        Type* elemTy = stTy->getElementType(i);
        PointerType* ptrElemTy = SVFUtil::dyn_cast<PointerType>(elemTy);
        if (!ptrElemTy) continue;
        Type* elemBaseType = ptrElemTy->getPointerElementType();
        if (SVFUtil::isa<FunctionType>(elemBaseType)) {
            Function* func = getFunction(init, i);
            if (func) {
                globalFptrIndices[gvar][i] = func;
            }
        }
    }
}

void Debloater::buildCallGraph(Module& module) {
    std::vector<Function*> workList;

    std::vector<Function*> visited;

    workList.push_back(module.getFunction("main"));

    while(!workList.empty()) {
        Function* F = workList.back();
        workList.pop_back();
        accessibleFuncs.push_back(F);
        visited.push_back(F);
        for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
            if (CallInst* callInst = SVFUtil::dyn_cast<CallInst>(&*I)) {
                Function* calledFunction = callInst->getCalledFunction();
                if (calledFunction &&
                        std::find(visited.begin(), visited.end(), calledFunction) == visited.end()) {
                    workList.push_back(calledFunction);
                }
            } else {
                Instruction* inst = &*I;
                for (int i = 0; i < inst->getNumOperands(); i++) {
                    Value* operand = inst->getOperand(i)->stripPointerCasts();
                    if (Function* func = SVFUtil::dyn_cast<Function>(operand)) {
                        workList.push_back(func);
                    }

                }
            }
        }
    }
}

bool
Debloater::runOnModule (Module & module) {
    int atFunctions = 0;
    int totalFunctions = module.getFunctionList().size();
    for (Function& func: module.getFunctionList()) {
        if (func.hasAddressTaken()) {
            atFunctions++;
        }
    }

    llvm::outs() << "Address taken functions: " << atFunctions << "\n";
    llvm::outs() << "Total functions: " << totalFunctions << "\n";
    /*
    buildCallGraph(module);
    for (GlobalVariable& gvar: module.getGlobalList()) {
        collectFunctionPointers(&gvar);
    }

    std::vector<int> indicesAccessed;

    for (auto& it: globalFptrIndices) {
        GlobalVariable* gvar = it.first;
        indicesAccessed.clear();
        
        bool accessed = false;
        // Find all the users of this gvar
        for (User* user: gvar->users()) {
            if (GetElementPtrInst* gepInst = SVFUtil::dyn_cast<GetElementPtrInst>(user)) {
                if (std::find(accessibleFuncs.begin(),
                            accessibleFuncs.end(), gepInst->getFunction()) 
                        == accessibleFuncs.end()) {
                    continue;
                }
                ConstantInt* index = SVFUtil::dyn_cast<ConstantInt>(gepInst->getOperand(gepInst->getNumOperands()-1));

                if (index) {
                    indicesAccessed.push_back(index->getZExtValue());
                }
            } else if (Instruction* inst = SVFUtil::dyn_cast<Instruction>(user)) {
                if (std::find(accessibleFuncs.begin(),
                            accessibleFuncs.end(), inst->getFunction()) 
                 != accessibleFuncs.end()) {
                    accessed = true;
                }
            }
        }

        for (auto& it2: it.second) {
            int index = it2.first;
            Function* func = it2.second;
        //    llvm::outs() << "Debloating candidate: " << func->getName() << "\n";
            if (std::find(indicesAccessed.begin(), indicesAccessed.end(),
                        index) == indicesAccessed.end()) {
         //       llvm::outs() << "Can debloat\n";
            }
        }
    }
    */

    return false;
}
