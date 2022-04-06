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
#include "llvm/IR/InstIterator.h"

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



void WPAPass::doCFI(Module& M) {

    std::map<Value*, std::vector<Function*>> indCallMap;
    std::vector<llvm::CallInst*> indCallProhibited;

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
                                    indCallMap[callInst].push_back(const_cast<Function*>(tgtFunction));
                                }
                            }
                        }
                        if (!hasTarget) {
                            indCallProhibited.push_back(callInst);
                        }
                    }
                }
            }
        }

    }


    /*
    for (auto const& x : indCallMap)
    {
        llvm::errs() << "tgt: " << x.second.size() << "\n";
    }
    */

    // The block function
    std::vector<Type*> blockTypes;
    llvm::ArrayRef<Type*> blockArr(blockTypes);

    FunctionType* blockCFIFnTy = FunctionType::get(Type::getVoidTy(M.getContext()), blockArr, false);
    Function* blockCFI = Function::Create(blockCFIFnTy, Function::ExternalLinkage, "blockCFI", &M);

    // The checkCFI function
    std::vector<Type*> typeCheckCFI;

    Type* longType = IntegerType::get(M.getContext(), 64);
    Type* intType = IntegerType::get(M.getContext(), 32);
    Type* ptrToLongType = PointerType::get(longType, 0);
    typeCheckCFI.push_back(ptrToLongType);
    typeCheckCFI.push_back(intType);
    typeCheckCFI.push_back(ptrToLongType); // checkCFI(unsigned long* valid_tgts, int len, unsigned long* tgt);

    llvm::ArrayRef<Type*> typeCheckCFIArr(typeCheckCFI);

    FunctionType* checkCFIFnTy = FunctionType::get(Type::getVoidTy(M.getContext()), typeCheckCFIArr, false);
    Function* checkCFI = Function::Create(checkCFIFnTy, Function::ExternalLinkage, "checkCFI", &M);

    for (auto pair: indCallMap) {
        Value* key = pair.first;
        // Need an alloca inst, of type arraytype, with len =
        // indCallMap[key].size()
        int len = pair.second.size();
        ArrayType* checkArrTy = ArrayType::get(ptrToLongType, len);
        // Create an AllocaInst
        CallInst* indCall = SVFUtil::dyn_cast<CallInst>(key);
        assert(indCall && "Should be a call instruction");
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
        for (Function* validTgt: pair.second) {
            tgtVec.push_back(ConstantExpr::getBitCast(validTgt, ptrToLongType));

        }
        llvm::ArrayRef<Constant*> tgtArrRef(tgtVec);
        Constant* validTgtsConstArr = ConstantArray::get(checkArrTy, tgtArrRef);

        GlobalVariable* validTgtsGVar = new GlobalVariable(M, checkArrTy, true, GlobalValue::ExternalLinkage,
                validTgtsConstArr);

        // Cast to ulong*
        Value* arg1 = Builder.CreateBitOrPointerCast(validTgtsGVar, ptrToLongType);
        Value* arg2 = clen;
        Value* arg3 = Builder.CreateBitOrPointerCast(indCall->getCalledOperand(), ptrToLongType);

        Builder.CreateCall(checkCFI, {arg1, arg2, arg3});
    }

    for (CallInst* prohibCall: indCallProhibited) {
        IRBuilder Builder(prohibCall);
        Builder.CreateCall(blockCFI, {});
    }
    /*

    // Finally, set the startCFI
    GlobalVariable* startCFI = new GlobalVariable(M, intType, true, GlobalValue::ExternalLinkage,
            0, "startCFI");

    IRBuilder<> CFIBuilder(wpaPass->getCurrInst());
    Constant* one = ConstantInt::get(intType, 1);
    CFIBuilder.CreateStore(one, startCFI);
    */

}

/*!
 * We start from here
 */
void WPAPass::runOnModule(SVFModule* svfModule)
{
    llvm::Module *module = SVF::LLVMModuleSet::getLLVMModuleSet()->getMainLLVMModule(); 

    if (Options::KaliRunTestDriver) {
        invariantInstrumentationDriver(*module);
    } else {
        for (u32_t i = 0; i<= PointerAnalysis::Default_PTA; i++)
        {
            if (Options::PASelected.isSet(i))
                runPointerAnalysis(svfModule, i);
        }
        assert(!ptaVector.empty() && "No pointer analysis is specified.\n");

        doCFI(*module);
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

/*!
 * Create pointer analysis according to a specified kind and then analyze the module.
 */
void WPAPass::runPointerAnalysis(SVFModule* svfModule, u32_t kind)
{
    llvm::Module *module = SVF::LLVMModuleSet::getLLVMModuleSet()->getMainLLVMModule(); 

	/// Build PAG
	PAGBuilder builder;

    Options::InvariantVGEP = true;
	PAG* pag = builder.build(svfModule);
    
    /*
    for (GetElementPtrInst* gepInst: builder.getVgeps()) {
        llvm::Module* mod = gepInst->getParent()->getParent()->getParent();
        Type* gepSrcTy = gepInst->getResultElementType();
        Value* index = gepInst->getOperand(gepInst->getNumOperands() - 1);
        StructType* stTy = SVFUtil::dyn_cast<StructType>(gepSrcTy);

        // Create a Constant with the size of the struct
        const StructLayout* stL = mod->getDataLayout().getStructLayout(stTy);
        uint64_t size = stL->getSizeInBytes();
        Constant* sizeConstant = ConstantInt::get(IntegerType::get(gepInst->getContext(), 64), size);
        IRBuilder builder(gepInst);
        Value* cmp = builder.CreateICmpUGT(sizeConstant, index);
        Instruction* cmpInst = SVFUtil::dyn_cast<Instruction>(cmp);
        assert(cmpInst && "not cmp inst?");

        // Ah, now split the current basic block
        BasicBlock* headBB = gepInst->getParent();
        Instruction* termInst = llvm::SplitBlockAndInsertIfThen(cmpInst, cmpInst->getNextNode(), false);

        // Insert call to the switch view function
        Function* switchViewFn = mod->getFunction("switch_view");
        if (!switchViewFn) {
            // TODO: for some reason the module gets switched from under me
            // The switch-view function
            llvm::ArrayRef<Type*> switchViewFnTypeArr = {};

            FunctionType* switchViewFnTy = FunctionType::get(Type::getVoidTy(mod->getContext()), switchViewFnTypeArr, false);
            switchViewFn = Function::Create(switchViewFnTy, Function::ExternalLinkage, "switch_view", mod);
        }
        IRBuilder switcherBuilder(termInst);
        switcherBuilder.CreateCall(switchViewFn->getFunctionType(), switchViewFn);
    }
    */

    _pta = new AndersenWaveDiff(pag);
    ptaVector.push_back(_pta);
    _pta->analyze();


    InvariantHandler IHandler(svfModule, module, pag);
    IHandler.handleVGEPInvariants();
    IHandler.handlePWCInvariants();

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
