#include <stdio.h>

int main(void) {
    int a = 10;
    int b = 300;
    int *p;
    int *q;

    p = &a;
    q = &b;

    int k = *p;

    *q = 400;

    printf("%d %d\n", k, b);
}
/*
int main(void) {
    int a = 10, b = 20, c = 30;
    int *p, *q;

    p = &a;
    p = &b;

    q = &c;

    int k = *p;
    int m = *q;

    printf("%d %d\n", k, m);
}
*/
