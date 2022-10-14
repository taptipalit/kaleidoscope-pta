#include <iostream>
#include <cstdint>
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

typedef uint32_t CallSiteID;

std::map<CallSiteID, std::vector<uint64_t>> callsiteTgtWInvMap;
std::map<CallSiteID, std::vector<uint64_t>> callsiteTgtWoInvMap;

bool invFlipped = false;

extern "C" void updateTgtWInv(CallSiteID callsite, uint64_t tgt) {
    callsiteTgtWInvMap[callsite].push_back(tgt);
}

extern "C" void updateTgtWoInv(CallSiteID callsite, uint64_t tgt) {
    callsiteTgtWoInvMap[callsite].push_back(tgt);
}

extern "C" void checkCFI(CallSiteID callsite, uint64_t tgt) {
    bool tgtFound = 0;
    if (!invFlipped) {
        vector<uint64_t>& validTgts = callsiteTgtWInvMap[callsite];
        tgtFound = (find(validTgts.begin(), validTgts.end(), tgt) != validTgts.end());
    } else {
        vector<uint64_t>& validTgts = callsiteTgtWoInvMap[callsite];
        tgtFound = (find(validTgts.begin(), validTgts.end(), tgt) != validTgts.end());
    }
    
    if (!tgtFound) {
        cerr << "CFI error\n";
        exit(-1);
    }
}


