//===- ConsGEdge.h -- Constraint graph edge-----------------------------------//
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
 * ConsGEdge.h
 *
 *  Created on: Mar 19, 2014
 *      Author: Yulei Sui
 */

#ifndef CONSGEDGE_H_
#define CONSGEDGE_H_

#include "Graphs/PAG.h"
#include "Util/WorkList.h"

#include <map>
#include <set>

namespace SVF
{

class ConstraintNode;
/*!
 * Self-defined edge for constraint resolution
 * including add/remove/re-target, but all the operations do not affect original PAG Edges
 */
typedef GenericEdge<ConstraintNode> GenericConsEdgeTy;
class ConstraintEdge : public GenericConsEdgeTy
{

public:
    /// five kinds of constraint graph edges
    /// Gep edge is used for field sensitivity
    enum ConstraintEdgeK
    {
        Addr, Copy, Store, Load, NormalGep, VariantGep
    };
private:
    EdgeID edgeId;

    int derivedWeight;
    int solvedCount;

    ConstraintEdge* origEdge;

    Value* llvmValue;

public:
    /// Constructor
    ConstraintEdge(ConstraintNode* s, ConstraintNode* d, ConstraintEdgeK k, EdgeID id = 0) : GenericConsEdgeTy(s,d,k),edgeId(id),derivedWeight(0),origEdge(nullptr),llvmValue(nullptr)
    {
    }
    /// Destructor
    ~ConstraintEdge()
    {
    }
    /// Return edge ID
    inline EdgeID getEdgeID() const
    {
        return edgeId;
    }
    /// ClassOf
    static inline bool classof(const GenericConsEdgeTy *edge)
    {
        return edge->getEdgeKind() == Addr ||
               edge->getEdgeKind() == Copy ||
               edge->getEdgeKind() == Store ||
               edge->getEdgeKind() == Load ||
               edge->getEdgeKind() == NormalGep ||
               edge->getEdgeKind() == VariantGep;
    }
    /// Constraint edge type
    typedef GenericNode<ConstraintNode,ConstraintEdge>::GEdgeSetTy ConstraintEdgeSetTy;

    void setSourceEdge(ConstraintEdge* edge) {
        origEdge = edge;
    }

    ConstraintEdge* getSourceEdge() {
        return origEdge;
    }

    void setLLVMValue(const Value* val) {
        llvmValue = const_cast<Value*>(val);
    }

    Value* getLLVMValue() const {
        return llvmValue;
    }

    int getDerivedWeight() { return derivedWeight; } 

    void setDerivedWeight(int derivedWeight) { this->derivedWeight = derivedWeight; }

    int getSolvedCount() { return solvedCount; }

    void incrementSolvedCount() { this->solvedCount++; }

};


/*!
 * Copy edge
 */
class AddrCGEdge: public ConstraintEdge
{
private:
    AddrCGEdge();                      ///< place holder
    AddrCGEdge(const AddrCGEdge &);  ///< place holder
    void operator=(const AddrCGEdge &); ///< place holder
public:
    /// Methods for support type inquiry through isa, cast, and dyn_cast:
    //@{
    static inline bool classof(const AddrCGEdge *)
    {
        return true;
    }
    static inline bool classof(const ConstraintEdge *edge)
    {
        return edge->getEdgeKind() == Addr;
    }
    static inline bool classof(const GenericConsEdgeTy *edge)
    {
        return edge->getEdgeKind() == Addr;
    }
    //@}

    /// constructor
    AddrCGEdge(ConstraintNode* s, ConstraintNode* d, EdgeID id);
};


/*!
 * Copy edge
 */
class CopyCGEdge: public ConstraintEdge
{
private:
    CopyCGEdge();                      ///< place holder
    CopyCGEdge(const CopyCGEdge &);  ///< place holder
    void operator=(const CopyCGEdge &); ///< place holder
public:
    /// Methods for support type inquiry through isa, cast, and dyn_cast:
    //@{
    static inline bool classof(const CopyCGEdge *)
    {
        return true;
    }
    static inline bool classof(const ConstraintEdge *edge)
    {
        return edge->getEdgeKind() == Copy;
    }
    static inline bool classof(const GenericConsEdgeTy *edge)
    {
        return edge->getEdgeKind() == Copy;
    }
    //@}

    /// constructor
    CopyCGEdge(ConstraintNode* s, ConstraintNode* d, EdgeID id) : ConstraintEdge(s,d,Copy,id)
    {
    }
};


/*!
 * Store edge
 */
class StoreCGEdge: public ConstraintEdge
{
private:
    StoreCGEdge();                      ///< place holder
    StoreCGEdge(const StoreCGEdge &);  ///< place holder
    void operator=(const StoreCGEdge &); ///< place holder

public:
    /// Methods for support type inquiry through isa, cast, and dyn_cast:
    //@{
    static inline bool classof(const StoreCGEdge *)
    {
        return true;
    }
    static inline bool classof(const ConstraintEdge *edge)
    {
        return edge->getEdgeKind() == Store;
    }
    static inline bool classof(const GenericConsEdgeTy *edge)
    {
        return edge->getEdgeKind() == Store;
    }
    //@}

    /// constructor
    StoreCGEdge(ConstraintNode* s, ConstraintNode* d, EdgeID id) : ConstraintEdge(s,d,Store,id)
    {
    }
};


/*!
 * Load edge
 */
class LoadCGEdge: public ConstraintEdge
{
private:
    LoadCGEdge();                      ///< place holder
    LoadCGEdge(const LoadCGEdge &);  ///< place holder
    void operator=(const LoadCGEdge &); ///< place holder


public:
    /// Methods for support type inquiry through isa, cast, and dyn_cast:
    //@{
    static inline bool classof(const LoadCGEdge *)
    {
        return true;
    }
    static inline bool classof(const ConstraintEdge *edge)
    {
        return edge->getEdgeKind() == Load;
    }
    static inline bool classof(const GenericConsEdgeTy *edge)
    {
        return edge->getEdgeKind() == Load;
    }
    //@}

    /// Constructor
    LoadCGEdge(ConstraintNode* s, ConstraintNode* d, EdgeID id) : ConstraintEdge(s,d,Load,id)
    {
    }
};


/*!
 * Gep edge
 */
class GepCGEdge: public ConstraintEdge
{
private:
    GepCGEdge();                      ///< place holder
    GepCGEdge(const GepCGEdge &);  ///< place holder
    void operator=(const GepCGEdge &); ///< place holder

protected:

    /// Constructor
    GepCGEdge(ConstraintNode* s, ConstraintNode* d, ConstraintEdgeK k, EdgeID id)
        : ConstraintEdge(s,d,k,id)
    {

    }

public:
    /// Methods for support type inquiry through isa, cast, and dyn_cast:
    //@{
    static inline bool classof(const GepCGEdge *)
    {
        return true;
    }
    static inline bool classof(const ConstraintEdge *edge)
    {
        return edge->getEdgeKind() == NormalGep ||
               edge->getEdgeKind() == VariantGep;
    }
    static inline bool classof(const GenericConsEdgeTy *edge)
    {
        return edge->getEdgeKind() == NormalGep ||
               edge->getEdgeKind() == VariantGep;
    }
    //@}

};

/*!
 * Gep edge with fixed offset size
 */
class NormalGepCGEdge : public GepCGEdge
{
private:
    NormalGepCGEdge();                      ///< place holder
    NormalGepCGEdge(const NormalGepCGEdge &);  ///< place holder
    void operator=(const NormalGepCGEdge &); ///< place holder

    LocationSet ls;	///< location set of the gep edge

public:
    /// Methods for support type inquiry through isa, cast, and dyn_cast:
    //@{
    static inline bool classof(const NormalGepCGEdge *)
    {
        return true;
    }
    static inline bool classof(const GepCGEdge *edge)
    {
        return edge->getEdgeKind() == NormalGep;
    }
    static inline bool classof(const ConstraintEdge *edge)
    {
        return edge->getEdgeKind() == NormalGep;
    }
    static inline bool classof(const GenericConsEdgeTy *edge)
    {
        return edge->getEdgeKind() == NormalGep;
    }
    //@}

    /// Constructor
    NormalGepCGEdge(ConstraintNode* s, ConstraintNode* d, const LocationSet& l, EdgeID id)
        : GepCGEdge(s,d,NormalGep,id), ls(l)
    {}

    /// Get location set of the gep edge
    inline const LocationSet& getLocationSet() const
    {
        return ls;
    }

    /// Get location set of the gep edge
    inline Size_t getOffset() const
    {
        return ls.getOffset();
    }
};

/*!
 * Gep edge with variant offset size
 */
class VariantGepCGEdge : public GepCGEdge
{
private:
    VariantGepCGEdge();                      ///< place holder
    VariantGepCGEdge(const VariantGepCGEdge &);  ///< place holder
    void operator=(const VariantGepCGEdge &); ///< place holder

public:
    enum VarGepSubType { 
        TL_STRUCT, // top-level struct. The `struct* ptr; ptr+i;` types
        DERIVED, // derived from a VarGep, but has constant offset
        CHAR, // char* ptr; ptr + i;
    };
    VarGepSubType subType;


    /// Methods for support type inquiry through isa, cast, and dyn_cast:
    //@{
    static inline bool classof(const VariantGepCGEdge *)
    {
        return true;
    }
    static inline bool classof(const GepCGEdge *edge)
    {
        return edge->getEdgeKind() == VariantGep;
    }
    static inline bool classof(const ConstraintEdge *edge)
    {
        return edge->getEdgeKind() == VariantGep;
    }
    static inline bool classof(const GenericConsEdgeTy *edge)
    {
        return edge->getEdgeKind() == VariantGep;
    }
    //@}

    /// Constructor
    VariantGepCGEdge(ConstraintNode* s, ConstraintNode* d, EdgeID id, VariantGepPE::VarGepSubType subTy)
        : GepCGEdge(s,d,VariantGep,id)
    {
        if (subTy == VariantGepPE::TL_STRUCT) {
            this->subType = TL_STRUCT;
        } else if (subTy == VariantGepPE::DERIVED) {
            this->subType = DERIVED;
        } else {
            this->subType = CHAR;
        }
    }

    virtual inline void setSubType(VarGepSubType ty) {
        subType = ty;
    }
    inline VarGepSubType getSubType() const{
        return subType;
    }

};

} // End namespace SVF

#endif /* CONSGEDGE_H_ */
