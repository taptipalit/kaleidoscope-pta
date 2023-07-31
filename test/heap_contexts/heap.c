#include <stdio.h>

struct TypeA {
    int id;
    int* ptr;

};

struct TypeB {
    int id;
    int* qtr;
};

void* my_malloc(int size) {
    void* p = malloc(size);
    return p;
}

int main(void) {
    int a, b;
    int* p, *q;
    struct TypeA* objA = my_malloc(sizeof(struct TypeA));
    struct TypeB* objB = my_malloc(sizeof(struct TypeB));

    objA->ptr = &a;
    objB->qtr = &b;

    p = objA->ptr;
    q = objB->qtr;

    return 0;
}
