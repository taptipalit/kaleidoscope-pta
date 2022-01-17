#include <stdio.h>

int main(void) {
    int a = 10;
    int *p = &a;
    int **pp = &p;

    int *q = *pp;
    int d = *q;
    printf("%d\n", d);
    return 0;
}
