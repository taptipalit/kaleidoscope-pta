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

extern "C" void initWithNoInvariant(void) {
    invFlipped = true;
}

/*
extern "C" void checkCFI(unsigned long* arr, int len_arr, unsigned long* tgt) {
    int found = 0;
    for (int i = 0; i < len_arr; i++) {
        if (tgt == *arr) {
            found = 1;
            break;
        } else {
            arr++;
        }
    }
    if (!found) {
        fprintf(stderr, "CFI error\n");
        exit (-1);
    }
}

extern "C" void blockCFI() {
    fprintf(stderr, "CFI error. This callsite shouldn't be triggered at all\n");
    exit (-1);
}
*/
