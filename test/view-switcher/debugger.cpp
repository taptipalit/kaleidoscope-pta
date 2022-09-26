#include <iostream>
#include <cstdint>
#include <vector>
#include <map>
#include <unordered_set>
#include <execinfo.h>

using namespace std;

typedef uint64_t Id;
typedef uint64_t Val;

//#define INLINE __attribute__((always_inline))
#define INLINE

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
    bool found = false;
    for (int i = 0; i < len; i++) {
        Id id  = validTargets[i];
        auto valSet = valMap[id];
        if (valSet.find((uint64_t)ptr) == valSet.end()) {
            found = true;
            break;
        }
    }
    if (!found) {
        if (!relax) {
            cerr << "[STRICT] Pointer relationship violated here:\n";
            printBacktrace();
            exit(-1);
        } else {
            cerr << "[RELAX] Pointer relationship violated here:\n";
            printBacktrace();
        }
    }
}

INLINE
extern "C" void recordTarget(Id id, Val val) {
    valMap[id].insert(val);
}
