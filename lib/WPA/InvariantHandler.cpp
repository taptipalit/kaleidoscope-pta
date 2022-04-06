#include <WPA/InvariantHandler.h>

void InvariantHandler::recordTarget(int id, Value* target) {
    IRBuilder* builder;
    LLVMContext& C = mod->getContext();

    Type* voidPtrTy = PointerType::get(Type::getInt8Ty(C), 0);
    IntegerType* i64Ty = IntegerType::get(C, 64);
    GlobalVariable* kaliMapGVar = mod->getGlobalVariable("kaliMap");
    assert(kaliMapGVar && "can't find KaliMap");

    // Get the index into the kaliMap
    Constant* zero = ConstantInt::get(i64Ty, 0);
    Constant* idConstant = ConstantInt::get(i64Ty, id);
    std::vector<Value*> idxVec;
    idxVec.push_back(zero);
    idxVec.push_back(idConstant);
    llvm::ArrayRef<Value*> idxs(idxVec);

    if (AllocaInst* allocaInst = SVFUtil::dyn_cast<AllocaInst>(target)) {
        builder = new IRBuilder(allocaInst->getNextNode());
    } else if (GlobalVariable* gvar = SVFUtil::dyn_cast<GlobalVariable>(target)) {
        Function* mainFunction = mod->getFunction("main");
        Instruction* inst = mainFunction->getEntryBlock().getFirstNonPHIOrDbg();
        builder = new IRBuilder(inst);
    } else {
        assert(false && "Not handling stuff here");
    }

    Value* gepIndexMapEntry = builder->CreateGEP(kaliMapGVar, idxs);
    // Cast the address to void*
    Value* voidPtrAddress = builder->CreateBitCast(target, voidPtrTy);
    // Do the store now!
    StoreInst* storeInst = builder->CreateStore(voidPtrAddress, gepIndexMapEntry);
}

/**
 * Check that the gep can never point to the objects in target
 */
void InvariantHandler::instrumentVGEPInvariant(GetElementPtrInst* gep, std::vector<Value*>& targets) {
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
            recordTarget(id, target);
        } else {
            id = valueToKaliIdMap[target];
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
            /*Linkage=*/GlobalValue::CommonLinkage,
            /*Initializer=*/0, // has initializer, specified below
            /*Name=*/"cons id");
    kaliArrGvar->setInitializer(kaliIdArr);

    IRBuilder Builder(gep);

    Constant* Zero = ConstantInt::get(C, llvm::APInt(64, 0));
    //Value* firstCons = Builder.CreateGEP(kaliIdArr->getType(), kaliIdArr, Zero);
    llvm::ArrayRef<Constant*> zeroIdxs {Zero, Zero};
    Value* firstCons = ConstantExpr::getGetElementPtr(arrTy, kaliArrGvar, zeroIdxs);
    Value* arg1 = Builder.CreateBitOrPointerCast(firstCons, ptrToLongType);
    Value* arg2 = clen;
    Value* arg3 = Builder.CreateBitOrPointerCast(pointer, ptrToLongType);

    Builder.CreateCall(ptdTargetCheckFn, {arg1, arg2, arg3});
}

void InvariantHandler::handleVGEPInvariants() {
    for (const GetElementPtrInst* gep: pag->getVarGeps()) {
        std::vector<Value*> targets;
        for (NodeID ptdId: pag->getVarGepPtdMap()[gep]) {
            PAGNode* pagNode = pag->getPAGNode(ptdId);
            if (pagNode->hasValue()) {
                Value* ptdValue = const_cast<Value*>(pagNode->getValue());
                // we will be modifying the value by inserting instrumentation
                targets.push_back(ptdValue);
            }
        }
        instrumentVGEPInvariant(const_cast<GetElementPtrInst*>(gep), targets);
    }
}

void InvariantHandler::handlePWCInvariants() {
    PAG::PWCInvariantIterator it, beg = pag->getPWCInvariants().begin();
    PAG::PWCInvariantIterator end = pag->getPWCInvariants().end();
    Type* longType = IntegerType::get(mod->getContext(), 64);
    Type* intType = IntegerType::get(mod->getContext(), 32);

    for (it = beg; it != end; it++) {
        CycleID pwcID = it->first;
        // Not all pointer nodes have backing values
        // Let's get the count of the ones that do
        int ptrValCount = 0;
        for (NodeID ptrID: it->second) {
            PAGNode* ptrNode = pag->getPAGNode(ptrID);
            if (ptrNode->hasValue()) {
                ptrValCount++;
            }
        }
        for (NodeID ptrID: it->second) {
            PAGNode* ptrNode = pag->getPAGNode(ptrID);
            if (ptrNode->hasValue()) {
                Value* valPtr = const_cast<Value*>(ptrNode->getValue());
                // Insert a call to record this
                if (Instruction* inst = SVFUtil::dyn_cast<Instruction>(valPtr)) {
                    IRBuilder builder(inst->getNextNode());
                    std::vector<Value*> argsList;
                    argsList.push_back(ConstantInt::get(intType, pwcID));
                    argsList.push_back(ConstantInt::get(intType, ptrValCount));
                    argsList.push_back(ConstantInt::get(intType, ptrID));
                    argsList.push_back(builder.CreateBitOrPointerCast(valPtr, longType));
                    llvm::ArrayRef<Value*> args(argsList);
                    FunctionType* fTy = updateCheckPWCFn->getFunctionType();
                    builder.CreateCall(fTy, updateCheckPWCFn, argsList);
                }
            }
        }
    }
}

/**
 * Initialization routine for the VGEP invariant
 * Install the ptdTargetCheck function
 * ptdTargetCheck(unsigned long* valid_tgts, long len, unsigned long* tgt);
 */
void InvariantHandler::initVGEPInvariants() {
    // Install the ptdTargetCheck Routine
    // ptdTargetCheck(unsigned long* valid_tgts, long len, unsigned long* tgt);
    std::vector<Type*> ptdTargetCheckType;

    Type* longType = IntegerType::get(mod->getContext(), 64);
    Type* intType = IntegerType::get(mod->getContext(), 32);

    Type* ptrToLongType = PointerType::get(longType, 0);
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
 * int updateAndCheckPWC(int cycleID, int totalInvariants, int invariantId, unsigned long val);
 * where, cycleID is the PWC CycleID
 *        invariantID is the NodeID of the pointer involved in the PWC
 *        val is the value of the pointer updated
 * It returns true if the invariant no longer is held
 */
void InvariantHandler::initPWCInvariants() {
    // Install the cycle invariant check
    // Each Cycle has a cycle id, a list of values that can't be equal to each
    // other
    std::vector<Type*> updateAndCheckType;

    Type* intType = IntegerType::get(mod->getContext(), 32);
    Type* longType = IntegerType::get(mod->getContext(), 64);
    //Type* ptrToLongType = PointerType::get(longType, 0);
    updateAndCheckType.push_back(intType);
    updateAndCheckType.push_back(intType);
    updateAndCheckType.push_back(intType);
    updateAndCheckType.push_back(longType); 

    llvm::ArrayRef<Type*> updateAndCheckTypeArr(updateAndCheckType);

    FunctionType* updateAndCheckFnTy = FunctionType::get(intType, updateAndCheckTypeArr, false);
    updateCheckPWCFn = Function::Create(updateAndCheckFnTy, Function::ExternalLinkage, "updateAndCheckPWC", mod);

    svfMod->addFunctionSet(updateCheckPWCFn);
}

void InvariantHandler::init() { 
    initVGEPInvariants();
    initPWCInvariants();
}
