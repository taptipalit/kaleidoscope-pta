#include <stdio.h>

typedef struct Object {
    int id;
    void (*ptr)();
    void (*qtr)();
} Object;

void f1() { printf("f1\n"); }
void f2() { printf("f2\n"); }
void f3() { printf("f3\n"); }
void f4() { printf("f4\n"); }

void copy(void (**a)(), void (**b)()) {
    *a = *b;
}

int main(void) {
    int a, b, c, d;
    Object* objA = malloc(sizeof(Object));
    Object* objB = malloc(sizeof(Object));

    objA->ptr = f1;
    copy(&(objA->qtr), &(objA->ptr));

    objB->ptr = f3;
    copy(&(objB->qtr), &(objB->ptr));

    (*(objA->qtr))();

    (*(objB->qtr))();
    return 0;
}
