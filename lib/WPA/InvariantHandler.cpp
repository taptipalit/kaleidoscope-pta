#include <WPA/InvariantHandler.h>
#include "llvm/IR/InstIterator.h"


void InvariantHandler::recordTarget(std::vector<int>& ids, Value* target, Function* instFun) {
    IRBuilder* builder;
    LLVMContext& C = mod->getContext();

    IntegerType* i32Ty = IntegerType::get(C, 32);
    IntegerType* i64Ty = IntegerType::get(C, 64);
    PointerType* i64PtrTy = PointerType::get(i64Ty, 0);
    ArrayType* arrTy = ArrayType::get(i64Ty, ids.size());

    bool shouldReset = false;
    AllocaInst* stackVar = nullptr;

    if (AllocaInst* allocaInst = SVFUtil::dyn_cast<AllocaInst>(target)) {
        builder = new IRBuilder(allocaInst->getNextNode());
        stackVar = allocaInst;
        shouldReset = true;
    } else if (GlobalValue* gvar = SVFUtil::dyn_cast<GlobalValue>(target)) {
        Function* mainFunction = mod->getFunction("main");
        Instruction* inst = mainFunction->getEntryBlock().getFirstNonPHIOrDbg();
        builder = new IRBuilder(inst);
    } else if (CallInst* heapCall = SVFUtil::dyn_cast<CallInst>(target)) {
        builder = new IRBuilder(heapCall->getNextNode());
    } else {
        assert(false && "Not handling stuff here");
    }

    
    std::vector<Constant*> consIdVec;
    for (int i: ids) {
        consIdVec.push_back(ConstantInt::get(i64Ty, i));
    }

    llvm::ArrayRef<Constant*> consIdArr(consIdVec);

    Constant* invariantIdArr = ConstantArray::get(arrTy, consIdArr);
    GlobalVariable* invariantIdArrGvar = new GlobalVariable(*mod, 
            /*Type=*/arrTy,
            /*isConstant=*/true,
            /*Linkage=*/GlobalValue::ExternalLinkage,
            /*Initializer=*/0, // has initializer, specified below
            /*Name=*/"cons id");
    invariantIdArrGvar->setInitializer(invariantIdArr);
    
    Constant* cLen = ConstantInt::get(i64Ty, consIdVec.size());
 
    Value* ptrVal = builder->CreateBitOrPointerCast(target, i64Ty);


    Constant* Zero = ConstantInt::get(C, llvm::APInt(64, 0));
    //Value* firstCons = Builder.CreateGEP(kaliIdArr->getType(), kaliIdArr, Zero);
    llvm::ArrayRef<Value*> zeroIdxs {Zero/*, Zero*/}; // single Zero, because
                                                      // it's not in memory
                                                      // It's a constant
    Value* ptrToInvArray = builder->CreateGEP(arrTy,invariantIdArrGvar, zeroIdxs);

    Value* invariantIdArrPtr = builder->CreateBitOrPointerCast(ptrToInvArray,
            i64PtrTy);
    builder->CreateCall(instFun, {invariantIdArrPtr, cLen, ptrVal});

    if (shouldReset) {
        Function* func = stackVar->getParent()->getParent();
        //std::vector<CallInst*> innerCalls;
        std::vector<ReturnInst*> returns;
        for (inst_iterator I = inst_begin(func), E = inst_end(func); I != E; ++I) {
            if (ReturnInst* ret = SVFUtil::dyn_cast<ReturnInst>(&*I)) {
                returns.push_back(ret);
            }
            /*
            if (CallInst* call = SVFUtil::dyn_cast<CallInst>(&*I)) {
                if (call->getCalledFunction() != instFun) {
                    innerCalls.push_back(call);
                }
            }
            */
        }
        for (ReturnInst* ret: returns) {
            builder->SetInsertPoint(ret); 
            // Reset the Invariant
            builder->CreateCall(instFun, {invariantIdArr, cLen, Constant::getNullValue(i64Ty)});
        }
        /*
        for (CallInst* call: innerCalls) {
            if (call->getCalledFunction() &&
                    call->getCalledFunction()->getName().contains(".dbg")) {
                continue;
            }
            builder->SetInsertPoint(call);
            // Reset the Invariant before the call
            builder->CreateCall(instFun, {idConstant, Constant::getNullValue(i64Ty)});
            // Restore it after the call
            builder->SetInsertPoint(call->getNextNode()); 
            Value* ptrVal = builder->CreateBitOrPointerCast(target, i64Ty);
            builder->CreateCall(instFun, {idConstant, ptrVal});
        }
        */
    }
}

/**
 * Check that the gep can never point to the objects in target
 */
void InvariantHandler::instrumentVGEPInvariant(GetElementPtrInst* gep, std::vector<Value*>& targets,
        std::map<Value*, std::vector<int>>& valueInvariantIdsMap) {
    LLVMContext& C = gep->getContext();
    Type* longType = IntegerType::get(mod->getContext(), 64);
    Type* intType = IntegerType::get(mod->getContext(), 32);
    Type* ptrToLongType = PointerType::get(longType, 0);

    std::vector<int> tgtKaliIDs;
    for (Value* target: targets) {
        int id = -1;
        if (valueToKaliIdMap.find(target) == valueToKaliIdMap.end()) {
            id = kaliInvariantId++;
            valueToKaliIdMap[target] = id;
            kaliIdToValueMap[id] = target;
            valueInvariantIdsMap[target].push_back(id);
            //recordTarget(id, target, vgepPtdRecordFn);
        } else {
            id = valueToKaliIdMap[target];
            valueInvariantIdsMap[target].push_back(id);
        }
        tgtKaliIDs.push_back(id);
    }

    Value* pointer = gep->getPointerOperand();
    
    // Call the ptdTargetCheckFn

    int len = tgtKaliIDs.size();
    Constant* clen = ConstantInt::get(longType, len);

    ArrayType* arrTy = ArrayType::get(longType, len);

    std::vector<Constant*> consIdVec;

    for (int i: tgtKaliIDs) {
        consIdVec.push_back(ConstantInt::get(longType, i));
    }

    llvm::ArrayRef<Constant*> consIdArr(consIdVec);

    Constant* kaliIdArr = ConstantArray::get(arrTy, consIdArr);
    
    GlobalVariable* kaliArrGvar = new GlobalVariable(*mod, 
            /*Type=*/arrTy,
            /*isConstant=*/true,
            /*Linkage=*/GlobalValue::ExternalLinkage,
            /*Initializer=*/0, // has initializer, specified below
            /*Name=*/"cons id");
    kaliArrGvar->setInitializer(kaliIdArr);

    IRBuilder Builder(gep);

    Constant* Zero = ConstantInt::get(C, llvm::APInt(64, 0));
    //Value* firstCons = Builder.CreateGEP(kaliIdArr->getType(), kaliIdArr, Zero);
    llvm::ArrayRef<Constant*> zeroIdxs {Zero, Zero};
    Value* firstCons = ConstantExpr::getGetElementPtr(arrTy, kaliArrGvar, zeroIdxs);

    Value* arg1 = Builder.CreateBitOrPointerCast(pointer, ptrToLongType);

    Value* arg2 = clen;
    Value* arg3 = Builder.CreateBitOrPointerCast(firstCons, ptrToLongType);

    Builder.CreateCall(ptdTargetCheckFn, {arg1, arg2, arg3});
}

void InvariantHandler::handleVGEPInvariants() {
    // We first add the invariant-checks
    //
    // Along the way, we record the mapping of which value contributed to which invariants
    // E.g. if a = alloca int*, is present in 20 vgep invariants with ids 10,11, ..., then
    // we'll have a map: a --> 10, 11, ...
    // We'll use this map to record the value of 'a' for all of these invariants

    std::map<Value*, std::vector<int>> valueInvariantIdsMap;

    for (const GetElementPtrInst* gep: pag->getVarGeps()) {
        std::vector<Value*> targets;
        for (NodeID ptdId: pag->getVarGepPtdMap()[gep]) {
            if (pag->hasPAGNode(ptdId)) {
                PAGNode* pagNode = pag->getPAGNode(ptdId);
                if (pagNode->hasValue()) {
                    Value* ptdValue = const_cast<Value*>(pagNode->getValue());
                    // we will be modifying the value by inserting instrumentation
                    targets.push_back(ptdValue);
                }
            }
        }
        instrumentVGEPInvariant(const_cast<GetElementPtrInst*>(gep), targets, valueInvariantIdsMap);
    }

    // Now that we have instrumented the variant gep instructions themselves,
    // we update the target values to record the values. 

    std::map<Value*, std::vector<int>>::iterator it;
    for (it = valueInvariantIdsMap.begin(); it != valueInvariantIdsMap.end(); it++) {
        Value* value = it->first;
        auto idsVec = it->second;
        recordTarget(idsVec, value, vgepPtdRecordFn);
    }
    
}

void InvariantHandler::handlePWCInvariants() {
    PAG::PWCInvariantIterator it, beg = pag->getPWCInvariants().begin();
    PAG::PWCInvariantIterator end = pag->getPWCInvariants().end();
    Type* longType = IntegerType::get(mod->getContext(), 64);
    Type* intType = IntegerType::get(mod->getContext(), 32);

    int ptrID = 0;
    std::set<Value*> instrumented; // TODO: make this handle different PWCs
    for (it = beg; it != end; it++) {
        CycleID pwcID = it->first;
        // Not all pointer nodes have backing values
        // Let's get the count of the ones that do
        int ptrValCount = it->second.size();
        GetElementPtrInst* nonLoopGep = nullptr;
        std::vector<GetElementPtrInst*> geps;

        for (const Value* vPtr: it->second) {
            if (const GetElementPtrInst* gep = SVFUtil::dyn_cast<GetElementPtrInst>(vPtr)) {
                geps.push_back(const_cast<GetElementPtrInst*>(gep));
                /*
                const BasicBlock* bb = gep->getParent();
                if (!loopInfo->getLoopFor(bb)) {
                    llvm::errs() << "found gep outside loop\n";
                    nonLoopGep = const_cast<GetElementPtrInst*>(gep);
                    break;
                }
                */
            }
        }
        nonLoopGep = geps[0];
        /*
        if (!nonLoopGep) {
            llvm::errs() << "Did not find gep outside loop\n";
            nonLoopGep = geps[0];
        }
        */

        if (std::find(instrumented.begin(), instrumented.end(), nonLoopGep) != instrumented.end()) {
            continue;
        }

        instrumented.insert(nonLoopGep);
        // p = r + 4
        // If the cycle is complete, then the first time r will have the value
        // r'
        // When the cycle is complete it'll have r' + 4
        // This indicates the cycle is complete

        // Invoke updatePWC (pwcId, gepVal) after this gep
        IRBuilder builder(nonLoopGep->getNextNode());
        std::vector<Value*> updatePWCArgsList;
        updatePWCArgsList.push_back(ConstantInt::get(intType, pwcID));
        updatePWCArgsList.push_back(builder.CreateBitOrPointerCast(nonLoopGep, longType));

        llvm::ArrayRef<Value*> updateArgs(updatePWCArgsList);
        FunctionType* fTy = updatePWCFn->getFunctionType();
        builder.CreateCall(fTy, updatePWCFn, updateArgs);

        // Invoke checkPWC (pwcId, gepPtrVal, offset) before this gep

        builder.SetInsertPoint(nonLoopGep);
        Value* gepPtr = nonLoopGep->getPointerOperand();
        std::vector<Value*> checkPWCArgsList;
        checkPWCArgsList.push_back(ConstantInt::get(intType, pwcID));
        checkPWCArgsList.push_back(builder.CreateBitOrPointerCast(gepPtr, longType));
        int totalOffset = computeOffsetInPWC(geps, nonLoopGep);
        checkPWCArgsList.push_back(ConstantInt::get(longType, totalOffset));
        llvm::ArrayRef<Value*> checkPWCArgs(checkPWCArgsList);

        fTy = checkPWCFn->getFunctionType();
        builder.CreateCall(fTy, checkPWCFn, checkPWCArgs); 
    }
}

int InvariantHandler::computeOffsetInPWC(std::vector<GetElementPtrInst*>& geps, GetElementPtrInst* nonLoopGep) {
    llvm::Module* mod = nonLoopGep->getModule();

    int totalOffset = 0;
    for (GetElementPtrInst* gep: geps) {
        llvm::APInt offset(64, 0);
        if (gep->accumulateConstantOffset(mod->getDataLayout(), offset)) {
            totalOffset += offset.getZExtValue();
        }
    }
    return totalOffset;
}

/**
 * Initialization routine for the VGEP invariant
 * Install the ptdTargetCheck function
 * ptdTargetCheck(unsigned long* valid_tgts, long len, unsigned long* tgt);
 */
void InvariantHandler::initVGEPInvariants() {
    Type* voidType = Type::getVoidTy(mod->getContext());
    Type* longType = IntegerType::get(mod->getContext(), 64);
    Type* intType = IntegerType::get(mod->getContext(), 32);

    Type* ptrToLongType = PointerType::get(longType, 0);

    // Install the vgepRecord Routine --> vgepRecordTarget(InvariantID* ids, int len, InvariantVal val)
    // The same invariant value can figure in many invariants
    // We can just update them all at once, instead of adding multiple function calls
    std::vector<Type*> vgepRecordTypes;
    vgepRecordTypes.push_back(ptrToLongType);
    vgepRecordTypes.push_back(longType);
    vgepRecordTypes.push_back(longType);

    llvm::ArrayRef<Type*> vgepRecordTypeArr(vgepRecordTypes);

    FunctionType* vgepRecordFnTy = FunctionType::get(voidType, vgepRecordTypeArr, false);
    vgepPtdRecordFn = Function::Create(vgepRecordFnTy, Function::ExternalLinkage, "vgepRecordTarget", mod);

    svfMod->addFunctionSet(vgepPtdRecordFn);
    // Install the ptdTargetCheck Routine
    // ptdTargetCheck(unsigned long* valid_tgts, long len, unsigned long* tgt);
    std::vector<Type*> ptdTargetCheckType;

    ptdTargetCheckType.push_back(ptrToLongType);
    ptdTargetCheckType.push_back(longType);
    ptdTargetCheckType.push_back(ptrToLongType); 

    llvm::ArrayRef<Type*> ptdTargetCheckTypeArr(ptdTargetCheckType);

    FunctionType* ptdTargetCheckFnTy = FunctionType::get(intType, ptdTargetCheckTypeArr, false);
    ptdTargetCheckFn = Function::Create(ptdTargetCheckFnTy, Function::ExternalLinkage, "ptdTargetCheck", mod);

    svfMod->addFunctionSet(ptdTargetCheckFn);
}

/**
 * Initialization routine for the PWC invariant
 * Install the updateAndCheckPWC function
 * Each cycle consists of a list of pointers that need to be equal to each
 * other for the cycle to hold.
 * int updateAndCheckPWC(int cycleID, int totalInvariants, int invariantId, unsigned long val, int isGep);
 * where, cycleID is the PWC CycleID
 *        invariantID is the NodeID of the pointer involved in the PWC
 *        val is the value of the pointer updated
 * It returns true if the invariant no longer is held
 */
void InvariantHandler::initPWCInvariants() {
    Type* voidType = Type::getVoidTy(mod->getContext());
    Type* intType = IntegerType::get(mod->getContext(), 32);
    Type* longType = IntegerType::get(mod->getContext(), 64);

    std::vector<Type*> updatePWCType;

    updatePWCType.push_back(intType);
    updatePWCType.push_back(longType);

    FunctionType* updatePWCFnTy = FunctionType::get(voidType, {intType, longType}, false);
    updatePWCFn = Function::Create(updatePWCFnTy, Function::ExternalLinkage, "updatePWC", mod);

    svfMod->addFunctionSet(updatePWCFn);

    std::vector<Type*> checkPWCType;

    checkPWCType.push_back(intType);
    checkPWCType.push_back(longType);
    checkPWCType.push_back(longType);

    llvm::ArrayRef<Type*> checkPWCTypeArr(checkPWCType);

    FunctionType* checkFnTy = FunctionType::get(intType, checkPWCTypeArr, false);
    checkPWCFn = Function::Create(checkFnTy, Function::ExternalLinkage, "checkPWC", mod);

    svfMod->addFunctionSet(checkPWCFn);
}

void InvariantHandler::init() { 
    initVGEPInvariants();
    initPWCInvariants();
}
