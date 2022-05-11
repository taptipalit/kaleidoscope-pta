#include <iostream>
#include <cstdint>
#include <vector>
#include <map>

using namespace std;

typedef uint32_t CycleID;
typedef uint32_t InvariantID;
typedef uint64_t InvariantVal;

// For PWC
std::map<CycleID, std::map<InvariantID, InvariantVal>> pwcInvariants;
std::map<CycleID, std::map<InvariantID, InvariantVal>> gepPWCInvariants;

// For VGEP
std::map<CycleID, uint64_t> vgepMap;

extern bool invFlipped;

extern "C" void vgepRecordTarget(InvariantID id, InvariantVal val) {
    vgepMap[id] = val;
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
extern "C" uint32_t ptdTargetCheck(uint64_t* tgt, uint64_t len, uint64_t* tgts) {
    for (int i = 0; i < len; i++) {
        uint64_t id = tgts[i];
        uint64_t ptrVal = vgepMap[id];
        if (tgt == (uint64_t*)ptrVal) {
            cout << "VGEP invariant failed\n";
            invFlipped = true;
            return 1;
        }
    }

    return 0;
}

/**
 * Return 1 if the view needs to be changed
 * Return 0 if the view does not need to be changed
 */
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
    cout << endl;

    if (sz1 + sz2 == (invLen - 1)) { // We've seen all other invariant values
        for (auto invIdValPair: pwcInvariants[pwcId]) {
            /*
            if (invIdValPair.first == invId)
                continue;
                */
            InvariantVal prevVal = invIdValPair.second;
            if (val != prevVal) {
                int notSeenGepVal = 0;
                // But then, it might even be a gep
                for (auto gepInvIdValPair: gepPWCInvariants[pwcId]) {
                    /*
                    if (gepInvIdValPair.first == invId) 
                        continue;
                        */
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
        cout << "Invariant flipped\n";
    }
    return cycleHappened;
}

void switch_view(void) {
    cerr << "Invariant violated\n";
}
