//===- Andersen.cpp -- Field-sensitive Andersen's analysis-------------------//
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
 * Andersen.cpp
 *
 *  Created on: Nov 12, 2013
 *      Author: Yulei Sui
 */

#include "Util/Options.h"
#include "SVF-FE/LLVMUtil.h"
#include "WPA/Andersen.h"

using namespace SVF;
using namespace SVFUtil;


Size_t AndersenBase::numOfProcessedAddr = 0;
Size_t AndersenBase::numOfProcessedCopy = 0;
Size_t AndersenBase::numOfProcessedGep = 0;
Size_t AndersenBase::numOfProcessedLoad = 0;
Size_t AndersenBase::numOfProcessedStore = 0;
Size_t AndersenBase::numOfSfrs = 0;
Size_t AndersenBase::numOfFieldExpand = 0;

Size_t AndersenBase::numOfSCCDetection = 0;
double AndersenBase::timeOfSCCDetection = 0;
double AndersenBase::timeOfSCCMerges = 0;
double AndersenBase::timeOfCollapse = 0;

Size_t AndersenBase::AveragePointsToSetSize = 0;
Size_t AndersenBase::MaxPointsToSetSize = 0;
double AndersenBase::timeOfProcessCopyGep = 0;
double AndersenBase::timeOfProcessLoadStore = 0;
double AndersenBase::timeOfUpdateCallGraph = 0;


/*!
 * Initilize analysis
 */
void AndersenBase::initialize()
{
    /// Build PAG
    PointerAnalysis::initialize();
    /// Build Constraint Graph
    consCG = new ConstraintGraph(pag);
    setGraph(consCG);
    /// Create statistic class
    stat = new AndersenStat(this);
	if (Options::ConsCGDotGraph)
		consCG->dump("consCG_initial");
}

/*!
 * Finalize analysis
 */
void AndersenBase::finalize()
{
    /// dump constraint graph if PAGDotGraph flag is enabled
	if (Options::ConsCGDotGraph) {
        // Compute the max edge width (max times a edge is traverse)
        consCG->computeMaxEdgeWidth();
		consCG->dump("consCG_final");
    }

	if (Options::PrintCGGraph)
		consCG->print();
    BVDataPTAImpl::finalize();
}

/*!
 * Andersen analysis
 */
void AndersenBase::analyze()
{
    /// Initialization for the Solver
    initialize();

    bool readResultsFromFile = false;
    if(!Options::ReadAnder.empty()) {
        readResultsFromFile = this->readFromFile(Options::ReadAnder);
        // Finalize the analysis
        PointerAnalysis::finalize();
    }

    if(!readResultsFromFile)
    {
        // Start solving constraints
        DBOUT(DGENERAL, outs() << SVFUtil::pasMsg("Start Solving Constraints\n"));

        bool limitTimerSet = SVFUtil::startAnalysisLimitTimer(Options::AnderTimeLimit);

        initWorklist();
        do
        {
            numOfIteration++;
            if (0 == numOfIteration % iterationForPrintStat)
                printStat();

            reanalyze = false;

            solveWorklist();

            if (updateCallGraph(getIndirectCallsites()))
                reanalyze = true;

        }
        while (reanalyze);

        // Analysis is finished, reset the alarm if we set it.
        SVFUtil::stopAnalysisLimitTimer(limitTimerSet);

        DBOUT(DGENERAL, outs() << SVFUtil::pasMsg("Finish Solving Constraints\n"));

        // Finalize the analysis
        finalize();
    }

    if (!Options::WriteAnder.empty())
        this->writeToFile(Options::WriteAnder);
}

void AndersenBase::cleanConsCG(NodeID id) {
    consCG->resetSubs(consCG->getRep(id));
    for (NodeID sub: consCG->getSubs(id))
        consCG->resetRep(sub);
    consCG->resetSubs(id);
    consCG->resetRep(id);
}

void AndersenBase::normalizePointsTo()
{
    PAG::MemObjToFieldsMap &memToFieldsMap = pag->getMemToFieldsMap();
    PAG::NodeLocationSetMap &GepObjNodeMap = pag->getGepObjNodeMap();

    // clear GepObjNodeMap/memToFieldsMap/nodeToSubsMap/nodeToRepMap
    // for redundant gepnodes and remove those nodes from pag
    for (NodeID n: redundantGepNodes) {
        NodeID base = pag->getBaseObjNode(n);
        GepObjPN *gepNode = SVFUtil::dyn_cast<GepObjPN>(pag->getPAGNode(n));
        assert(gepNode && "Not a gep node in redundantGepNodes set");
        const LocationSet ls = gepNode->getLocationSet();
        GepObjNodeMap.erase(std::make_pair(base, ls));
        memToFieldsMap[base].reset(n);
        cleanConsCG(n);

        pag->removeGNode(gepNode);
    }
}

/*!
 * Initilize analysis
 */
void Andersen::initialize()
{
    resetData();
    setDiffOpt(Options::PtsDiff);
    setPWCOpt(Options::MergePWC);
    AndersenBase::initialize();
    /// Initialize worklist
    processAllAddr();
}

/*!
 * Finalize analysis
 */
void Andersen::finalize()
{
    /// sanitize field insensitive obj
    /// TODO: Fields has been collapsed during Andersen::collapseField().
    //	sanitizePts();
	AndersenBase::finalize();
}

/*!
 * Start constraint solving
 */
void Andersen::processNode(NodeID nodeId)
{
    // sub nodes do not need to be processed
    if (sccRepNode(nodeId) != nodeId)
        return;

    ConstraintNode* node = consCG->getConstraintNode(nodeId);
    double insertStart = stat->getClk();
    handleLoadStore(node);
    double insertEnd = stat->getClk();
    timeOfProcessLoadStore += (insertEnd - insertStart) / TIMEINTERVAL;

    double propStart = stat->getClk();
    handleCopyGep(node);
    double propEnd = stat->getClk();
    timeOfProcessCopyGep += (propEnd - propStart) / TIMEINTERVAL;
}

/*!
 * Process copy and gep edges
 */
void Andersen::handleCopyGep(ConstraintNode* node)
{
    NodeID nodeId = node->getId();
    computeDiffPts(nodeId);

    if (!getDiffPts(nodeId).empty())
    {
        for (ConstraintEdge* edge : node->getCopyOutEdges())
            processCopy(nodeId, edge);
        for (ConstraintEdge* edge : node->getGepOutEdges())
        {
            if (GepCGEdge* gepEdge = SVFUtil::dyn_cast<GepCGEdge>(edge))
                processGep(nodeId, gepEdge);
        }
    }
}

/*!
 * Process load and store edges
 */
void Andersen::handleLoadStore(ConstraintNode *node)
{
    NodeID nodeId = node->getId();
    for (PointsTo::iterator piter = getPts(nodeId).begin(), epiter =
                getPts(nodeId).end(); piter != epiter; ++piter)
    {
        NodeID ptd = *piter;
        // handle load
        for (ConstraintNode::const_iterator it = node->outgoingLoadsBegin(),
                eit = node->outgoingLoadsEnd(); it != eit; ++it)
        {
            if (processLoad(ptd, *it))
                pushIntoWorklist(ptd);
        }

        // handle store
        for (ConstraintNode::const_iterator it = node->incomingStoresBegin(),
                eit = node->incomingStoresEnd(); it != eit; ++it)
        {
            if (processStore(ptd, *it))
                pushIntoWorklist((*it)->getSrcID());
        }
    }
}

/*!
 * Process address edges
 */
void Andersen::processAllAddr()
{
    for (ConstraintGraph::const_iterator nodeIt = consCG->begin(), nodeEit = consCG->end(); nodeIt != nodeEit; nodeIt++)
    {
        ConstraintNode * cgNode = nodeIt->second;
        for (ConstraintNode::const_iterator it = cgNode->incomingAddrsBegin(), eit = cgNode->incomingAddrsEnd();
                it != eit; ++it)
            processAddr(SVFUtil::cast<AddrCGEdge>(*it));
    }
}

/*!
 * Process address edges
 */
void Andersen::processAddr(const AddrCGEdge* addr)
{
    numOfProcessedAddr++;

    NodeID dst = addr->getDstID();
    NodeID src = addr->getSrcID();
    if(addPts(dst,src))
        pushIntoWorklist(dst);
}

/*!
 * Process load edges
 *	src --load--> dst,
 *	node \in pts(src) ==>  node--copy-->dst
 */
bool Andersen::processLoad(NodeID node, const ConstraintEdge* load)
{
    /// TODO: New copy edges are also added for black hole obj node to
    ///       make gcc in spec 2000 pass the flow-sensitive analysis.
    ///       Try to handle black hole obj in an appropiate way.
//	if (pag->isBlkObjOrConstantObj(node) || isNonPointerObj(node))
    if (pag->isConstantObj(node) || isNonPointerObj(node))
        return false;

    numOfProcessedLoad++;
    (const_cast<ConstraintEdge*>(load))->traverseCount++;

    NodeID dst = load->getDstID();
    return addCopyEdge(node, dst, load->getSrcID(), dst);
}

/*!
 * Process store edges
 *	src --store--> dst,
 *	node \in pts(dst) ==>  src--copy-->node
 */
bool Andersen::processStore(NodeID node, const ConstraintEdge* store)
{
    /// TODO: New copy edges are also added for black hole obj node to
    ///       make gcc in spec 2000 pass the flow-sensitive analysis.
    ///       Try to handle black hole obj in an appropiate way
//	if (pag->isBlkObjOrConstantObj(node) || isNonPointerObj(node))
    if (pag->isConstantObj(node) || isNonPointerObj(node))
        return false;

    (const_cast<ConstraintEdge*>(store))->traverseCount++;
    numOfProcessedStore++;

    NodeID src = store->getSrcID();
    return addCopyEdge(src, node, src, store->getDstID());
}

/*!
 * Process copy edges
 *	src --copy--> dst,
 *	union pts(dst) with pts(src)
 */
bool Andersen::processCopy(NodeID node, const ConstraintEdge* edge)
{
    numOfProcessedCopy++;
    (const_cast<ConstraintEdge*>(edge))->traverseCount++;

    assert((SVFUtil::isa<CopyCGEdge>(edge)) && "not copy/call/ret ??");
    NodeID dst = edge->getDstID();
    const PointsTo& srcPts = getDiffPts(node);

    bool changed = unionPts(dst, srcPts);
    if (changed)
        pushIntoWorklist(dst);
    return changed;
}

/*!
 * Process gep edges
 *	src --gep--> dst,
 *	for each srcPtdNode \in pts(src) ==> add fieldSrcPtdNode into tmpDstPts
 *		union pts(dst) with tmpDstPts
 */
bool Andersen::processGep(NodeID, const GepCGEdge* edge)
{
    const PointsTo& srcPts = getDiffPts(edge->getSrcID());
    (const_cast<GepCGEdge*>(edge))->traverseCount++;
    return processGepPts(srcPts, edge);
}

/*!
 * Compute points-to for gep edges
 */
bool Andersen::processGepPts(const PointsTo& pts, const GepCGEdge* edge)
{
    numOfProcessedGep++;

    PointsTo tmpDstPts;
    if (SVFUtil::isa<VariantGepCGEdge>(edge))
    {
        // If a pointer is connected by a variant gep edge,
        // then set this memory object to be field insensitive,
        // unless the object is a black hole/constant.
        for (NodeID o : pts)
        {
            if (consCG->isBlkObjOrConstantObj(o))
            {
                tmpDstPts.set(o);
                continue;
            }

            if (!isFieldInsensitive(o))
            {
                setObjFieldInsensitive(o);
                consCG->addNodeToBeCollapsed(consCG->getBaseObjNode(o));
            }

            // Add the field-insensitive node into pts.
            NodeID baseId = consCG->getFIObjNode(o);
            tmpDstPts.set(baseId);
        }
    }
    else if (const NormalGepCGEdge* normalGepEdge = SVFUtil::dyn_cast<NormalGepCGEdge>(edge))
    {
        // TODO: after the node is set to field insensitive, handling invariant
        // gep edge may lose precision because offsets here are ignored, and the
        // base object is always returned.
        for (NodeID o : pts)
        {
            if (consCG->isBlkObjOrConstantObj(o))
            {
                tmpDstPts.set(o);
                continue;
            }

            if (!matchType(edge->getSrcID(), o, normalGepEdge)) continue;

            NodeID fieldSrcPtdNode = consCG->getGepObjNode(o, normalGepEdge->getLocationSet());
            tmpDstPts.set(fieldSrcPtdNode);
            addTypeForGepObjNode(fieldSrcPtdNode, normalGepEdge);
        }
    }
    else
    {
        assert(false && "Andersen::processGepPts: New type GEP edge type?");
    }

    NodeID dstId = edge->getDstID();
    if (unionPts(dstId, tmpDstPts))
    {
        pushIntoWorklist(dstId);
        return true;
    }

    return false;
}

/**
 * Detect and collapse PWC nodes produced by processing gep edges, under the constraint of field limit.
 */
inline void Andersen::collapsePWCNode(NodeID nodeId)
{
    // If a node is a PWC node, collapse all its points-to tarsget.
    // collapseNodePts() may change the points-to set of the nodes which have been processed
    // before, in this case, we may need to re-do the analysis.
    if (mergePWC() && consCG->isPWCNode(nodeId) && collapseNodePts(nodeId))
        reanalyze = true;
}

inline void Andersen::collapseFields()
{
    while (consCG->hasNodesToBeCollapsed())
    {
        NodeID node = consCG->getNextCollapseNode();
        // collapseField() may change the points-to set of the nodes which have been processed
        // before, in this case, we may need to re-do the analysis.
        if (collapseField(node))
            reanalyze = true;
    }
}

/*
 * Merge constraint graph nodes based on SCC cycle detected.
 */
void Andersen::mergeSccCycle()
{
    NodeStack revTopoOrder;
    NodeStack & topoOrder = getSCCDetector()->topoNodeStack();
    consCG->dump(string("temp")+std::to_string(numOfSCCDetection));
    while (!topoOrder.empty())
    {
        NodeID repNodeId = topoOrder.top();
        topoOrder.pop();
        revTopoOrder.push(repNodeId);
        const NodeBS& subNodes = getSCCDetector()->subNodes(repNodeId);
        // merge sub nodes to rep node
        mergeSccNodes(repNodeId, subNodes);
    }

    // restore the topological order for later solving.
    while (!revTopoOrder.empty())
    {
        NodeID nodeId = revTopoOrder.top();
        revTopoOrder.pop();
        topoOrder.push(nodeId);
    }
}

void Andersen::doBackwardAnalysis(CopyCGEdge* copy) {
    // Copy must be a complex CopyCGEdge
    // First we find the "base" nodes (src for load-derived-copy-edge, dst for store-derived-copy-edge
    // For these load and store edges, we compute how the ptd came to exist in pts(base)
    // 1. If it is via an addr edge, then we know what's happening
    // 2. If it is via another copy edge, then we take further steps outlined below

    llvm::errs() << "Handling copy edge with src = " << copy->getSrcID() << ", dest = " << copy->getDstID() << ", complex src = " << copy->srcComplexID << ", complex dst = " << copy->dstComplexID << "\n";

    ConstraintNode* baseNode = nullptr;
    ConstraintNode* ptd = nullptr;
    // Determine load or store edge
    if (copy->getSrcComplexID() != copy->getSrcID()) {
        // Load edge
        // q = *p implies a -- copy --> p, where pts(p) contains a
        assert(getPts(copy->getSrcComplexID()).test(copy->getSrcID()) && "complex constraint derivation error");
        baseNode = consCG->getConstraintNode(copy->getSrcComplexID());
        ptd = consCG->getConstraintNode(copy->getSrcID());
    } else if (copy->getDstComplexID() != copy->getDstID()) {
        // Store edge
        // p -- load --> *q implies p -- copy --> a, where pts(q) contains a
        assert(getPts(copy->getDstComplexID()).test(copy->getDstID()) && "complex constraint derivation error");
        baseNode = consCG->getConstraintNode(copy->getDstComplexID());
        ptd = consCG->getConstraintNode(copy->getDstID());
    }

    // Check if there's an addr edge into the baseNode that contains ptd

    bool foundSource = false;
    for (ConstraintEdge* inEdge: baseNode->getAddrInEdges()) {
        AddrCGEdge* addrEdge = dyn_cast<AddrCGEdge>(inEdge);
        assert(addrEdge && "Address edge, but not address edge?");
        if (addrEdge->getSrcID() == ptd->getId()) {
            llvm::errs() << "Success!\n";
            foundSource = true;
            break;
        }
    }

    if (foundSource) return;

    // Now we know that we have a baseNode and somehow ptd became an element of its pts-set
    // This is a bit tricky here
    // In case of vanilla copy edge (p1 --> p2), we simply have to go backwards analyzing the source
    // In case of derived copy edge (p1 --> p2), caused by another load/store edge involving p3,
    // there can be imprecision along two different paths.
    //      a) the copy edge path (p1 --> p2). Here we need to identify how ptd came into the pts(p1)
    //      b) the complex edge path (p1 --> p3 or p3 --> p2). Here we need to identify how pts(p3) came to contain p1 or p2 (depending on the type of edge)

    // Where did this ptd come from? 

    ConstraintEdge* responsibleEdge = nullptr;
    for (ConstraintEdge* edge: baseNode->getCopyInEdges()) {
        CopyCGEdge* copyCGEdge = dyn_cast<CopyCGEdge>(edge);
        assert(copyCGEdge && "Copy edge");
        if (copyCGEdge->getPtContributedData().test(copy->getSrcID())) {
            responsibleEdge = copyCGEdge;
        }
    }
    std::vector<std::tuple<ConstraintEdge*, NodeID>> workList;
    workList.push_back(std::make_tuple(responsibleEdge, ptd->getId()));

    while (workList.size() > 0) {
        std::tuple<ConstraintEdge*, NodeID>& tuple = workList.back();
        workList.pop_back();
        ConstraintEdge* responsibleEdge = std::get<0>(tuple);
        NodeID ptd = std::get<1>(tuple);

        if (CopyCGEdge* copy = dyn_cast<CopyCGEdge>(responsibleEdge)) {
            if (!copy->isDerived()) {
                // Where did this come from? 
                // Incoming ptds for this
                NodeID nodeId = copy->getSrcID();
                ConstraintNode* srcNode = consCG->getConstraintNode(nodeId);
                // Incoming copy edges
                for (ConstraintEdge* inCopy: srcNode->getCopyInEdges()) {
                    CopyCGEdge* copyCGEdge = dyn_cast<CopyCGEdge>(inCopy);
                    if (copyCGEdge->getPtContributedData().test(ptd)) {
                        // backward analysis
                        workList.push_back(std::make_tuple(copyCGEdge, ptd));
                    }
                }
            } else {
                // Find the correct edge
                if (copy->getSrcComplexID() > 0) {
                    // Load edge
                    ConstraintNode* loadSrcNode = consCG->getConstraintNode(copy->getSrcComplexID());
                    ConstraintNode* loadDstNode = consCG->getConstraintNode(copy->getDstID());

                    ConstraintEdge* complexEdge = consCG->getEdge(loadSrcNode, loadDstNode, ConstraintEdge::ConstraintEdgeK::Load);

                    LoadCGEdge* loadCGEdge = dyn_cast<LoadCGEdge>(complexEdge);

                    assert(loadCGEdge && "not a load edge?");

                    NodeID ptdFollow = copy->getSrcID();

                    // src -- load --> dst: caused copy->getSrc() -- copy --> copy->getDst()
                    // We should track what added copy->getSrc() to pts(src)

                    for (ConstraintEdge* inCopy: loadSrcNode->getCopyInEdges()) {
                        CopyCGEdge* copyCGEdge = dyn_cast<CopyCGEdge>(inCopy);
                        assert(copyCGEdge && "not copy edge?");
                        // We need to find how ptdFollow came up in the copyCGEdge
                        workList.push_back(std::make_tuple(copyCGEdge, ptdFollow));                                                
                    }
                    
                } else if (copy->getDstComplexID() > 0) {
                    ConstraintNode* storeSrcNode = consCG->getConstraintNode(copy->getSrcID());
                    ConstraintNode* storeDstNode = consCG->getConstraintNode(copy->getDstComplexID());

                    ConstraintEdge* complexEdge = consCG->getEdge(storeSrcNode, storeDstNode, ConstraintEdge::ConstraintEdgeK::Store);

                    StoreCGEdge* storeEdge = dyn_cast<StoreCGEdge>(complexEdge);
                    assert(storeEdge && "not a store edge");

                    NodeID ptdFollow = copy->getDstID();

                    for (ConstraintEdge* inCopy: storeDstNode->getCopyInEdges()) {
                        CopyCGEdge* copyCGEdge = dyn_cast<CopyCGEdge>(inCopy);
                        assert(copyCGEdge && "not copy edge?");
                        workList.push_back(std::make_tuple(copyCGEdge, ptdFollow));
                    }
                }
            }
        }
    }

}

/*
void Andersen::doBackwardAnalysis(CopyCGEdge* copy, NodeID ptd) {
    llvm::errs() << "Handling copy edge with src = " << copy->getSrcID() << ", dest = " << copy->getDstID() << ", complex src = " << copy->srcComplexID << ", complex dst = " << copy->dstComplexID << "\n";
    std::stack<ConstraintEdge*> stack;
    // Is this an artifact of a load or a store edge? 
    if (copy->srcComplexID != copy->getSrcID()) {
        // Load edge
        // pts(copy->srcComplexID) contained the copy->getSrcID()
        assert(getPts(copy->srcComplexID).test(copy->getSrcID()) && "complex constraint derivation error");
        // Find the copy edge that led to the addition of this element in the points-to set
        
        ConstraintNode* node = consCG->getConstraintNode(copy->srcComplexID);
        bool copyDerived = false;
        for (ConstraintEdge* edge: node->getCopyInEdges()) {
            CopyCGEdge* copyCGEdge = dyn_cast<CopyCGEdge>(edge);
            assert(copyCGEdge && "Copy edge");
            if (copyCGEdge->getPtContributedData().test(copy->getSrcID())) {
                doBackwardAnalysis(copyCGEdge, ptd);
                copyDerived = true;
            }
        }
        if (!copyDerived) {
            // Must be an addr edge
            for (ConstraintEdge* edge: node->getAddrInEdges()) {
                AddrCGEdge* addrCGEdge = dyn_cast<AddrCGEdge>(edge);
                if (addrCGEdge->getSrcID() == copy->getSrcID()) {
                    llvm::errs() << "Addr edge: \n";
                }
            }
        }
    } else if (copy->dstComplexID != copy->getDstID()) {
        // Store edge
        assert(getPts(copy->dstComplexID).test(copy->getDstID()) && "complex constraint derivation error");
        ConstraintNode* node = consCG->getConstraintNode(copy->dstComplexID);
        bool copyDerived = false;
        for (ConstraintEdge* edge: node->getCopyInEdges()) {
            CopyCGEdge* copyCGEdge = dyn_cast<CopyCGEdge>(edge);
            assert(copyCGEdge && "Copy edge");
            if (copyCGEdge->getPtContributedData().test(copy->getSrcID())) {
                doBackwardAnalysis(copyCGEdge, ptd);
                copyDerived = true;
            }
        }
        if (!copyDerived) {
            // Must be an addr edge
            for (ConstraintEdge* edge: node->getAddrInEdges()) {
                AddrCGEdge* addrCGEdge = dyn_cast<AddrCGEdge>(edge);
                if (addrCGEdge->getSrcID() == copy->getSrcID()) {
                    llvm::errs() << "Addr edge: \n";
                }
            }
        }
    } else {
        // A real copy edge
        // Among the incoming copy edges of the source, find the one that brought this ptd
        ConstraintNode* node = consCG->getConstraintNode(copy->getSrcID());
        bool copyDerived = false;
        for (ConstraintEdge* edge: node->getCopyInEdges()) {
            CopyCGEdge* copyCGEdge = dyn_cast<CopyCGEdge>(edge);
            assert(copyCGEdge && "Copy edge");
            if (copyCGEdge->getPtContributedData().test(ptd) {
                doBackwardAnalysis(copyCGEdge, ptd);
                copyDerived = true;
            }
        }
        // Not found copy?
        // If this corresponds to a return edge, we can do something nice, like make it context-sensitive
        llvm::errs() << "Dumping llvm value: " << *(copy->getLLVMValue()) << "\n";
    }
}
*/

/**
 * Union points-to of subscc nodes into its rep nodes
 * Move incoming/outgoing direct edges of sub node to rep node
 */
void Andersen::mergeSccNodes(NodeID repNodeId, const NodeBS& subNodes)
{
    std::vector<ConstraintEdge*> edges;

    if (subNodes.count() > 1) {
        for (NodeBS::iterator nodeIt = subNodes.begin(); nodeIt != subNodes.end(); nodeIt++)
        {
            NodeID subNodeId = *nodeIt;
            errs() << "NumOfSCCDetection: " << numOfSCCDetection << "Subnode id: " << subNodeId << "\n";
            ConstraintNode* subNode = consCG->getConstraintNode(subNodeId);
            for (ConstraintEdge* edge : subNode->getCopyOutEdges()) {
                NodeID destId = edge->getDstID();
                if (subNodes.test(destId)) 
                    edges.push_back(edge);
            }
        }
    }

    if (edges.size() > 1) {
        errs() << "SCC Edges count: " << edges.size() << "\n";
        for (ConstraintEdge* sccEdge: edges) {
            // Do it for something simple
            llvm::errs() << sccEdge->getSrcID() << " : " << sccEdge->getDstID() << "\n";

            if (CopyCGEdge* copy = SVFUtil::dyn_cast<CopyCGEdge>(sccEdge)) {
                if (copy->isDerived()) {
                    // Move backwards to analyze this
                    doBackwardAnalysis(copy);                    
                }
                //llvm::errs() << "Src complex id: " << copy->srcComplexID << "\n";
                //llvm::errs() << "Dst complex id: " << copy->dstComplexID << "\n";
            }
        }
    }

    for (NodeBS::iterator nodeIt = subNodes.begin(); nodeIt != subNodes.end(); nodeIt++)
    {
        NodeID subNodeId = *nodeIt;
        if (subNodeId != repNodeId)
        {
            mergeNodeToRep(subNodeId, repNodeId);
        }
    }
}

/**
 * Collapse node's points-to set. Change all points-to elements into field-insensitive.
 */
bool Andersen::collapseNodePts(NodeID nodeId)
{
    bool changed = false;
    const PointsTo& nodePts = getPts(nodeId);
    /// Points to set may be changed during collapse, so use a clone instead.
    PointsTo ptsClone = nodePts;
    for (PointsTo::iterator ptsIt = ptsClone.begin(), ptsEit = ptsClone.end(); ptsIt != ptsEit; ptsIt++)
    {
        if (isFieldInsensitive(*ptsIt))
            continue;

        if (collapseField(*ptsIt))
            changed = true;
    }
    return changed;
}

/**
 * Collapse field. make struct with the same base as nodeId become field-insensitive.
 */
bool Andersen::collapseField(NodeID nodeId)
{
    /// Black hole doesn't have structures, no collapse is needed.
    /// In later versions, instead of using base node to represent the struct,
    /// we'll create new field-insensitive node. To avoid creating a new "black hole"
    /// node, do not collapse field for black hole node.
    if (consCG->isBlkObjOrConstantObj(nodeId))
        return false;

    bool changed = false;

    double start = stat->getClk();

    // set base node field-insensitive.
    setObjFieldInsensitive(nodeId);

    // replace all occurrences of each field with the field-insensitive node
    NodeID baseId = consCG->getFIObjNode(nodeId);
    NodeID baseRepNodeId = consCG->sccRepNode(baseId);
    NodeBS & allFields = consCG->getAllFieldsObjNode(baseId);
    for (NodeBS::iterator fieldIt = allFields.begin(), fieldEit = allFields.end(); fieldIt != fieldEit; fieldIt++)
    {
        NodeID fieldId = *fieldIt;
        if (fieldId != baseId)
        {
            // use the reverse pts of this field node to find all pointers point to it
            const NodeSet revPts = getRevPts(fieldId);
            for (const NodeID o : revPts)
            {
                // change the points-to target from field to base node
                clearPts(o, fieldId);
                addPts(o, baseId);
                pushIntoWorklist(o);

                changed = true;
            }
            // merge field node into base node, including edges and pts.
            NodeID fieldRepNodeId = consCG->sccRepNode(fieldId);
            if (fieldRepNodeId != baseRepNodeId)
                mergeNodeToRep(fieldRepNodeId, baseRepNodeId);

            // collect each gep node whose base node has been set as field-insensitive
            redundantGepNodes.set(fieldId);
        }
    }

    if (consCG->isPWCNode(baseRepNodeId))
        if (collapseNodePts(baseRepNodeId))
            changed = true;

    double end = stat->getClk();
    timeOfCollapse += (end - start) / TIMEINTERVAL;

    return changed;
}

/*!
 * SCC detection on constraint graph
 */
NodeStack& Andersen::SCCDetect()
{
    numOfSCCDetection++;

    double sccStart = stat->getClk();
    WPAConstraintSolver::SCCDetect();
    double sccEnd = stat->getClk();

    timeOfSCCDetection +=  (sccEnd - sccStart)/TIMEINTERVAL;

    double mergeStart = stat->getClk();

    mergeSccCycle();

    double mergeEnd = stat->getClk();

    timeOfSCCMerges +=  (mergeEnd - mergeStart)/TIMEINTERVAL;

    return getSCCDetector()->topoNodeStack();
}

/*!
 * Update call graph for the input indirect callsites
 */
bool Andersen::updateCallGraph(const CallSiteToFunPtrMap& callsites)
{

    double cgUpdateStart = stat->getClk();

    CallEdgeMap newEdges;
    onTheFlyCallGraphSolve(callsites,newEdges);
    NodePairSet cpySrcNodes;	/// nodes as a src of a generated new copy edge
    for(CallEdgeMap::iterator it = newEdges.begin(), eit = newEdges.end(); it!=eit; ++it )
    {
        CallSite cs = SVFUtil::getLLVMCallSite(it->first->getCallSite());
        for(FunctionSet::iterator cit = it->second.begin(), ecit = it->second.end(); cit!=ecit; ++cit)
        {
            connectCaller2CalleeParams(cs,*cit,cpySrcNodes);
        }
    }
    for(NodePairSet::iterator it = cpySrcNodes.begin(), eit = cpySrcNodes.end(); it!=eit; ++it)
    {
        pushIntoWorklist(it->first);
    }

    double cgUpdateEnd = stat->getClk();
    timeOfUpdateCallGraph += (cgUpdateEnd - cgUpdateStart) / TIMEINTERVAL;

    return (!newEdges.empty());
}

void Andersen::heapAllocatorViaIndCall(CallSite cs, NodePairSet &cpySrcNodes)
{
    assert(SVFUtil::getCallee(cs) == nullptr && "not an indirect callsite?");
    RetBlockNode* retBlockNode = pag->getICFG()->getRetBlockNode(cs.getInstruction());
    const PAGNode* cs_return = pag->getCallSiteRet(retBlockNode);
    NodeID srcret;
    CallSite2DummyValPN::const_iterator it = callsite2DummyValPN.find(cs);
    if(it != callsite2DummyValPN.end())
    {
        srcret = sccRepNode(it->second);
    }
    else
    {
        NodeID valNode = pag->addDummyValNode();
        NodeID objNode = pag->addDummyObjNode(cs.getType());
        addPts(valNode,objNode);
        callsite2DummyValPN.insert(std::make_pair(cs,valNode));
        consCG->addConstraintNode(new ConstraintNode(valNode),valNode);
        consCG->addConstraintNode(new ConstraintNode(objNode),objNode);
        srcret = valNode;
    }

    NodeID dstrec = sccRepNode(cs_return->getId());
    if(addCopyEdge(srcret, dstrec))
        cpySrcNodes.insert(std::make_pair(srcret,dstrec));
}

/*!
 * Connect formal and actual parameters for indirect callsites
 */
void Andersen::connectCaller2CalleeParams(CallSite cs, const SVFFunction* F, NodePairSet &cpySrcNodes)
{
    assert(F);

    DBOUT(DAndersen, outs() << "connect parameters from indirect callsite " << *cs.getInstruction() << " to callee " << *F << "\n");

    CallBlockNode* callBlockNode = pag->getICFG()->getCallBlockNode(cs.getInstruction());
    RetBlockNode* retBlockNode = pag->getICFG()->getRetBlockNode(cs.getInstruction());

    if(SVFUtil::isHeapAllocExtFunViaRet(F) && pag->callsiteHasRet(retBlockNode))
    {
        heapAllocatorViaIndCall(cs,cpySrcNodes);
    }

    if (pag->funHasRet(F) && pag->callsiteHasRet(retBlockNode))
    {
        const PAGNode* cs_return = pag->getCallSiteRet(retBlockNode);
        const PAGNode* fun_return = pag->getFunRet(F);
        if (cs_return->isPointer() && fun_return->isPointer())
        {
            NodeID dstrec = sccRepNode(cs_return->getId());
            NodeID srcret = sccRepNode(fun_return->getId());
            if(addCopyEdge(srcret, dstrec))
            {
                cpySrcNodes.insert(std::make_pair(srcret,dstrec));
            }
        }
        else
        {
            DBOUT(DAndersen, outs() << "not a pointer ignored\n");
        }
    }

    if (pag->hasCallSiteArgsMap(callBlockNode) && pag->hasFunArgsList(F))
    {

        // connect actual and formal param
        const PAG::PAGNodeList& csArgList = pag->getCallSiteArgsList(callBlockNode);
        const PAG::PAGNodeList& funArgList = pag->getFunArgsList(F);
        //Go through the fixed parameters.
        DBOUT(DPAGBuild, outs() << "      args:");
        PAG::PAGNodeList::const_iterator funArgIt = funArgList.begin(), funArgEit = funArgList.end();
        PAG::PAGNodeList::const_iterator csArgIt  = csArgList.begin(), csArgEit = csArgList.end();
        for (; funArgIt != funArgEit; ++csArgIt, ++funArgIt)
        {
            //Some programs (e.g. Linux kernel) leave unneeded parameters empty.
            if (csArgIt  == csArgEit)
            {
                DBOUT(DAndersen, outs() << " !! not enough args\n");
                break;
            }
            const PAGNode *cs_arg = *csArgIt ;
            const PAGNode *fun_arg = *funArgIt;

            if (cs_arg->isPointer() && fun_arg->isPointer())
            {
                DBOUT(DAndersen, outs() << "process actual parm  " << cs_arg->toString() << " \n");
                NodeID srcAA = sccRepNode(cs_arg->getId());
                NodeID dstFA = sccRepNode(fun_arg->getId());
                if(addCopyEdge(srcAA, dstFA))
                {
                    cpySrcNodes.insert(std::make_pair(srcAA,dstFA));
                }
            }
        }

        //Any remaining actual args must be varargs.
        if (F->isVarArg())
        {
            NodeID vaF = sccRepNode(pag->getVarargNode(F));
            DBOUT(DPAGBuild, outs() << "\n      varargs:");
            for (; csArgIt != csArgEit; ++csArgIt)
            {
                const PAGNode *cs_arg = *csArgIt;
                if (cs_arg->isPointer())
                {
                    NodeID vnAA = sccRepNode(cs_arg->getId());
                    if (addCopyEdge(vnAA,vaF))
                    {
                        cpySrcNodes.insert(std::make_pair(vnAA,vaF));
                    }
                }
            }
        }
        if(csArgIt != csArgEit)
        {
            writeWrnMsg("too many args to non-vararg func.");
            writeWrnMsg("(" + getSourceLoc(cs.getInstruction()) + ")");
        }
    }
}

/*!
 * merge nodeId to newRepId. Return true if the newRepId is a PWC node
 */
bool Andersen::mergeSrcToTgt(NodeID nodeId, NodeID newRepId)
{

    if(nodeId==newRepId)
        return false;

    /// union pts of node to rep
    updatePropaPts(newRepId, nodeId);
    unionPts(newRepId,nodeId);

    /// move the edges from node to rep, and remove the node
    ConstraintNode* node = consCG->getConstraintNode(nodeId);
    bool gepInsideScc = consCG->moveEdgesToRepNode(node, consCG->getConstraintNode(newRepId));

    /// set rep and sub relations
    updateNodeRepAndSubs(node->getId(),newRepId);

    consCG->removeConstraintNode(node);

    return gepInsideScc;
}
/*
 * Merge a node to its rep node based on SCC detection
 */
void Andersen::mergeNodeToRep(NodeID nodeId,NodeID newRepId)
{

    ConstraintNode* node = consCG->getConstraintNode(nodeId);
    bool gepInsideScc = mergeSrcToTgt(nodeId,newRepId);
    /// 1. if find gep edges inside SCC cycle, the rep node will become a PWC node and
    /// its pts should be collapsed later.
    /// 2. if the node to be merged is already a PWC node, the rep node will also become
    /// a PWC node as it will have a self-cycle gep edge.
    if (gepInsideScc || node->isPWCNode())
        consCG->setPWCNode(newRepId);
}

/*
 * Updates subnodes of its rep, and rep node of its subs
 */
void Andersen::updateNodeRepAndSubs(NodeID nodeId, NodeID newRepId)
{
    consCG->setRep(nodeId,newRepId);
    NodeBS repSubs;
    repSubs.set(nodeId);
    /// update nodeToRepMap, for each subs of current node updates its rep to newRepId
    //  update nodeToSubsMap, union its subs with its rep Subs
    NodeBS& nodeSubs = consCG->sccSubNodes(nodeId);
    for(NodeBS::iterator sit = nodeSubs.begin(), esit = nodeSubs.end(); sit!=esit; ++sit)
    {
        NodeID subId = *sit;
        consCG->setRep(subId,newRepId);
    }
    repSubs |= nodeSubs;
    consCG->setSubs(newRepId,repSubs);
    consCG->resetSubs(nodeId);
}

/*!
 * Print pag nodes' pts by an ascending order
 */
void Andersen::dumpTopLevelPtsTo()
{
    for (OrderedNodeSet::iterator nIter = this->getAllValidPtrs().begin();
            nIter != this->getAllValidPtrs().end(); ++nIter)
    {
        const PAGNode* node = getPAG()->getPAGNode(*nIter);
        if (getPAG()->isValidTopLevelPtr(node))
        {
            const PointsTo& pts = this->getPts(node->getId());
            outs() << "\nNodeID " << node->getId() << " ";

            if (pts.empty())
            {
                outs() << "\t\tPointsTo: {empty}\n\n";
            }
            else
            {
                outs() << "\t\tPointsTo: { ";

                multiset<Size_t> line;
                for (PointsTo::iterator it = pts.begin(), eit = pts.end();
                        it != eit; ++it)
                {
                    line.insert(*it);
                }
                for (multiset<Size_t>::const_iterator it = line.begin(); it != line.end(); ++it)
                    outs() << *it << " ";
                outs() << "}\n\n";
            }
        }
    }

    outs().flush();
}

