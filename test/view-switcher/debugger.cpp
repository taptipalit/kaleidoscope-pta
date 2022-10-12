#include <iostream>
#include <cstdint>
#include <vector>
#include <map>
#include <unordered_set>
#include <execinfo.h>
#include <algorithm>

using namespace std;

typedef uint64_t Id;
typedef uint64_t Val;

#define INLINE __attribute__((always_inline))
//#define INLINE

// For VGEP
std::map<Id, std::unordered_set<uint64_t>> valMap;


static inline void printBacktrace() {
    void *stackBuffer[64]; 
    int numAddresses = backtrace((void**) &stackBuffer, 64); 
    char **addresses = backtrace_symbols(stackBuffer, numAddresses); 
    for( int i = 0 ; i < numAddresses ; ++i ) { 
        fprintf(stderr, "[%2d]: %s\n", i, addresses[i]); 
    } 
    free(addresses); 
} 

INLINE 
extern "C" void checkPtr(Val ptr, int len, Id* validTargets, int relax) {
    if (ptr == 0x0) return;
    bool found = false;
    std::vector<uint64_t> valList;
    for (int i = 0; i < len; i++) {
        Id id  = validTargets[i];
        auto valSet = valMap[id];
        valList.insert(valList.end(), valSet.begin(), valSet.end());
        
    }
    if (std::find(valList.begin(), valList.end(), ptr) != valList.end()) {
        found = true;
    }
    if (!found) {
        if (!relax) {
            cerr << "[STRICT] Pointer relationship violated here:\n";
            printBacktrace();
        } else {
            cerr << "[RELAX] Pointer relationship violated here:\n";
            printBacktrace();
        }
    }
}

INLINE
extern "C" void recordTarget(Id id, Val val) {
    if (val == 0x0) {
        valMap[id].clear();
    } else {
        valMap[id].insert(val);
    }
}
