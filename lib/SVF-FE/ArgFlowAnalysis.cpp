/* SVF - Static Value-Flow Analysis Framework
Copyright (C) 2015 Yulei Sui
Copyright (C) 2015 Jingling Xue

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#define DEBUG_TYPE "arg-flow-analysis"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/InstIterator.h"


#include "SVF-FE/ArgFlowAnalysis.h"

using namespace SVF;

// Identifier variable for the pass
char ArgFlowAnalysis::ID = 0;


// Register the pass
static llvm::RegisterPass<ArgFlowAnalysis> ARGFLOW ("argflow",
        "ArgFlowAnalysis");


void ArgFlowSummary::findSinkSites(Argument* arg) {
	std::vector<Value*> workList;
	std::vector<Value*> processedList;
	workList.push_back(arg);

	while (!workList.empty()) {
		Value* val = workList.back();
		workList.pop_back();

		processedList.push_back(val);

		for (User* u: val->users()) {
			if (Value* uVal = SVFUtil::dyn_cast<Value>(u)) {
				if (StoreInst* store = SVFUtil::dyn_cast<StoreInst>(uVal)) {
					// Is val being sunk?
					if (store->getValueOperand() == val) {
						Value* ptr = store->getPointerOperand();

						getArgSinkMap()[arg].push_back(ptr);
						getArgToSinkStoreMap()[arg].push_back(ptr);

						// We track multiple levels of sinks
						if (std::find(processedList.begin(), processedList.end(), ptr)
								== processedList.end()) {
							workList.push_back(ptr);
						}
					}
					// If it's not being sunk, then something else is being sunk into it
					// We don't care about this situation because we always check the
					// following condition:
					// Is the sink-site of any arg in the _forward slice_ of another arg
					// The forward slice doesn't have to capture the store instruction
					// itself, only its ptr component.

				} else {
					// If it is a call instruction ignore
					if (SVFUtil::isa<CallInst>(uVal)) continue;
					// Just push it back
					if (std::find(processedList.begin(), processedList.end(), uVal) == processedList.end()
							&& SVFUtil::isa<PointerType>(uVal->getType())) {
						workList.push_back(uVal);
					}
					getArgForwardSliceMap()[arg].push_back(uVal);
				}

				// Just copy the backward slice vectors
				getBackwardSliceMap()[uVal] = getBackwardSliceMap()[val];

				// Append val to the end of all of the backward slice vectors
				for (auto& vec: getBackwardSliceMap()[uVal]) {
					vec.push_back(val);
				}
			}
		}
	}
}

ArgFlowSummary* ArgFlowSummary::argFlowSummary;


void ArgFlowSummary::dumpBackwardSlice (Value* val) {

}

bool
ArgFlowAnalysis::runOnModule (Module & module) {
	ArgFlowSummary* summary = ArgFlowSummary::getArgFlowSummary();
	for (Function& func: module.getFunctionList()) {
		if (func.arg_size() < 2) {
			continue;
		}

		if (func.getName() == "evmap_io_add_") {
			llvm::errs() << "# of callsites " << func.getNumUses() << "\n";
		}

		for (Argument& arg: func.args()) {
			// Check if this is a pointer type (Note that this isn't going to be a
			// pointer to a pointer because Args aren't assumed to be stack
			// locations in LLVM IR
			Type* argTy = arg.getType();
			if (SVFUtil::isa<PointerType>(argTy)) {
				summary->findSinkSites(&arg);
			}
		}
		for (Argument& arg1Ref: func.args()) {
			Argument* arg1 = &arg1Ref;
			std::map<Argument*, std::vector<Value*>>::iterator sinkIt;
			for (sinkIt = summary->getArgSinkMap().begin(); sinkIt != summary->getArgSinkMap().end(); sinkIt++) {
				for (Value* sinkSite: sinkIt->second) {
					// Check if this sink-site is in the forward slice of any of the
					// other arguments
					for (Argument& arg2Ref: func.args()) {
						Argument* arg2 = &arg2Ref;
						if (arg1 == arg2 || !SVFUtil::isa<PointerType>(arg1->getType()) || !SVFUtil::isa<PointerType>(arg1->getType())) {
							continue;
						}
						std::vector<Value*>::iterator forwardSliceIt;
						if (std::find(summary->getArgForwardSliceMap()[arg2].begin(), summary->getArgForwardSliceMap()[arg2].end(), sinkSite)
								!= summary->getArgForwardSliceMap()[arg2].end()) {
							// Match found
							llvm::errs() << "Function: " << func.getName() << ", Argument: " << *arg1 << " stored to "  << *arg2 << "\n";
							summary->dumpBackwardSlice(sinkSite);
						}
					}
				}
			}
		}
	}
  return false;
}
