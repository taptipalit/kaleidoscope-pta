#include <stdio.h>

int main(void) {
    int a = 10, b = 120, c = 130;
    int* p, *q, *r;

    p = &a;
    q = &b;
    r = &c;

    int **s;
    s = &q;

    s = &r;

    q = p;
    p = *s; // This isn't really a cycle as s --> r and not q here

    int *ptr = p;

    printf("%d\n", *ptr);

    if (ptr == &a) { printf("ptr --> a\n"); }
    if (ptr == &b) { printf("ptr --> b\n"); }
    if (ptr == &c) { printf("ptr --> c\n"); }

    return 0;
}

