#include <stdio.h>

struct Object {
    int *ptr;
    int *qtr;
};

int a, b;

int main(void) {
    struct Object* optr = malloc(sizeof(struct Object));
    int offset = 0;

    optr->ptr = &a;
    optr->qtr = &b;

    int* ab = (int*) optr;
    int* ba = ab + offset;

    struct Object* newptr = (struct Object*) ba;
    int* myptr = newptr->ptr;

    printf("%d\n", *myptr);
    return 0;
}
