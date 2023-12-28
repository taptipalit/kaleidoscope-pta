#ifndef ARGFLOWANALYSIS_H
#define ARGFLOWANALYSIS_H

#include "Util/BasicTypes.h"
#include <map>
#include <utility>
#include <vector>
#include <set>


namespace SVF
{

class ArgFlowSummary {

	private:
		// The master map containing the backward slice of all the 
		// values originating from the Argument
		// This only captures the paths from the argument
		// We assume there is only one path. We will add an assertion here.
		std::map<Value*, std::vector<Value*>> backwardSliceMap;

		// Arg to sink map
		// This records the `store operand` of the StoreInst
		std::map<Argument*, std::vector<Value*>> argSinkMap;
		
		// Arg to forward-slice map
		// This contains loads/casts/geps, etc. Stores live in the argSinkMap
		std::map<Argument*, std::vector<Value*>> argForwardSliceMap;

		static ArgFlowSummary* argFlowSummary;
	public:
		static ArgFlowSummary* getArgFlowSummary() {
			if (!argFlowSummary) {
				argFlowSummary = new ArgFlowSummary();
			}
			return argFlowSummary;
		}

		ArgFlowSummary() {}

		std::map<Value*, std::vector<Value*>>& getBackwardSliceMap() {
			return backwardSliceMap;
		}

		std::map<Argument*, std::vector<Value*>>& getArgSinkMap() {
			return argSinkMap;
		}

		std::map<Argument*, std::vector<Value*>>& getArgForwardSliceMap() {
			return argForwardSliceMap;
		}

		void findSinkSites(Argument*);
};

class ArgFlowAnalysis : public ModulePass
{
private:

public:
    static char ID;

    ArgFlowAnalysis() : ModulePass(ID) {
    }

    StringRef getPassName() const
    {
        return "Do arg flow analysis";
    }

    virtual bool runOnModule (Module & M);
    virtual void getAnalysisUsage(AnalysisUsage &AU) const
    {
        // This pass modifies the control-flow graph of the function
        //AU.setPreservesCFG();
    }


};

}
#endif
