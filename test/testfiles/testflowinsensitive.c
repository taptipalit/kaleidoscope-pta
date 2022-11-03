#include <stdio.h>

int main(void) {
    int a = 10, b = 20, c = 30, d = 40;
    int *p, *q;
    int *r;

    q = &a;

    if (a == 10) {
        p = &a;
        q = p;
    } else {
        p = &b;
    }

    printf ("%d %d\n", *p, *q);

    r = q;

    printf ("%d \n", *r);

    return 0;
}
