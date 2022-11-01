#include <stdio.h>

struct Object {
    int* field0;
    int* field1;
    int* field2;
    int* field3;
    int* field4;
    int* field5;
};

int a, b, c, d, e, f, g, h;

void dothis(void**s, void** p) {
    *s = *p;
}

void (*indcall)() = dothis;

struct Object obj;

int main(void) {
    struct Object* p = &obj; //malloc(sizeof(struct Object));
    int** s;
    p->field0 = &a;
    p->field1 = &b;
    p->field2 = &c;
    p->field3 = &d;
    p->field4 = &e;
    p->field5 = &f;

    //int** q = p;
    //struct Object* p = optr;
    int** q = &(p->field1);
//    s = q; // <---- cycle edge
    p = (struct Object*) s;

    int **r = &(p->field2);

    int* ptr = *r;

    (*indcall)(&s, &q);

    printf("%d\n", *ptr);

    return 0;
}
