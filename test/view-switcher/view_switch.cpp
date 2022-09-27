#include <iostream>
#include <cstdint>
#include <vector>
#include <map>
#include <unordered_set>

using namespace std;

typedef uint32_t CycleID;
typedef uint32_t InvariantID;
typedef uint64_t InvariantVal;

//#define INLINE __attribute__((always_inline))
#define INLINE
// For PWC
std::map<InvariantID, InvariantVal> pwcInvariants;

// For VGEP
std::map<CycleID, std::unordered_set<uint64_t>> vgepMap;

extern bool invFlipped;

INLINE
extern "C" void vgepRecordTarget(InvariantID id, InvariantVal val) {
    if (val == 0x0) {
        // This is used to reset stack variables
        vgepMap[id].clear();
    } else {
        vgepMap[id].insert(val);
    }
}

/**
 * Return true (1) if we must switch the view
 * Return false (0) if we don't need to switch the view
 * @param tgt: the current operand for this vgep operation
 * @param len: the len of the targets array
 * @param tgts: the array of valid targets for thie vgep that don't result in
 * changing the view
 *
 */
INLINE
extern "C" uint32_t ptdTargetCheck(uint64_t* tgt, uint64_t len, uint64_t* tgts) {
    for (int i = 0; i < len; i++) {
        uint64_t id = tgts[i];
        auto valSet = vgepMap[id];
        if (valSet.find((uint64_t)tgt) != valSet.end()) {
            cout << "VGEP invariant failed\n";
            //invFlipped = true;
            return 1;
        }
    }

    return 0;
}

INLINE 
extern "C" void updatePWC(uint32_t pwcId, uint32_t invVal) {
   pwcInvariants[pwcId] = invVal;  
}

INLINE 
extern "C" uint32_t checkPWC(uint32_t pwcId, uint64_t newVal, uint64_t offset) {
    if (newVal == pwcInvariants[pwcId]/* + offset*/) { // We're instrumenting the pointer before the gep and after the gep. 
        cout << "PWC invariant " << pwcId << " failed\n";
        //invFlipped = true;
        return 1;
    } else {
        return 0;
    } 
}
 
/**
 * Return 1 if the view needs to be changed
 * Return 0 if the view does not need to be changed
 */
/*
INLINE
extern "C" uint32_t updateAndCheckPWC(uint32_t pwcId, uint32_t invLen, uint32_t invId, uint64_t val, int isGep) {
    int cycleHappened = 1;
    // Check if the cycle happened
    // In case it is a GEP instruction that is trying to update it's value
    // Then, we first check if it is at a known offset from any of the
    // previous pointers
    //
    // We also record the gep values separately and if we can't find a match
    // in our base pointer set, we check there
    int sz1 = pwcInvariants[pwcId].size();
    int sz2 = gepPWCInvariants[pwcId].size();


    // dump out what we've seen so far
    cout << "\nPWC ID: " << pwcId << " invLen = " << invLen << endl;
    cout << "InvID: " << invId << " Value: " << hex << val << endl;
    cout << "invariant values: \n";
    for (auto invIdValPair: pwcInvariants[pwcId]) {
        InvariantVal val = invIdValPair.second;
        cout << dec << "Inv id: " << invIdValPair.first << " " << hex << val << " ";
    }
    cout << endl;
    for (auto gepInvIdValPair: gepPWCInvariants[pwcId]) {
        InvariantVal val = gepInvIdValPair.second;
        cout << hex << val << " ";
    }
    cout << dec << endl;

    if (sz1 + sz2 == (invLen - 1)) { // We've seen all other invariant values
        for (auto invIdValPair: pwcInvariants[pwcId]) {
            InvariantVal prevVal = invIdValPair.second;
            if (val != prevVal) {
                int notSeenGepVal = 0;
                // But then, it might even be a gep
                for (auto gepInvIdValPair: gepPWCInvariants[pwcId]) {
                    InvariantVal gepVal = gepInvIdValPair.second;
                    if (val != gepVal) {
                        notSeenGepVal = 1;
                        break;
                    }
                }
                if (notSeenGepVal) {
                    cycleHappened = 0;
                    break;
                }
            }
        }
    } else {
        cycleHappened = 0;
    }

    if (!isGep) {
        pwcInvariants[pwcId][invId] = val; 
    } else {
        gepPWCInvariants[pwcId][invId] = val;
    }
    if (cycleHappened) {
        invFlipped = true;
        cout << dec << "PWC Invariant " << pwcId << " for invariant-id " << invId << " flipped\n";
    }
    return cycleHappened;
}
*/

INLINE
void switch_view(void) {
    cerr << "Invariant violated\n";
}
