#include <stdio.h>

void* mytest_malloc(int sz) {
    return malloc(sz);
}

int main(void) {
    char* p = (char*) mytest_malloc(100);
    p[1] = 'a';
    return 0;
}
