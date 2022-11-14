#include <stdio.h>

char* funny(char* s);

int main(void) {
    char* p, *q, *r;
    char a, b,c,d;

    p = &a;
    q = &b;
    r = &c;
    r = &d;

    p = funny(p);
    q = funny(q);
    r = funny(r);

    printf("%c %c %c\n", *p, *q, *r);
    return 0;
}
