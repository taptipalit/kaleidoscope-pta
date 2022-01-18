#include <stdio.h>

void fn(int* p) {
    printf("%d\n", *p);
}

int main(void) {
    int a = 10;
    int *p = &a;
    int **pp = &p;

    int *q = *pp;
    int d = *q;
    fn(p);
    printf("%d\n", d);
    return 0;
}
