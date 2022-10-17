//===- AndersenStat.cpp -- Statistics of Andersen's analysis------------------//
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
//===----------------------------------------------------------------------===//

/*
 * AndersenStat.cpp
 *
 *  Created on: Oct 12, 2013
 *      Author: Yulei Sui
 */

#include "SVF-FE/LLVMUtil.h"
#include "WPA/WPAStat.h"
#include "WPA/Andersen.h"

using namespace SVF;
using namespace SVFUtil;

u32_t AndersenStat::_MaxPtsSize = 0;
u32_t AndersenStat::_NumOfCycles = 0;
u32_t AndersenStat::_NumOfPWCCycles = 0;
u32_t AndersenStat::_NumOfNodesInCycles = 0;
u32_t AndersenStat::_MaxNumOfNodesInSCC = 0;

const char* AndersenStat::CollapseTime = "CollapseTime";

/*!
 * Constructor
 */
AndersenStat::AndersenStat(AndersenBase* p): PTAStat(p),pta(p)
{
    _NumOfNullPtr = 0;
    _NumOfConstantPtr= 0;
    _NumOfBlackholePtr = 0;
    startClk();
}

/*!
 * Collect cycle information
 */
void AndersenStat::collectCycleInfo(ConstraintGraph* consCG)
{
    _NumOfCycles = 0;
    _NumOfPWCCycles = 0;
    _NumOfNodesInCycles = 0;
    NodeSet repNodes;
    repNodes.clear();
    for(ConstraintGraph::iterator it = consCG->begin(), eit = consCG->end(); it!=eit; ++it)
    {
        // sub nodes have been removed from the constraint graph, only rep nodes are left.
        NodeID repNode = consCG->sccRepNode(it->first);
        NodeBS& subNodes = consCG->sccSubNodes(repNode);
        NodeBS clone = subNodes;
        for (NodeBS::iterator it = subNodes.begin(), eit = subNodes.end(); it != eit; ++it)
        {
            NodeID nodeId = *it;
            if (!pta->getPAG()->hasPAGNode(nodeId)) {
                continue;
            }
            PAGNode* pagNode = pta->getPAG()->getPAGNode(nodeId);
            if (SVFUtil::isa<ObjPN>(pagNode) && pta->isFieldInsensitive(nodeId))
            {
                NodeID baseId = consCG->getBaseObjNode(nodeId);
                clone.reset(nodeId);
                clone.set(baseId);
            }
        }
        u32_t num = clone.count();
        if (num > 1)
        {
            if(repNodes.insert(repNode).second)
            {
                _NumOfNodesInCycles += num;
                if(consCG->isPWCNode(repNode))
                    _NumOfPWCCycles ++;
            }
            if( num > _MaxNumOfNodesInSCC)
                _MaxNumOfNodesInSCC = num;
        }
    }
    _NumOfCycles += repNodes.size();
}

void AndersenStat::constraintGraphStat()
{


    ConstraintGraph* consCG = pta->getConstraintGraph();

    u32_t numOfCopys = 0;
    u32_t numOfGeps = 0;
    // collect copy and gep edges
    for(ConstraintEdge::ConstraintEdgeSetTy::iterator it = consCG->getDirectCGEdges().begin(),
            eit = consCG->getDirectCGEdges().end(); it!=eit; ++it)
    {
        if(SVFUtil::isa<CopyCGEdge>(*it))
            numOfCopys++;
        else if(SVFUtil::isa<GepCGEdge>(*it))
            numOfGeps++;
        else
            assert(false && "what else!!");
    }

    u32_t totalNodeNumber = 0;
    u32_t cgNodeNumber = 0;
    u32_t objNodeNumber = 0;
    u32_t addrtotalIn = 0;
    u32_t addrtotalOut = 0;
    u32_t addrmaxIn = 0;
    u32_t addrmaxOut = 0;
    u32_t copytotalIn = 0;
    u32_t copytotalOut = 0;
    u32_t copymaxIn = 0;
    u32_t copymaxOut = 0;
    u32_t loadtotalIn = 0;
    u32_t loadtotalOut = 0;
    u32_t loadmaxIn = 0;
    u32_t loadmaxOut = 0;
    u32_t storetotalIn = 0;
    u32_t storetotalOut = 0;
    u32_t storemaxIn = 0;
    u32_t storemaxOut = 0;


    NodeID copymaxOutNode = -1;
    NodeID copymaxInNode = -1;
    
    for (ConstraintGraph::ConstraintNodeIDToNodeMapTy::iterator nodeIt = consCG->begin(), nodeEit = consCG->end();
            nodeIt != nodeEit; nodeIt++)
    {
        totalNodeNumber++;
        if(nodeIt->second->getInEdges().empty() && nodeIt->second->getOutEdges().empty())
            continue;
        cgNodeNumber++;
        if (!pta->getPAG()->hasPAGNode(nodeIt->first)) continue;
        if(SVFUtil::isa<ObjPN>(pta->getPAG()->getPAGNode(nodeIt->first)))
            objNodeNumber++;

        u32_t nCopyIn = nodeIt->second->getDirectInEdges().size();
        if(nCopyIn > copymaxIn) {
            copymaxIn = nCopyIn;
            copymaxInNode = nodeIt->first;
        }
        copytotalIn +=nCopyIn;
        u32_t nCopyOut = nodeIt->second->getDirectOutEdges().size();
        if(nCopyOut > copymaxOut) {
            copymaxOut = nCopyOut;
            copymaxOutNode = nodeIt->first;
        }
        copytotalOut +=nCopyOut;
        u32_t nLoadIn = nodeIt->second->getLoadInEdges().size();
        if(nLoadIn > loadmaxIn)
            loadmaxIn = nLoadIn;
        loadtotalIn +=nLoadIn;
        u32_t nLoadOut = nodeIt->second->getLoadOutEdges().size();
        if(nLoadOut > loadmaxOut)
            loadmaxOut = nLoadOut;
        loadtotalOut +=nLoadOut;
        u32_t nStoreIn = nodeIt->second->getStoreInEdges().size();
        if(nStoreIn > storemaxIn)
            storemaxIn = nStoreIn;
        storetotalIn +=nStoreIn;
        u32_t nStoreOut = nodeIt->second->getStoreOutEdges().size();
        if(nStoreOut > storemaxOut)
            storemaxOut = nStoreOut;
        storetotalOut +=nStoreOut;
        u32_t nAddrIn = nodeIt->second->getAddrInEdges().size();
        if(nAddrIn > addrmaxIn)
            addrmaxIn = nAddrIn;
        addrtotalIn +=nAddrIn;
        u32_t nAddrOut = nodeIt->second->getAddrOutEdges().size();
        if(nAddrOut > addrmaxOut)
            addrmaxOut = nAddrOut;
        addrtotalOut +=nAddrOut;
    }
    double storeavgIn = (double)storetotalIn/cgNodeNumber;
    double loadavgIn = (double)loadtotalIn/cgNodeNumber;
    double copyavgIn = (double)copytotalIn/cgNodeNumber;
    double addravgIn = (double)addrtotalIn/cgNodeNumber;
    double avgIn = (double)(addrtotalIn + copytotalIn + loadtotalIn + storetotalIn)/cgNodeNumber;


    PTNumStatMap["NumOfCGNode"] = totalNodeNumber;
    PTNumStatMap["TotalValidNode"] = cgNodeNumber;
    PTNumStatMap["TotalValidObjNode"] = objNodeNumber;
    PTNumStatMap["NumOfCGEdge"] = consCG->getLoadCGEdges().size() + consCG->getStoreCGEdges().size()
                                  + numOfCopys + numOfGeps;
    PTNumStatMap["NumOfAddrs"] =  consCG->getAddrCGEdges().size();
    PTNumStatMap["NumOfCopys"] = numOfCopys;
    PTNumStatMap["NumOfGeps"] =  numOfGeps;
    PTNumStatMap["NumOfLoads"] = consCG->getLoadCGEdges().size();
    PTNumStatMap["NumOfStores"] = consCG->getStoreCGEdges().size();
    PTNumStatMap["MaxInCopyEdge"] = copymaxIn;
    PTNumStatMap["MaxOutCopyEdge"] = copymaxOut;
    PTNumStatMap["MaxInLoadEdge"] = loadmaxIn;
    PTNumStatMap["MaxOutLoadEdge"] = loadmaxOut;
    PTNumStatMap["MaxInStoreEdge"] = storemaxIn;
    PTNumStatMap["MaxOutStoreEdge"] = storemaxOut;
    PTNumStatMap["AvgIn/OutStoreEdge"] = storeavgIn;
    PTNumStatMap["MaxInAddrEdge"] = addrmaxIn;
    PTNumStatMap["MaxOutAddrEdge"] = addrmaxOut;
    timeStatMap["AvgIn/OutCopyEdge"] = copyavgIn;
    timeStatMap["AvgIn/OutLoadEdge"] = loadavgIn;
    timeStatMap["AvgIn/OutAddrEdge"] = addravgIn;
    timeStatMap["AvgIn/OutEdge"] = avgIn;

    if (copymaxOutNode != -1) {
        PAGNode* outNode = pta->getPAG()->getPAGNode(copymaxOutNode);
        if (outNode->hasValue()) {
            const Value* val = outNode->getValue();
            if (const Instruction* inst = SVFUtil::dyn_cast<Instruction>(val)) {
                llvm::errs() << "Copy max out value: " << *inst << " in " << inst->getParent()->getParent()->getName() << "\n";
            } else {
                llvm::errs() << "Copy max out value: " << *val << "\n";
            }
        } else {
            llvm::errs() << "Dummy node of type: " << outNode->getNodeKind() << "\n";
        }
    }
    if (copymaxInNode != -1) {
        PAGNode* inNode = pta->getPAG()->getPAGNode(copymaxInNode);
        if (inNode->hasValue()) {
            const Value* val = inNode->getValue();
            if (const Instruction* inst = SVFUtil::dyn_cast<Instruction>(val)) {
                llvm::errs() << "Copy max in value: " << *inst << " in " << inst->getParent()->getParent()->getName() << "\n";
            } else {
                llvm::errs() << "Copy max in value: " << *val << "\n";
            }
        } else {
            llvm::errs() << "Dummy node of type: " << inNode->getNodeKind() << "\n";
        }
    }
    PTAStat::printStat("Constraint Graph Stats");
}
/*!
 * Stat null pointers
 */
void AndersenStat::statNullPtr()
{

    _NumOfNullPtr = 0;
    for (PAG::iterator iter = pta->getPAG()->begin(), eiter = pta->getPAG()->end();
            iter != eiter; ++iter)
    {
        NodeID pagNodeId = iter->first;
        PAGNode* pagNode = iter->second;
        if (pagNode->isTopLevelPtr() == false)
            continue;
        PAGEdge::PAGEdgeSetTy& inComingStore = pagNode->getIncomingEdges(PAGEdge::Store);
        PAGEdge::PAGEdgeSetTy& outGoingLoad = pagNode->getOutgoingEdges(PAGEdge::Load);
        if (inComingStore.empty()==false || outGoingLoad.empty()==false)
        {
            ///TODO: change the condition here to fetch the points-to set
            const PointsTo& pts = pta->getPts(pagNodeId);
            if (pta->containBlackHoleNode(pts))
                _NumOfBlackholePtr++;

            if (pta->containConstantNode(pts))
                _NumOfConstantPtr++;

            if(pts.empty())
            {
                std::string str;
                raw_string_ostream rawstr(str);
                if (!SVFUtil::isa<DummyValPN>(pagNode) && !SVFUtil::isa<DummyObjPN>(pagNode) )
                {
                    // if a pointer is in dead function, we do not care
                    if(isPtrInDeadFunction(pagNode->getValue()) == false)
                    {
                        _NumOfNullPtr++;
                        rawstr << "##Null Pointer : (NodeID " << pagNode->getId()
                               << ") PtrName:" << pagNode->getValue()->getName();
                        writeWrnMsg(rawstr.str());
                        //pagNode->getValue()->dump();
                    }
                }
                else
                {
                    _NumOfNullPtr++;
                    rawstr << "##Null Pointer : (NodeID " << pagNode->getId() << ")";
                    writeWrnMsg(rawstr.str());
                }
            }
        }
    }

}

void AndersenStat::filter(std::set<NodeID>& fiPts, const PointsTo& pts) {
    ConstraintGraph* consCG = pta->getConstraintGraph();
    PAG* pag = pta->getPAG();

    for (PointsTo::iterator piter = pts.begin(), epiter = pts.end(); 
            piter != epiter; ++piter) {
        NodeID ptd = *piter;
        if (pag->hasPAGNode(ptd)) {
            fiPts.insert(consCG->getBaseObjNode(ptd));
        } else {
            fiPts.insert(ptd);
        }
    } 
}

/*!
 * Start here
 */
void AndersenStat::performStat()
{

    assert(SVFUtil::isa<AndersenBase>(pta) && "not an andersen pta pass!! what else??");
    endClk();

    PAG* pag = pta->getPAG();
    ConstraintGraph* consCG = pta->getConstraintGraph();

    // collect constraint graph cycles
    collectCycleInfo(consCG);

    // stat null ptr number
    statNullPtr();

    u32_t totalPointers = 0;
    u32_t totalTopLevPointers = 0;
    u32_t totalPtsSize = 0;
    u32_t totalTopLevPtsSize = 0;


    u32_t totalFieldPtsSize = 0; // treat each field as distinct
    u32_t maxFieldPtsSize = 0;

    if (Options::DumpCFIStat) {
        SVFModule* svfModule = pag->getModule();
        std::map<llvm::CallInst*, std::set<Function*>> indCallMap;
        std::map<int, int> histogram;

        for (auto it = svfModule->llvmFunBegin(), eit = svfModule->llvmFunEnd(); it != eit; it++) {
            Function* F = *it;
            llvm::errs() << "Function: " << F->getName() << "\n";
            for (inst_iterator I = llvm::inst_begin(F), E = llvm::inst_end(F); I != E; ++I) {
                if (CallInst* callInst = SVFUtil::dyn_cast<CallInst>(&*I)) {
                    if (callInst->isIndirectCall()) {
                        NodeID callNodePtr = pag->getPAGNode(pag->getValueNode(callInst->getCalledOperand()))->getId(); 
                        const PointsTo& pts = pta->getPts(callNodePtr);
                        bool hasTarget = false;

                        llvm::errs() << "For a callsite in " << F->getName() << " : \n";
                        for (PointsTo::iterator piter = pts.begin(), epiter = pts.end(); piter != epiter; ++piter) {
                            NodeID ptd = *piter;
                            if (pag->hasPAGNode(ptd)) {
                                PAGNode* tgtNode = pag->getPAGNode(ptd);
                                if (tgtNode->hasValue()) {
                                    if (const Function* tgtFunction = SVFUtil::dyn_cast<Function>(tgtNode->getValue())) {
                                        hasTarget = true;
                                        indCallMap[callInst].insert(const_cast<Function*>(tgtFunction));
                                        llvm::errs() << "    calls " << tgtFunction->getName() << "\n";
                                    }
                                }
                            }
                        }
                        if (!hasTarget) {
                            llvm::errs() << "   in " << F->getName() << " NO TARGET FOUND\n";
                        }
                    }
                }
            }
        }

        for (auto it: indCallMap) {
            CallInst* cInst = it.first;
            int sz = it.second.size();
            histogram[sz]++;
        }

        llvm::errs() << "EC Size:\t Ind. Call-sites\n";
        int totalTgts = 0;
        int totalIndCallSites = 0;
        for (auto it: histogram) {
            llvm::errs() << it.first << " : " << it.second << "\n";
            totalIndCallSites += it.second;
            totalTgts += it.first*it.second;
        }
        llvm::errs() << "Total Ind. Call-sites: " << totalIndCallSites << "\n";
        llvm::errs() << "Total Tgts: " << totalTgts << "\n";
        llvm::errs() << "Average CFI: " << std::to_string( (float)totalTgts / (float)totalIndCallSites) << "\n";
    }

    NodeID maxPtsNodeID = -1;
    int maxFields = 0;
    NodeID maxFieldsNodeID = 0;
    for (PAG::iterator iter = pta->getPAG()->begin(), eiter = pta->getPAG()->end();
            iter != eiter; ++iter)
    {
        NodeID node = iter->first;
        const PointsTo& pts = pta->getPts(node);
        // Changing how we collect stats
        std::set<NodeID> fiPts;
        filter(fiPts, pts);
        u32_t size = fiPts.size();
        //u32_t size = pts.count();
        totalPointers++;
        totalPtsSize+=size;
        totalFieldPtsSize+= pts.count();

        if(pta->getPAG()->isValidTopLevelPtr(pta->getPAG()->getPAGNode(node)))
        {
            totalTopLevPointers++;
            totalTopLevPtsSize+=size;
        }

        if(size > _MaxPtsSize )
            _MaxPtsSize = size;

        if (pts.count() > maxFieldPtsSize) {
            maxFieldPtsSize = pts.count();
            maxPtsNodeID = node;
        }
        if (pag->hasPAGNode(node)) {
            PAGNode* pagNode = pag->getPAGNode(node);
            if (SVFUtil::isa<ObjPN>(pagNode)) {
                NodeBS& fieldSize = consCG->getAllFieldsObjNode(node);
                if (fieldSize.count() > maxFields) {
                    maxFields = fieldSize.count();
                    maxFieldsNodeID = node;
                }
            }
        }
    }

    PAGNode* maxPtsNode = pag->getPAGNode(maxPtsNodeID);
    if (maxPtsNode->hasValue()) {
        if (const Function* maxPtsFunc = SVFUtil::dyn_cast<Function>(maxPtsNode->getValue())) {
            llvm::errs() << "Max pts node: " << maxPtsFunc->getName() << "\n";
        }
        llvm::errs() << "Max pts node: " << *(maxPtsNode->getValue()) << " " << (maxPtsNode->getFunction()? maxPtsNode->getFunction()->getName() : "") << "\n";
    } else {
        llvm::errs() << "Max pts node: " << *maxPtsNode << "\n";
    }

    if (maxFieldsNodeID > 0 && pag->hasPAGNode(maxFieldsNodeID)) {
        PAGNode* maxFieldsNode = pag->getPAGNode(maxFieldsNodeID);
        if (maxFieldsNode->hasValue()) {
            llvm::errs() << "Max fields node: " << *(maxFieldsNode->getValue()) << " ... max fields: " << maxFields << "\n";
        } else {
            llvm::errs() << "Max fields node: " << *maxFieldsNode << " ... max fields: " << maxFields << "\n";
        }
    }


    PTAStat::performStat();

    constraintGraphStat();

    timeStatMap[TotalAnalysisTime] = (endTime - startTime)/TIMEINTERVAL;
    timeStatMap[SCCDetectionTime] = Andersen::timeOfSCCDetection;
    timeStatMap[SCCMergeTime] =  Andersen::timeOfSCCMerges;
    timeStatMap[CollapseTime] =  Andersen::timeOfCollapse;

    timeStatMap[ProcessLoadStoreTime] =  Andersen::timeOfProcessLoadStore;
    timeStatMap[ProcessCopyGepTime] =  Andersen::timeOfProcessCopyGep;
    timeStatMap[UpdateCallGraphTime] =  Andersen::timeOfUpdateCallGraph;

    PTNumStatMap[TotalNumOfPointers] = pag->getValueNodeNum() + pag->getFieldValNodeNum();
    PTNumStatMap[TotalNumOfObjects] = pag->getObjectNodeNum() + pag->getFieldObjNodeNum();


    PTNumStatMap[NumOfProcessedAddrs] = Andersen::numOfProcessedAddr;
    PTNumStatMap[NumOfProcessedCopys] = Andersen::numOfProcessedCopy;
    PTNumStatMap[NumOfProcessedGeps] = Andersen::numOfProcessedGep;
    PTNumStatMap[NumOfProcessedLoads] = Andersen::numOfProcessedLoad;
    PTNumStatMap[NumOfProcessedStores] = Andersen::numOfProcessedStore;

    PTNumStatMap[NumOfSfr] = Andersen::numOfSfrs;
    PTNumStatMap[NumOfFieldExpand] = Andersen::numOfFieldExpand;

    PTNumStatMap[NumOfPointers] = pag->getValueNodeNum();
    PTNumStatMap[NumOfMemObjects] = pag->getObjectNodeNum();
    PTNumStatMap[NumOfGepFieldPointers] = pag->getFieldValNodeNum();
    PTNumStatMap[NumOfGepFieldObjects] = pag->getFieldObjNodeNum();

    timeStatMap[AveragePointsToSetSize] = (double)totalPtsSize/totalPointers;;
    timeStatMap[AveragePointsToSetSizeFields] = (double)totalFieldPtsSize/totalPointers;;

    timeStatMap[AverageTopLevPointsToSetSize] = (double)totalTopLevPtsSize/totalTopLevPointers;;

    timeStatMap[MaxPointsToSetSizeFields] = (double)maxFieldPtsSize;


    PTNumStatMap[MaxPointsToSetSize] = _MaxPtsSize;

    PTNumStatMap[NumOfIterations] = pta->numOfIteration;

    PTNumStatMap[NumOfIndirectCallSites] = consCG->getIndirectCallsites().size();
    PTNumStatMap[NumOfIndirectEdgeSolved] = pta->getNumOfResolvedIndCallEdge();

    PTNumStatMap[NumOfSCCDetection] = Andersen::numOfSCCDetection;
    PTNumStatMap[NumOfCycles] = _NumOfCycles;
    PTNumStatMap[NumOfPWCCycles] = _NumOfPWCCycles;
    PTNumStatMap[NumOfNodesInCycles] = _NumOfNodesInCycles;
    PTNumStatMap[MaxNumOfNodesInSCC] = _MaxNumOfNodesInSCC;
    PTNumStatMap[NumOfNullPointer] = _NumOfNullPtr;
    PTNumStatMap["PointsToConstPtr"] = _NumOfConstantPtr;
    PTNumStatMap["PointsToBlkPtr"] = _NumOfBlackholePtr;

    PTAStat::printStat("Andersen Pointer Analysis Stats");
}

