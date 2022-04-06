#include <iostream>
#include <cstdint>
#include <vector>
#include <map>

using namespace std;

typedef uint32_t CycleID;
typedef uint32_t InvariantID;
typedef uint64_t InvariantVal;

std::map<CycleID, std::map<InvariantID, InvariantVal>> pwcInvariants;
/**
 * Return true (1) if we must switch the view
 * Return false (0) if we don't need to switch the view
 * @param tgt: the current operand for this vgep operation
 * @param len: the len of the targets array
 * @param tgts: the array of valid targets for thie vgep that don't result in
 * changing the view
 *
 */
extern "C" uint32_t ptdTargetCheck(uint64_t* tgt, uint64_t len, uint64_t* tgts) {
    for (int i = 0; i < len; i++) {
        if (tgt == (uint64_t*)tgts[i]) {
            return 0;
        }
    }
    return 1;
}

/**
 * Return 1 if the view needs to be changed
 * Return 0 if the view does not need to be changed
 */
extern "C" uint32_t updateAndCheckPWC(uint32_t pwcId, uint32_t invLen, uint32_t invId, uint64_t val) {
    pwcInvariants[pwcId][invId] = val;     
    // Check if the cycle happened
    if (pwcInvariants[pwcId].size() == invLen) {
        InvariantVal prevVal = pwcInvariants[pwcId][0];
        for (auto invIdValPair: pwcInvariants[pwcId]) {
            InvariantVal val = invIdValPair.second;
            if (val != prevVal) {
                return 0;
            }
        }
        return 1;
    } else {
        return 0;
    }
}

void switch_view(void) {
    cerr << "Invariant violated\n";
}
