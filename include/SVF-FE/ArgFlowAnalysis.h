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
		// There can be multiple paths to the value from different (or same)
		// arguments
		std::map<Value*, std::vector<std::vector<Value*>>> backwardSliceMap;

		// Arg to sink map
		// This records the `store operand` of the StoreInst
		std::map<Argument*, std::vector<Value*>> argSinkMap;

		// Arg to store inst map
		std::map<Argument*, std::vector<Value*>> argToSinkStoreMap;
		
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

		std::map<Value*, std::vector<std::vector<Value*>>>& getBackwardSliceMap() {
			return backwardSliceMap;
		}

		std::map<Argument*, std::vector<Value*>>& getArgSinkMap() {
			return argSinkMap;
		}

		std::map<Argument*, std::vector<Value*>>& getArgToSinkStoreMap() {
			return argToSinkStoreMap;
		}

		std::map<Argument*, std::vector<Value*>>& getArgForwardSliceMap() {
			return argForwardSliceMap;
		}

		void findSinkSites(Argument*);

		void dumpBackwardSlice(Value*);
};

class ArgFlowAnalysis : public ModulePass
{
private:

		// The callgraph information
		std::map<Function*, std::vector<Function*>> calleeMap;
		std::map<Function*, std::vector<Function*>> callerMap;
		std::map<Function*, bool> isSummarizableMap; // Is the function summarizable
																								// Check the function
																								// computeISSummarizable() for
																								// the criteria

		void buildCallGraph(Module&, ArgFlowSummary*);
		bool hasFunctionPointer(Type*);

		void computeIsSummarizable();

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
