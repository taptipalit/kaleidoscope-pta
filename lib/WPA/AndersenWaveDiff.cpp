//===- AndersenWaveDiff.cpp -- Wave propagation based Andersen's analysis with caching--//
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
//===--------------------------------------------------------------------------------===//

/*
 * AndersenWaveDiff.cpp
 *
 *  Created on: 23/11/2013
 *      Author: yesen
 */

#include "WPA/Andersen.h"

using namespace SVF;
using namespace SVFUtil;

AndersenWaveDiff* AndersenWaveDiff::diffWave = nullptr;

int AndersenWaveDiff::getAvgPts() {
    int sumPts = 0;
    int count = 0;
    for (PAG::iterator iter = getPAG()->begin(), eiter = getPAG()->end();
            iter != eiter; ++iter)
    {
        NodeID node = iter->first;
        const PointsTo& pts = getPts(node);
        u32_t size = pts.count();
        sumPts += size;
        count++;
    }
    return sumPts/count;
}

int AndersenWaveDiff::getMaxPts() {
    int maxPts = 0;
    for (PAG::iterator iter = getPAG()->begin(), eiter = getPAG()->end();
            iter != eiter; ++iter)
    {
        NodeID node = iter->first;
        const PointsTo& pts = getPts(node);
        u32_t size = pts.count();
        if (size > maxPts) {
            maxPts = size;
        }
    }
    return maxPts;
}
/*!
 * solve worklist
 */
void AndersenWaveDiff::solveWorklist()
{
    // Initialize the nodeStack via a whole SCC detection
    // Nodes in nodeStack are in topological order by default.
    NodeStack& nodeStack = SCCDetect();

    // Process nodeStack and put the changed nodes into workList.
    while (!nodeStack.empty())
    {
        NodeID nodeId = nodeStack.top();
        nodeStack.pop();
        collapsePWCNode(nodeId);
        // process nodes in nodeStack
        processNode(nodeId);
        collapseFields();
    }
    maxPts.push_back(getMaxPts());
    avgPts.push_back(getAvgPts());

    // This modification is to make WAVE feasible to handle PWC analysis
    if (!mergePWC())
    {
        NodeStack tmpWorklist;
        int genCount = worklist.size();
        int count = 0;
        while (!isWorklistEmpty())
        {
            NodeID nodeId = popFromWorklist();
            collapsePWCNode(nodeId);
            // process nodes in nodeStack
            processNode(nodeId);
            collapseFields();
            tmpWorklist.push(nodeId);
            count ++; 
            if (count == genCount) {
                genCount = worklist.size();
                count = 0;
                maxPts.push_back(getMaxPts());
                avgPts.push_back(getAvgPts());
            }
        }
        maxPts.push_back(getMaxPts());
        avgPts.push_back(getAvgPts());


        while (!tmpWorklist.empty())
        {
            NodeID nodeId = tmpWorklist.top();
            tmpWorklist.pop();
            pushIntoWorklist(nodeId);
        }
        maxPts.push_back(getMaxPts());
        avgPts.push_back(getAvgPts());

    }

    // New nodes will be inserted into workList during processing.
    int genCount = worklist.size();
    int count = 0;

    while (!isWorklistEmpty())
    {
        NodeID nodeId = popFromWorklist();
        // process nodes in worklist
        postProcessNode(nodeId);
        count ++; 
        if (count == genCount) {
            genCount = worklist.size();
            count = 0;
            maxPts.push_back(getMaxPts());
            avgPts.push_back(getAvgPts());
        }

    }
    maxPts.push_back(getMaxPts());
    avgPts.push_back(getAvgPts());
}

/*!
 * Process edge PAGNode
 */
void AndersenWaveDiff::processNode(NodeID nodeId)
{
    // This node may be merged during collapseNodePts() which means it is no longer a rep node
    // in the graph. Only rep node needs to be handled.
    if (sccRepNode(nodeId) != nodeId)
        return;

    double propStart = stat->getClk();
    ConstraintNode* node = consCG->getConstraintNode(nodeId);
    handleCopyGep(node);
    double propEnd = stat->getClk();
    timeOfProcessCopyGep += (propEnd - propStart) / TIMEINTERVAL;
}

/*!
 * Post process node
 */
void AndersenWaveDiff::postProcessNode(NodeID nodeId)
{
    double insertStart = stat->getClk();

    ConstraintNode* node = consCG->getConstraintNode(nodeId);

    // handle load
    for (ConstraintNode::const_iterator it = node->outgoingLoadsBegin(), eit = node->outgoingLoadsEnd();
            it != eit; ++it)
    {
        if (handleLoad(nodeId, *it))
            reanalyze = true;
    }
    // handle store
    for (ConstraintNode::const_iterator it = node->incomingStoresBegin(), eit =  node->incomingStoresEnd();
            it != eit; ++it)
    {
        if (handleStore(nodeId, *it))
            reanalyze = true;
    }

    double insertEnd = stat->getClk();
    timeOfProcessLoadStore += (insertEnd - insertStart) / TIMEINTERVAL;
}

/*!
 * Handle copy gep
 */
void AndersenWaveDiff::handleCopyGep(ConstraintNode* node)
{
    NodeID nodeId = node->getId();
    computeDiffPts(nodeId);

    if (!getDiffPts(nodeId).empty())
    {
        for (ConstraintEdge* edge : node->getCopyOutEdges())
            if (CopyCGEdge* copyEdge = SVFUtil::dyn_cast<CopyCGEdge>(edge))
                processCopy(nodeId, copyEdge);
        for (ConstraintEdge* edge : node->getGepOutEdges())
            if (GepCGEdge* gepEdge = SVFUtil::dyn_cast<GepCGEdge>(edge))
                processGep(nodeId, gepEdge);
    }
}

/*!
 * Handle load
 */
bool AndersenWaveDiff::handleLoad(NodeID nodeId, const ConstraintEdge* edge)
{
    bool changed = false;
    for (PointsTo::iterator piter = getPts(nodeId).begin(), epiter = getPts(nodeId).end();
            piter != epiter; ++piter)
    {
        if (processLoad(*piter, edge))
        {
            changed = true;
        }
    }
    return changed;
}

/*!
 * Handle store
 */
bool AndersenWaveDiff::handleStore(NodeID nodeId, const ConstraintEdge* edge)
{
    bool changed = false;
    for (PointsTo::iterator piter = getPts(nodeId).begin(), epiter = getPts(nodeId).end();
            piter != epiter; ++piter)
    {
        if (processStore(*piter, edge))
        {
            changed = true;
        }
    }
    return changed;
}

#define INSTRUMENT

/*!
 * Propagate diff points-to set from src to dst
 */
bool AndersenWaveDiff::processCopy(NodeID node, const ConstraintEdge* edge)
{
    numOfProcessedCopy++;
    (const_cast<ConstraintEdge*>(edge))->traverseCount++;

    bool changed = false;
    assert((SVFUtil::isa<CopyCGEdge>(edge)) && "not copy/call/ret ??");
    NodeID dst = edge->getDstID();
    const PointsTo& srcDiffPts = getDiffPts(node);

#ifdef INSTRUMENT
    const PointsTo beforePts(getPts(dst));
#endif

    processCast(edge);
    if(unionPts(dst,srcDiffPts))
    {

#ifdef INSTRUMENT
        PointsTo afterPts(getPts(dst)); // copy

        int aftCount = afterPts.count();
        afterPts.intersectWithComplement(beforePts);
        llvm::errs() << "nodeId : " << dst << " bef/aft/intersec size: " << beforePts.count() << " : " << aftCount << " : " << afterPts.count() << "\n";

        CopyCGEdge* copyCGEdge = const_cast<CopyCGEdge*>(SVFUtil::dyn_cast<CopyCGEdge>(edge));
        assert(copyCGEdge && "processing copy but not CopyCGEdge?");
        copyCGEdge->unionPtsContributed(afterPts);
#endif
        changed = true;
        pushIntoWorklist(dst);
    }

    return changed;
}

/*
 * Merge a node to its rep node
 */
void AndersenWaveDiff::mergeNodeToRep(NodeID nodeId,NodeID newRepId)
{
    if(nodeId==newRepId)
        return;

    /// update rep's propagated points-to set
    updatePropaPts(newRepId, nodeId);

    Andersen::mergeNodeToRep(nodeId, newRepId);
}
