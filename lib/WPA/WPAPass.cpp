//===- WPAPass.cpp -- Whole program analysis pass------------------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013-2017>  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===-----------------------------------------------------------------------===//

/*
 * @file: WPA.cpp
 * @author: yesen
 * @date: 10/06/2014
 * @version: 1.0
 *
 * @section LICENSE
 *
 * @section DESCRIPTION
 *
 */


#include "Util/Options.h"
#include "Util/SVFModule.h"
#include "MemoryModel/PointerAnalysisImpl.h"
#include "WPA/WPAPass.h"
#include "WPA/Andersen.h"
#include "WPA/AndersenSFR.h"
#include "WPA/FlowSensitive.h"
#include "WPA/FlowSensitiveTBHC.h"
#include "WPA/VersionedFlowSensitive.h"
#include "WPA/TypeAnalysis.h"
#include "WPA/Steensgaard.h"
#include "SVF-FE/PAGBuilder.h"
#include "WPA/InvariantHandler.h"
#include "InsertFunctionSwitch.h"
#include "InsertExecutionSwitch.h"

#include "llvm/IR/InstIterator.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "llvm/Transforms/Utils/FunctionComparator.h"


using namespace SVF;

char WPAPass::ID = 0;

static llvm::RegisterPass<WPAPass> WHOLEPROGRAMPA("wpa",
        "Whole Program Pointer AnalysWPAis Pass");


/*!
 * Destructor
 */
WPAPass::~WPAPass()
{
    PTAVector::const_iterator it = ptaVector.begin();
    PTAVector::const_iterator eit = ptaVector.end();
    for (; it != eit; ++it)
    {
        PointerAnalysis* pta = *it;
        delete pta;
    }
    ptaVector.clear();
}

/*
CallInst* WPAPass::findCorrespondingCallInClone(CallInst* indCall, llvm::Module* clonedModule) {
    int indCallIndex = -1;
    Function* origFunc = indCall->getFunction();
    llvm::StringRef indCallFuncName = origFunc->getName();
    for (inst_iterator I = llvm::inst_begin(origFunc), E = llvm::inst_end(origFunc); I != E; ++I) {
        if (CallInst* callInst = SVFUtil::dyn_cast<CallInst>(&*I)) {
            if (callInst->isIndirectCall()) {
                indCallIndex++;
                if (indCall == callInst) {
                    break;
                }
            }
        }
    }

    int indexIt = -1;
    // Now find that same function and call-index in the cloned
    Function* clonedFunc = clonedModule->getFunction(indCallFuncName);
    for (inst_iterator I = llvm::inst_begin(clonedFunc), E = llvm::inst_end(clonedFunc); I != E; ++I) {
        if (CallInst* callInst = SVFUtil::dyn_cast<CallInst>(&*I)) {
            if (callInst->isIndirectCall()) {
                indexIt++;
                if (indexIt == indCallIndex) {
                    return callInst;
                }
            }
        }
    }
    return nullptr;
}
*/

void WPAPass::collectCFI(Module& M, bool woInv) {
    
    std::map<llvm::CallInst*, std::vector<Function*>>* indCallMap;
    std::vector<llvm::CallInst*>* indCallProhibited;

    if (!woInv) {
        indCallMap = &wInvIndCallMap;
        indCallProhibited = &wInvIndCallProhibited;
    } else {
        indCallMap = &woInvIndCallMap;
        indCallProhibited = &woInvIndCallProhibited;
    }


    PAG* pag = _pta->getPAG();
    for (Module::iterator MIterator = M.begin(); MIterator != M.end(); MIterator++) {
        if (Function* F = SVFUtil::dyn_cast<Function>(&*MIterator)) {
            for (inst_iterator I = llvm::inst_begin(F), E = llvm::inst_end(F); I != E; ++I) {
                if (CallInst* callInst = SVFUtil::dyn_cast<CallInst>(&*I)) {
                    if (callInst->isIndirectCall()) {
                        NodeID callNodePtr = pag->getPAGNode(pag->getValueNode(callInst->getCalledOperand()))->getId(); 
                        const PointsTo& pts = _pta->getPts(callNodePtr);
                        bool hasTarget = false;
                        for (PointsTo::iterator piter = pts.begin(), epiter = pts.end(); piter != epiter; ++piter) {
                            NodeID ptd = *piter;
                            PAGNode* tgtNode = pag->getPAGNode(ptd);
                            if (tgtNode->hasValue()) {
                                if (const Function* tgtFunction = SVFUtil::dyn_cast<Function>(tgtNode->getValue())) {
                                    hasTarget = true;
                                    (*indCallMap)[callInst].push_back(const_cast<Function*>(tgtFunction));
                                }
                            }
                        }
                        if (!hasTarget) {
                            (*indCallProhibited).push_back(callInst);
                        }
                    }
                }
            }
        }

    }
}

void WPAPass::instrumentCFICheck(llvm::CallInst* indCall, std::vector<llvm::Function*>& validTgts) {
    llvm::Module* M = indCall->getParent()->getParent()->getParent();
    Type* longType = IntegerType::get(M->getContext(), 64);
    Type* intType = IntegerType::get(M->getContext(), 32);
    Type* ptrToLongType = PointerType::get(longType, 0);

    // Need an alloca inst, of type arraytype, with len =
    // indCallMap[key].size()
    int len = validTgts.size();
    ArrayType* checkArrTy = ArrayType::get(ptrToLongType, len);
    // Create an AllocaInst
    IRBuilder Builder(indCall);

    // Create the arguments
    // 1. a. Create an array (allocainst) with list of function pointers, each casted
    // to ulong*
    // b. Cast it to a ulong*, this is your first argument
    // 2. Create a i32 from the length, this is your second argument
    // 3. Cast the actual target in callInst->getCalledValue(), this is
    // your last argument

    Constant* clen = ConstantInt::get(intType, len);
    std::vector<Constant*> tgtVec;
    for (Function* validTgt: validTgts) {
        tgtVec.push_back(ConstantExpr::getBitCast(validTgt, ptrToLongType));

    }
    llvm::ArrayRef<Constant*> tgtArrRef(tgtVec);
    Constant* validTgtsConstArr = ConstantArray::get(checkArrTy, tgtArrRef);

    GlobalVariable* validTgtsGVar = new GlobalVariable(*M, checkArrTy, true, GlobalValue::ExternalLinkage,
            validTgtsConstArr);

    // Cast to ulong*
    Value* arg1 = Builder.CreateBitOrPointerCast(validTgtsGVar, ptrToLongType);
    Value* arg2 = clen;
    Value* arg3 = Builder.CreateBitOrPointerCast(indCall->getCalledOperand(), ptrToLongType);

    Builder.CreateCall(checkCFI, {arg1, arg2, arg3});

}

CallInst* WPAPass::findCInstFA(Value* val) {
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
bool WPAPass::deepClone(llvm::Function* func, llvm::Function*& clonedFunc, std::vector<std::string>& mallocFunctions, 
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
                    // If it returns a function pointer
                    PointerType* retPtrTy = SVFUtil::dyn_cast<PointerType>(calledFunc->getReturnType());
                    if (!retPtrTy || !SVFUtil::isa<PointerType>(retPtrTy->getPointerElementType())) {
                        // We don't need to clone this function as it doesn't
                        // return a pointer.
                        //
                        // We only need to clone the path that returns the
                        // heap object created on the heap
                        continue;
                    }
                    if (std::find(visited.begin(), visited.end(), calledFunc) != visited.end()) {
                        workList.push_back(calledFunc);
                        visited.push_back(calledFunc);
                    }
                    cloneFuncs.push_back(calledFunc);
                }
            }
        }
    }

    if (mallocCalls.size() != 1) return false;

    std::map<Function*, Function*> cloneMap;

    // Fix the cloned Functions
    // We will handle the callers of the clone later
    for (Function* toCloneFunc: cloneFuncs) {
        llvm::ValueToValueMapTy VMap;
        Function* clone = llvm::CloneFunction(toCloneFunc, VMap);
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
        cloneMap[toCloneFunc] = clone;
    }

    mallocCalls[0]->addAnnotationMetadata("StructType");
    // Okay, now go over all the original copies of these functions
    // If they had a call to _another_ function that was also cloned, replace
    // them with the code. 
    //

    Function* topLevelClone = cloneMap[func];

    workList.clear();
    workList.push_back(topLevelClone);

    while (!workList.empty()) {
        Function* F = workList.back();
        workList.pop_back();
        for (llvm::inst_iterator I = llvm::inst_begin(F), E = llvm::inst_end(F); I != E; ++I) {
            if (CallInst* CI = SVFUtil::dyn_cast<CallInst>(&*I)) {
                Function* calledFunc = CI->getCalledFunction();
                assert(calledFunc && "We shouldn't have to reach here");
                // If this was cloned
                if (std::find(cloneFuncs.begin(), cloneFuncs.end(), calledFunc) != cloneFuncs.end()) {
                    CI->setCalledFunction(cloneMap[calledFunc]);
                }
            }
        }
    }
    return true;
}

void WPAPass::deriveHeapAllocationTypesWithCloning(llvm::Module& module) {

    //StructType* stTy = StructType::getTypeByName(module.getContext(), tyName);
    std::vector<std::string> mallocFunctions;

    mallocFunctions.push_back("malloc");
    mallocFunctions.push_back("calloc");

    typedef std::map<Function*, std::set<Type*>> FunctionSizeOfTypeMapTy;
    FunctionSizeOfTypeMapTy functionSizeOfTypeMap;
    typedef std::map<CallInst*, Type*> MallocSizeOfTypeMapTy;
    MallocSizeOfTypeMapTy mallocSizeOfMap;

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
        /*
        bool isArray = false;
        bool isStruct = false;
        */
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

/**
 * We can go forward, from a sizeof operator
 * Or backward from a heap allocation call (malloc, etc) to a cast
 * Going backward might have unintended consequences. 
 * How do you know when you stop? void* p = malloc(..); return p;
 *
 * You can track all type-cast operations and track if they type-cast
 * any of the return values
 *
 */
void WPAPass::deriveHeapAllocationTypes(llvm::Module& module) {
    std::vector<CallInst*> callInsts; // functions that return pointers
    std::vector<Instruction*> typeCasts;
    std::map<CallInst*, StructType*> callInstCastToStructs;
    std::map<CallInst*, ArrayType*> callInstCastToArrays;

    std::vector<std::string> mallocCallers;

    mallocCallers.push_back("malloc");
    mallocCallers.push_back("calloc");

    for (Module::iterator MIterator = module.begin(); MIterator != module.end(); MIterator++) {
        if (Function *F = SVFUtil::dyn_cast<Function>(&*MIterator)) {
            std::vector<AllocaInst*> stackVars;
            std::vector<LoadInst*> loadInsts;
            std::vector<StoreInst*> storeInsts;
            if (!F->isDeclaration()) {
                for (llvm::inst_iterator I = llvm::inst_begin(F), E = llvm::inst_end(F); I != E; ++I) {
                    if (CallInst* cInst = SVFUtil::dyn_cast<CallInst>(&*I)) {
                        // Does it return a pointer
                        Function* calledFunc = cInst->getCalledFunction();
                        Type* retTy = cInst->getType();
                        if (retTy->isPointerTy()) {
                            callInsts.push_back(cInst);
                        }
                    }
                }
            }
        }
    }

    // Type-casts
    std::vector<Instruction*> workList;
    for (CallInst* cInst: callInsts) {
        workList.push_back(cInst);
        while (!workList.empty()) {
            Instruction* inst = workList.back();
            workList.pop_back();
            bool term = false;
            for (User* u: inst->users()) {
                Instruction* uInst = SVFUtil::dyn_cast<Instruction>(u);
                if (BitCastInst* bcInst = SVFUtil::dyn_cast<BitCastInst>(u)) {
                    Type* destTy = bcInst->getDestTy();
                    if (destTy->isPointerTy()) {
                        Type* ptrTy = destTy->getPointerElementType();
                        if (StructType* stTy = SVFUtil::dyn_cast<StructType>(ptrTy)) {
                            term = true;
                            callInstCastToStructs[cInst] = stTy;
                        } else if (ArrayType* arrTy = SVFUtil::dyn_cast<ArrayType>(ptrTy)) {
                            term = true;
                            callInstCastToArrays[cInst] = arrTy;
                        }
                    }
                }
                if (!term) {
                    workList.push_back(uInst);
                }
            }
        }
    }

    for (auto pair: callInstCastToStructs) {
        CallInst* cInst = pair.first;
        StructType* stTy = pair.second;
        llvm::errs() << "CallInst in function: " << cInst->getFunction()->getName() << " is cast to struct: " << *stTy << "\n";
        
    }

    for (auto pair: callInstCastToArrays) {
        CallInst* cInst = pair.first;
        ArrayType* arrTy = pair.second;
        llvm::errs() << "CallInst in function: " << cInst->getFunction()->getName() << " is cast to array: " << *arrTy << "\n";
        
    }

}

/*!
 * We start from here
 */
void WPAPass::runOnModule(SVFModule* svfModule)
{
    llvm::Module *module = SVF::LLVMModuleSet::getLLVMModuleSet()->getMainLLVMModule(); 
    deriveHeapAllocationTypesWithCloning(*module);

    if (Options::KaliRunTestDriver) {
        invariantInstrumentationDriver(*module);
    } else {
        for (u32_t i = 0; i<= PointerAnalysis::Default_PTA; i++)
        {
            if (Options::PASelected.isSet(i))
                runPointerAnalysis(svfModule, i);
        }
        assert(!ptaVector.empty() && "No pointer analysis is specified.\n");

    }


    //module->dump();
    std::error_code EC;
    llvm::raw_fd_ostream OS("instrumented-module.bc", EC,
            llvm::sys::fs::F_None);
    WriteBitcodeToFile(*module, OS);
    OS.flush();
}

/*!
 * We start from here
 */
bool WPAPass::runOnModule(Module& module)
{
    SVFModule* svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(module);
    runOnModule(svfModule);
    return false;
}

void WPAPass::invariantInstrumentationDriver(Module& module) {
    PAG* pag = nullptr;
    Andersen* andersen = new Andersen(pag);
    Module::GlobalListType &Globals = module.getGlobalList();
    for (Module::iterator MIterator = module.begin(); MIterator != module.end(); MIterator++) {
        if (Function *F = SVFUtil::dyn_cast<Function>(&*MIterator)) {
            std::vector<AllocaInst*> stackVars;
            std::vector<LoadInst*> loadInsts;
            std::vector<StoreInst*> storeInsts;
            if (!F->isDeclaration()) {
                // Find the AllocaInsts
                for (llvm::inst_iterator I = llvm::inst_begin(F), E = llvm::inst_end(F); I != E; ++I) {
                    if (AllocaInst* stackVar = SVFUtil::dyn_cast<AllocaInst>(&*I)) {
                        if (stackVar->hasName() && stackVar->getName() == "retval") {
                            continue;
                        }
                        stackVars.push_back(stackVar);
                    } else if (LoadInst* loadInst = SVFUtil::dyn_cast<LoadInst>(&*I)) {
                        // Is it a pointer
                        if (SVFUtil::isa<PointerType>(loadInst->getType())) {
                            loadInsts.push_back(loadInst);
                        }
                    } else if (StoreInst* storeInst = SVFUtil::dyn_cast<StoreInst>(&*I)) {
                        if (SVFUtil::isa<PointerType>(storeInst->getType())) {
                            storeInsts.push_back(storeInst);
                        }
                    }
                }
                for (LoadInst* loadInst: loadInsts) {
                    /*
                    for (AllocaInst* stackVar: stackVars) {
                        andersen->instrumentInvariant(loadInst, stackVar);
                        break;
                    }
                    break;
                    */
                    for (GlobalVariable& globalVar: Globals) {
                        if (globalVar.getName() == "myGlobal") {
                            andersen->instrumentInvariant(loadInst, &globalVar);
                        }
                    }
                }
            }
        }
    }
}

void WPAPass::addCFIFunctions(llvm::Module* module) {
   // The block function
    std::vector<Type*> blockTypes;
    llvm::ArrayRef<Type*> blockArr(blockTypes);

    FunctionType* blockCFIFnTy = FunctionType::get(Type::getVoidTy(module->getContext()), blockArr, false);
    Function* blockCFI = Function::Create(blockCFIFnTy, Function::ExternalLinkage, "blockCFI", module);

    // The checkCFI function
    std::vector<Type*> typeCheckCFI;

    Type* longType = IntegerType::get(module->getContext(), 64);
    Type* intType = IntegerType::get(module->getContext(), 32);
    Type* ptrToLongType = PointerType::get(longType, 0);
    typeCheckCFI.push_back(ptrToLongType);
    typeCheckCFI.push_back(intType);
    typeCheckCFI.push_back(ptrToLongType); // checkCFI(unsigned long* valid_tgts, int len, unsigned long* tgt);

    llvm::ArrayRef<Type*> typeCheckCFIArr(typeCheckCFI);

    FunctionType* checkCFIFnTy = FunctionType::get(Type::getVoidTy(module->getContext()), typeCheckCFIArr, false);
    checkCFI = Function::Create(checkCFIFnTy, Function::ExternalLinkage, "checkCFI", module);
}


/*!
 * Create pointer analysis according to a specified kind and then analyze the module.
 */
void WPAPass::runPointerAnalysis(SVFModule* svfModule, u32_t kind)
{
    llvm::Module *module = SVF::LLVMModuleSet::getLLVMModuleSet()->getMainLLVMModule(); 

	/// Build PAG
	PAGBuilder builder;

    llvm::errs() << "Running with invariants turned on\n";
    Options::InvariantVGEP = true;
    Options::InvariantPWC = true;
	PAG* pag = builder.build(svfModule);
    _pta = new AndersenWaveDiff(pag);
    ptaVector.push_back(_pta);
    _pta->analyze();

    collectCFI(*module, false);

    Options::InvariantVGEP = false;
    Options::InvariantPWC = false;
    _pta = new AndersenWaveDiff(pag);
    ptaVector.clear();
    ptaVector.push_back(_pta);
    _pta->analyze();

    collectCFI(*module, true);
    
    // At the end of collectCFI
    // we have two maps inside WPAPass populated
    // the wInvIndCallMap and the woInvIndCallMap
    //
    // Using these, we must build the CFI policies for both the with invariant
    // and without invariant versions.
    // And then have the memory view switcher switch between these
    //
    // We do this as follows:
    // We use the wInvMaps to instrument the module
    // In case of indirect calls where the profile produced by wInvMap and
    // woInvMap differs, we create a clone of the Function and add the pair
    // (the original Function and the cloned Function) to
    // a vector of memory-view-pairs.
    //
    // We instrument the original function according to the wInvMap's profile
    // and the cloned function according to the woInvMap's profile
    //
    // We pass the memory-view-pairs vector the the Memory View Switcher pass
    //

    std::vector<std::pair<Function*, Function*>> memViewPairs;

    addCFIFunctions(module);
    for (auto pair: wInvIndCallMap) {
        llvm::CallInst* callInst = pair.first;
        std::vector<Function*>& wInvTgts = pair.second;
        std::vector<Function*>& woInvTgts = woInvIndCallMap[callInst]; // TODO: does it need to have it?
        if (wInvTgts.size() != woInvTgts.size()) {
            Function* origFunc = callInst->getFunction();
            llvm::ValueToValueMapTy VMap;
            Function* clonedFunc = llvm::CloneFunction(origFunc, VMap);
            llvm::Value* clonedVal = VMap[callInst];
            llvm::CallInst* clonedCallInst = SVFUtil::dyn_cast<llvm::CallInst>(clonedVal);
            instrumentCFICheck(clonedCallInst, woInvTgts);
            memViewPairs.push_back(std::make_pair(origFunc, clonedFunc));
        }

        instrumentCFICheck(callInst, wInvTgts);
    }

    // Now go over all functions 
    // and see if the versions are different

    // Handle the invariants
    InvariantHandler IHandler(svfModule, module, pag);
    IHandler.handleVGEPInvariants();
    IHandler.handlePWCInvariants();

    std::unique_ptr<llvm::InsertFunctionSwitchPass> p1 = std::make_unique<llvm::InsertFunctionSwitchPass>(memViewPairs);
    p1->runOnModule(*module);

    /*
    Options::InvariantVGEP = false;
    PAG::releasePAG();
    PAGBuilder builder2;
    PAG* pag2 = builder2.build(svfModule);

    _pta = new AndersenWaveDiff(pag2);
    ptaVector.push_back(_pta);
    _pta->analyze();
    */

    /*
    /// Initialize pointer analysis.
    switch (kind)
    {
    case PointerAnalysis::Andersen_WPA:
        _pta = new Andersen(pag);
        break;
    case PointerAnalysis::AndersenLCD_WPA:
        _pta = new AndersenLCD(pag);
        break;
    case PointerAnalysis::AndersenHCD_WPA:
        _pta = new AndersenHCD(pag);
        break;
    case PointerAnalysis::AndersenHLCD_WPA:
        _pta = new AndersenHLCD(pag);
        break;
    case PointerAnalysis::AndersenSCD_WPA:
        _pta = new AndersenSCD(pag);
        break;
    case PointerAnalysis::AndersenSFR_WPA:
        _pta = new AndersenSFR(pag);
        break;
    case PointerAnalysis::AndersenWaveDiff_WPA:
        _pta = new AndersenWaveDiff(pag);
        break;
    case PointerAnalysis::AndersenWaveDiffWithType_WPA:
        _pta = new AndersenWaveDiffWithType(pag);
        break;
    case PointerAnalysis::Steensgaard_WPA:
        _pta = new Steensgaard(pag);
        break;
    case PointerAnalysis::FSSPARSE_WPA:
        _pta = new FlowSensitive(pag);
        break;
    case PointerAnalysis::FSTBHC_WPA:
        _pta = new FlowSensitiveTBHC(pag);
        break;
    case PointerAnalysis::VFS_WPA:
        _pta = new VersionedFlowSensitive(pag);
        break;
    case PointerAnalysis::TypeCPP_WPA:
        _pta = new TypeAnalysis(pag);
        break;
    default:
        assert(false && "This pointer analysis has not been implemented yet.\n");
        return;
    }

    ptaVector.push_back(_pta);
    _pta->analyze();
    */

    /*
    if (Options::AnderSVFG)
    {
        SVFGBuilder memSSA(true);
        assert(SVFUtil::isa<AndersenBase>(_pta) && "supports only andersen/steensgaard for pre-computed SVFG");
        SVFG *svfg;
        if (Options::WPAOPTSVFG)
        {
            svfg = memSSA.buildFullSVFG((BVDataPTAImpl*)_pta);
        } else
        {
            svfg = memSSA.buildFullSVFGWithoutOPT((BVDataPTAImpl*)_pta);
        }

        /// support mod-ref queries only for -ander
        if (Options::PASelected.isSet(PointerAnalysis::AndersenWaveDiff_WPA))
            _svfg = svfg;
    }

    if (Options::PrintAliases)
        PrintAliasPairs(_pta);
    */
}

void WPAPass::PrintAliasPairs(PointerAnalysis* pta)
{
    PAG* pag = pta->getPAG();
    for (PAG::iterator lit = pag->begin(), elit = pag->end(); lit != elit; ++lit)
    {
        PAGNode* node1 = lit->second;
        PAGNode* node2 = node1;
        for (PAG::iterator rit = lit, erit = pag->end(); rit != erit; ++rit)
        {
            node2 = rit->second;
            if(node1==node2)
                continue;
            const Function* fun1 = node1->getFunction();
            const Function* fun2 = node2->getFunction();
            AliasResult result = pta->alias(node1->getId(), node2->getId());
            SVFUtil::outs()	<< (result == AliasResult::NoAlias ? "NoAlias" : "MayAlias")
                            << " var" << node1->getId() << "[" << node1->getValueName()
                            << "@" << (fun1==nullptr?"":fun1->getName()) << "] --"
                            << " var" << node2->getId() << "[" << node2->getValueName()
                            << "@" << (fun2==nullptr?"":fun2->getName()) << "]\n";
        }
    }
}

/*!
 * Return alias results based on our points-to/alias analysis
 * TODO: Need to handle PartialAlias and MustAlias here.
 */
AliasResult WPAPass::alias(const Value* V1, const Value* V2)
{

    AliasResult result = llvm::MayAlias;

    PAG* pag = _pta->getPAG();

    /// TODO: When this method is invoked during compiler optimizations, the IR
    ///       used for pointer analysis may been changed, so some Values may not
    ///       find corresponding PAG node. In this case, we only check alias
    ///       between two Values if they both have PAG nodes. Otherwise, MayAlias
    ///       will be returned.
    if (pag->hasValueNode(V1) && pag->hasValueNode(V2))
    {
        /// Veto is used by default
        if (Options::AliasRule.getBits() == 0 || Options::AliasRule.isSet(Veto))
        {
            /// Return NoAlias if any PTA gives NoAlias result
            result = llvm::MayAlias;

            for (PTAVector::const_iterator it = ptaVector.begin(), eit = ptaVector.end();
                    it != eit; ++it)
            {
                if ((*it)->alias(V1, V2) == llvm::NoAlias)
                    result = llvm::NoAlias;
            }
        }
        else if (Options::AliasRule.isSet(Conservative))
        {
            /// Return MayAlias if any PTA gives MayAlias result
            result = llvm::NoAlias;

            for (PTAVector::const_iterator it = ptaVector.begin(), eit = ptaVector.end();
                    it != eit; ++it)
            {
                if ((*it)->alias(V1, V2) == llvm::MayAlias)
                    result = llvm::MayAlias;
            }
        }
    }

    return result;
}

/*!
 * Return mod-ref result of a CallInst
 */
ModRefInfo WPAPass::getModRefInfo(const CallInst* callInst)
{
    assert(Options::PASelected.isSet(PointerAnalysis::AndersenWaveDiff_WPA) && Options::AnderSVFG && "mod-ref query is only support with -ander and -svfg turned on");
    ICFG* icfg = _svfg->getPAG()->getICFG();
    const CallBlockNode* cbn = icfg->getCallBlockNode(callInst);
    return _svfg->getMSSA()->getMRGenerator()->getModRefInfo(cbn);
}

/*!
 * Return mod-ref results of a CallInst to a specific memory location
 */
ModRefInfo WPAPass::getModRefInfo(const CallInst* callInst, const Value* V)
{
    assert(Options::PASelected.isSet(PointerAnalysis::AndersenWaveDiff_WPA) && Options::AnderSVFG && "mod-ref query is only support with -ander and -svfg turned on");
    ICFG* icfg = _svfg->getPAG()->getICFG();
    const CallBlockNode* cbn = icfg->getCallBlockNode(callInst);
    return _svfg->getMSSA()->getMRGenerator()->getModRefInfo(cbn, V);
}

/*!
 * Return mod-ref result between two CallInsts
 */
ModRefInfo WPAPass::getModRefInfo(const CallInst* callInst1, const CallInst* callInst2)
{
    assert(Options::PASelected.isSet(PointerAnalysis::AndersenWaveDiff_WPA) && Options::AnderSVFG && "mod-ref query is only support with -ander and -svfg turned on");
    ICFG* icfg = _svfg->getPAG()->getICFG();
    const CallBlockNode* cbn1 = icfg->getCallBlockNode(callInst1);
    const CallBlockNode* cbn2 = icfg->getCallBlockNode(callInst2);
    return _svfg->getMSSA()->getMRGenerator()->getModRefInfo(cbn1, cbn2);
}
