#include <stdio.h>

struct Object {
    int* field0;
    int* field1;
    int* field2;
};

int a, b, c;

void dothis(void**s, void** p) {
    *s = *p;
}

void (*indcall)() = dothis;

int main(void) {
    struct Object obj;
    struct Object* p = &obj;
    int** s;
    p->field0 = &a;
    p->field1 = &b;
    p->field2 = &c;

    //int** q = p;
    //struct Object* p = optr;
    int** q = &(p->field1);
    //s = q; // <---- cycle edge
    p = (struct Object*) s;

    int **r = &(p->field2);

    int* ptr = *r;

    (*indcall)(&s, &q);

    printf("%d\n", *ptr);

    return 0;
}
