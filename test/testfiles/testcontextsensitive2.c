#include <stdio.h>

void* returnMe(void* p) {
    return p;
}

int main(void) {
    char* s = malloc(1024);
    char* p = returnMe(s);
    return 0;
}
