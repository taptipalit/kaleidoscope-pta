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
//            invFlipped = true;
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
    if (newVal == pwcInvariants[pwcId] + offset) { // The +offset should probably not be there. We're instrumenting the pointer before the gep and after the gep. 
        cout << "PWC invariant " << pwcId << " failed\n";
 //       invFlipped = true;
        return 1;
    } else {
        return 0;
    } 
}

void free(void* p) { }

INLINE
void switch_view(void) {
    cerr << "Invariant violated\n";
}
