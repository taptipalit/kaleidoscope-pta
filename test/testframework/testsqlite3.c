#include <stdio.h>

struct A {
    int i;
    int j;
    int k;
};

struct A objA = {10, 30, 40};

int main(void) {
    printf("%d\n", objA.i);
    return 0;
}
