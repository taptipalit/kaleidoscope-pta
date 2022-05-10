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



#define DEBUG_TYPE "heap-type"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Transforms/Utils/Cloning.h"


#include "SVF-FE/HeapTypeAnalyzer.h"

using namespace SVF;

// Identifier variable for the pass
char HeapTypeAnalyzer::ID = 0;

// Register the pass
static llvm::RegisterPass<HeapTypeAnalyzer> HT ("heap-type-analyzer",
        "Heap Type Analyzer");

void HeapTypeAnalyzer::handleVersions(Module& module) {
    for (Module::iterator MIterator = module.begin(); MIterator != module.end(); MIterator++) {
        if (Function *F = SVFUtil::dyn_cast<Function>(&*MIterator)) {
            for (llvm::inst_iterator I = llvm::inst_begin(F), E = llvm::inst_end(F); I != E; ++I) {
                Instruction* inst = &*I;
                CallInst* cInst = SVFUtil::dyn_cast<CallInst>(inst);
                if (cInst) {
                    Function* calledFunc = cInst->getCalledFunction();
                    if (calledFunc) {
                        for (std::string memAllocFnName: memAllocFns) {
                            if (calledFunc->getName().startswith(memAllocFnName+".")) {
                                Function* memAllocFun = module.getFunction(memAllocFnName);
                                cInst->setCalledFunction(memAllocFun); // remove versioning
                            }
                        }
                        for (std::string la0FnName: L_A0_Fns) {
                            if (calledFunc->getName().startswith(la0FnName+".")) {
                                Function* la0Fun = module.getFunction(la0FnName);
                                cInst->setCalledFunction(la0Fun); // remove versioning
                            }
                        }
                    }
                }
            }
                /*
            bool isMemAlloc = false;
            for (std::string memAllocFn: memAllocFns) {
                if (F->getName().startswith(memAllocFn)) {
                    isMemAlloc = true;
                    break;
                }
            }
            if (isMemAlloc) {
                memAllocFns.push_back(F->getName().str());
            }
            bool isLA0 = false;
            for (std::string L_A0_Fn: L_A0_Fns) {
                if (F->getName().startswith(L_A0_Fn)) {
                    isLA0 = true;
                }
            }
            if (isLA0) {
                L_A0_Fns.push_back(F->getName().str());
            }
            */
        }
    }
 
}

void HeapTypeAnalyzer::removePoolAllocatorBody(Module& module) {
    for (Module::iterator MIterator = module.begin(); MIterator != module.end(); MIterator++) {
        if (Function *F = SVFUtil::dyn_cast<Function>(&*MIterator)) {
            if (std::find(memAllocFns.begin(), memAllocFns.end(), F->getName()) != memAllocFns.end()) {
                F->deleteBody();
            }
            if (std::find(L_A0_Fns.begin(), L_A0_Fns.end(), F->getName()) != L_A0_Fns.end()) {
                F->deleteBody();
            }
        }
    }
}

CallInst* HeapTypeAnalyzer::findCInstFA(Value* val) {
    std::vector<Value*> workList;
    workList.push_back(val);

    while (!workList.empty()) {
        Value* val = workList.back();
        workList.pop_back();
        
        for (User* user: val->users()) {
            if (user == val) continue;
            Instruction* userInst = SVFUtil::dyn_cast<Instruction>(user);
            assert(userInst && "Must be instruction");
            if (CallInst* cInst = SVFUtil::dyn_cast<CallInst>(userInst)) {
                return cInst;
            } else {
                workList.push_back(userInst);
            }
        }
    }
    return nullptr;
}

/**
 * func: The function that must be deep cloned
 * mallocFunctions: The list of malloc wrappers that our system is aware of
 * (malloc, calloc)
 * cloneHeapTy: What type the deepest malloc clone should be set to
 * origHeapTy: What type the deepest malloc of the original should be set to
 */
bool HeapTypeAnalyzer::deepClone(llvm::Function* func, llvm::Function*& topClonedFunc, std::vector<std::string>& mallocFunctions, 
        Type* cloneHeapTy, Type* origHeapTy) {
    std::vector<Function*> workList;
    std::vector<Function*> visited;

    std::vector<Function*> cloneFuncs; // The list of functions to clone
    cloneFuncs.push_back(func);
    std::vector<CallInst*> mallocCalls; // the list of calls to malloc. Should be 1

    workList.push_back(func);
    visited.push_back(func);
    while (!workList.empty()) {
        Function* F = workList.back();
        workList.pop_back();
        for (llvm::inst_iterator I = llvm::inst_begin(F), E = llvm::inst_end(F); I != E; ++I) {
            if (CallInst* CI = SVFUtil::dyn_cast<CallInst>(&*I)) {
                Function* calledFunc = CI->getCalledFunction();
                if (!calledFunc) {
                    // We can't handle indirect calls
                    return false;
                }
                if (std::find(mallocFunctions.begin(), mallocFunctions.end(), calledFunc->getName())
                        != mallocFunctions.end()) {
                    mallocCalls.push_back(CI);
                } else {
                    // If it returns a pointer
                    PointerType* retPtrTy = SVFUtil::dyn_cast<PointerType>(calledFunc->getReturnType());
                    if (!retPtrTy) {
                        // We don't need to clone this function as it doesn't
                        // return a pointer.
                        //
                        // We only need to clone the path that returns the
                        // heap object created on the heap
                        continue;
                    }
                    if (std::find(visited.begin(), visited.end(), calledFunc) == visited.end()) {
                        workList.push_back(calledFunc);
                        visited.push_back(calledFunc);
                        cloneFuncs.push_back(calledFunc);
                    }
                }
            }
        }
    }

    if (mallocCalls.size() != 1) {
        return false;
    }

    std::map<Function*, Function*> cloneMap;

    // Fix the cloned Functions
    // We will handle the callers of the clone later
    for (Function* toCloneFunc: cloneFuncs) {
        llvm::ValueToValueMapTy VMap;
        Function* clonedFunc = llvm::CloneFunction(toCloneFunc, VMap);
        // If the VMap contains the call to malloc, then we can set it right
        // away
        llvm::ValueToValueMapTy::iterator it = VMap.find(mallocCalls[0]);
        if (it != VMap.end()) {
            Value* clonedMallocValue = VMap[mallocCalls[0]];
            //llvm::errs() << *clonedMallocValue << "\n";
            Instruction* clonedMallocInst = SVFUtil::dyn_cast<Instruction>(clonedMallocValue);
            clonedMallocInst->addAnnotationMetadata("ArrayType");
            // CallInst* clonedMalloc = SVFUtil::dyn_cast<CallInst>(&(VMap[mallocCalls[0]]));
            // SVFUtil::dyn_cast<CallInst*>(it->second());
        }
        cloneMap[toCloneFunc] = clonedFunc;
    }

    mallocCalls[0]->addAnnotationMetadata("StructType");
    // Okay, now go over all the original copies of these functions
    // If they had a call to _another_ function that was also cloned, replace
    // them with the code. 
    //

    Function* topLevelClone = cloneMap[func];

    workList.clear();
    workList.push_back(topLevelClone);

    for (auto it: cloneMap) {
        llvm::errs() << "Function " << it.first->getName() << " cloned to " << it.second->getName() << "\n";
    }

    while (!workList.empty()) {
        Function* F = workList.back();
        workList.pop_back();
        for (llvm::inst_iterator I = llvm::inst_begin(F), E = llvm::inst_end(F); I != E; ++I) {
            if (CallInst* CI = SVFUtil::dyn_cast<CallInst>(&*I)) {
                Function* calledFunc = CI->getCalledFunction();
                assert(calledFunc && "We shouldn't have to reach here");
                // If this was cloned
                if (std::find(cloneFuncs.begin(), cloneFuncs.end(), calledFunc) != cloneFuncs.end()) {
                    Function* clonedFunction = cloneMap[calledFunc];
                    CI->setCalledFunction(clonedFunction);
                    llvm::errs() << "Updated CI " << *CI << "\n";
                }
            }
        }
    }
    topClonedFunc = cloneMap[func];
    return true;
}

HeapTypeAnalyzer::HeapTy HeapTypeAnalyzer::getSizeOfTy(Module& module, LLVMContext& Ctx, MDNode* sizeOfTyName, MDNode* sizeOfTyArgNum, MDNode* mulFactor) {
    // Get the type
    MDString* typeNameStr = (MDString*)sizeOfTyName->getOperand(0).get();
    Type* sizeOfTy = nullptr;
    if (typeNameStr->getString() == "scalar_type") {
        return HeapTy::ScalarTy;
    } else {
        MDString* mulFactorStr = (MDString*)mulFactor->getOperand(0).get();
        int mulFactorInt = std::stoi(mulFactorStr->getString().str());
        assert(mulFactorInt > 0 && "The multiplicator must be greater than 1");

        if (mulFactorInt == 1) {
            return HeapTy::StructTy;
        } else {
            return HeapTy::ArrayTy;
        }
    }
    assert(false && "Shouldn't reach here");
    
}

void HeapTypeAnalyzer::deriveHeapAllocationTypes(llvm::Module& module) {
    // Just check all the locations that call the malloc functions 
    // or the pool allocators
    for (Module::iterator MIterator = module.begin(); MIterator != module.end(); MIterator++) {
        if (Function *F = SVFUtil::dyn_cast<Function>(&*MIterator)) {
            for (llvm::inst_iterator I = llvm::inst_begin(F), E = llvm::inst_end(F); I != E; ++I) {
                Instruction* inst = &*I;
                LLVMContext& Ctx = inst->getContext();

                MDNode* sizeOfTyName = inst->getMetadata("sizeOfTypeName");
                MDNode* sizeOfTyArgNum = inst->getMetadata("sizeOfTypeArgNum");
                MDNode* mulFactor = inst->getMetadata("sizeOfMulFactor");

                bool handled = false;
                // Check if this is a call to malloc or pool allocator
                CallInst* callInst = SVFUtil::dyn_cast<CallInst>(inst);
                if (callInst) {
                    Function* calledFunc = callInst->getCalledFunction();
                    if (calledFunc) {
                        if (std::find(memAllocFns.begin(), memAllocFns.end(), calledFunc->getName()) != memAllocFns.end()) {
                            if (calledFunc->getName().startswith("ngx_array_init") ||
                                    calledFunc->getName().startswith("ngx_array_create")) {
                                callInst->addAnnotationMetadata("ArrayType");
                                handled = true;
                            } else {
                                if (!sizeOfTyName) {
                                    llvm::errs() << "No type annotation for heap call: " << *callInst << " in function : " << callInst->getFunction()->getName() << " treating as scalar\n";
                                    callInst->addAnnotationMetadata("IntegerType");
                                    handled = true;
                                    continue;
                                }

                                HeapTy ty = getSizeOfTy(module, Ctx, sizeOfTyName, sizeOfTyArgNum, mulFactor);
                                switch(ty) {
                                    case ScalarTy:
                                        callInst->addAnnotationMetadata("IntegerType");
                                        break;
                                    case StructTy:
                                        callInst->addAnnotationMetadata("StructType");
                                        break;
                                    case ArrayTy:
                                        callInst->addAnnotationMetadata("ArrayType"); 
                                        break;
                                    default:
                                        assert(false && "Shouldn't reach here");
                                }
                                handled = true;
                            }

                        }
                    }
                }
                // If not, check if this has the metadata that we're missing 
                if (sizeOfTyName && !handled) {
                    if (callInst && callInst->getCalledFunction() && callInst->getCalledFunction()->isIntrinsic()) {
                        continue;
                    }
                    llvm::errs() << "Instruction: " << *inst << " has type info but not a heap allocation\n";
                }
            }
        }
    }
}

void HeapTypeAnalyzer::deriveHeapAllocationTypesWithCloning(llvm::Module& module) {

    //StructType* stTy = StructType::getTypeByName(module.getContext(), tyName);
    std::vector<std::string> mallocFunctions;

    mallocFunctions.push_back("malloc");
    mallocFunctions.push_back("calloc");

    typedef std::map<Function*, std::set<Type*>> FunctionSizeOfTypeMapTy;
    FunctionSizeOfTypeMapTy functionSizeOfTypeMap;
    typedef std::map<CallInst*, Type*> MallocSizeOfTypeMapTy;
    MallocSizeOfTypeMapTy mallocSizeOfMap;

    typedef std::map<Function*, std::set<CallInst*>> WrapperTypeCallerTy;
    WrapperTypeCallerTy arrayTyCallers;
    WrapperTypeCallerTy structTyCallers;


    for (Module::iterator MIterator = module.begin(); MIterator != module.end(); MIterator++) {
        if (Function *F = SVFUtil::dyn_cast<Function>(&*MIterator)) {
            if (!F->isDeclaration()) {
                for (llvm::inst_iterator I = llvm::inst_begin(F), E = llvm::inst_end(F); I != E; ++I) {
                    Instruction* inst = &*I;
                    LLVMContext& Ctx = inst->getContext();

                    MDNode* sizeOfTyName = inst->getMetadata("sizeOfTypeName");
                    MDNode* sizeOfTyArgNum = inst->getMetadata("sizeOfTypeArgNum");
                    MDNode* mulFactor = inst->getMetadata("sizeOfMulFactor");
                    if (sizeOfTyName) {
                        CallInst* cInst = SVFUtil::dyn_cast<CallInst>(&*I);
                        if (!cInst) {
                            StoreInst* SI = SVFUtil::dyn_cast<StoreInst>(&*I);
                            assert(SI && "Can only handle Store Insts here");

                            cInst = findCInstFA(SI->getPointerOperand());
                        }
                        if (!cInst) {
                            continue;
                        }
                        Function* calledFunction = cInst->getCalledFunction();
                        if (!calledFunction) {
                            continue;
                        }
                        // Get the type
                        MDString* typeNameStr = (MDString*)sizeOfTyName->getOperand(0).get();
                        Type* sizeOfTy = nullptr;
                        if (typeNameStr->getString() == "scalar_type") {
                            sizeOfTy = IntegerType::get(Ctx, 8);
                        } else {
                            std::string prefix = "struct.";
                            std::string structNameStr = typeNameStr->getString().str();
                            StringRef llvmTypeName(prefix+structNameStr);
                            sizeOfTy = StructType::getTypeByName(module.getContext(), llvmTypeName);
                        }
                        if (!sizeOfTy) {
                            continue;
                        }

                        assert(mulFactor && "Sizeof operator must have a multiplicative factor present");

                        MDString* mulFactorStr = (MDString*)mulFactor->getOperand(0).get();
                        int mulFactorInt = std::stoi(mulFactorStr->getString().str());
                        assert(mulFactorInt > 0 && "The multiplicator must be greater than 1");

                        // Create the type
                        if (mulFactorInt > 1) {
                            sizeOfTy = ArrayType::get(sizeOfTy, mulFactorInt);
                        }
                        
                        if (std::find(mallocFunctions.begin(), mallocFunctions.end(), calledFunction->getName()) == mallocFunctions.end()) {
                            functionSizeOfTypeMap[calledFunction].insert(sizeOfTy);
                            if (SVFUtil::isa<ArrayType>(sizeOfTy)) {
                                arrayTyCallers[calledFunction].insert(cInst); 
                            } else if (SVFUtil::isa<StructType>(sizeOfTy)) {
                                structTyCallers[calledFunction].insert(cInst); 
                            }
                        } else {
                            mallocSizeOfMap[cInst] = sizeOfTy;
                        }
                    }
                }
            }
        }
    }

    for (auto it: functionSizeOfTypeMap) {
        Function* potentialMallocWrapper = it.first;
        std::set<Type*>& types = it.second;
        Type* arrTy = nullptr;
        Type* structTy = nullptr;
        
        llvm::errs() << "Potential malloc wrapper: " << potentialMallocWrapper->getName() << "\n";
        for (Type* type: types) {
            llvm::errs() << "\t" << *type << "\n";
            if (SVFUtil::isa<ArrayType>(type)) {
                arrTy = SVFUtil::dyn_cast<ArrayType>(type);
            } else if (SVFUtil::isa<StructType>(type)) {
                structTy = SVFUtil::dyn_cast<StructType>(type);
            }
        }

        if (arrTy && structTy) {
            // This function is invoked with both structtype and arraytype
            // Try to deep clone it, and specialize it for the 
            Function* clonedFunc = nullptr;
            bool couldSpecialize = deepClone(potentialMallocWrapper, clonedFunc, mallocFunctions, arrTy, structTy);
            // Update the callsites
            // It _might_ be possible we're re-updating something that was
            // already updated inside deepClone, but it _should_ be okay
            // The cloned function is for the array, and the original is for
            // the struct type.

            if (couldSpecialize) {
                for (CallInst* caller: arrayTyCallers[potentialMallocWrapper]) {
                    caller->setCalledFunction(clonedFunc);
                } 
            }
        }
    }

    for (auto it: mallocSizeOfMap) {
        CallInst* cInst = it.first;
        Type* type = it.second;
        if (SVFUtil::isa<ArrayType>(type)) {
            cInst->addAnnotationMetadata("ArrayType"); 
        } else if (SVFUtil::isa<StructType>(type)) {
            cInst->addAnnotationMetadata("StructType");
        } else if (SVFUtil::isa<IntegerType>(type)) {
            cInst->addAnnotationMetadata("IntegerType");
        }
        llvm::errs() << "Call inst in function: " << cInst->getFunction()->getName() << " takes type " << *type << "\n";
    }

}


bool
HeapTypeAnalyzer::runOnModule (Module & module) {
    handleVersions(module);
    removePoolAllocatorBody(module);
    deriveHeapAllocationTypes(module);
    
    std::error_code EC;
    llvm::raw_fd_ostream OS("heap-cloned-module.bc", EC,
            llvm::sys::fs::F_None);
    WriteBitcodeToFile(module, OS);
    OS.flush();

    return false;
}





