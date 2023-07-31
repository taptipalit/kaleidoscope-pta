#include <stdio.h>

struct Object {
    int id;
    int* ptr;
    int* qtr;
};

int a, b;

int main(void) {
    struct Object* optr = malloc(10*sizeof(struct Object));
    for (int i = 0; i < 10; i++) {
        optr[i].ptr = &a;
        optr[i].qtr = &b;
    }    

    int* mptr = optr[1].ptr;
    int* nptr = optr[1].qtr;

    printf("%d %d\n", *mptr, *nptr);
    return 0;
}
