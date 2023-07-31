#include <stdio.h>

struct Student {
    int id;
    int* ptr;
    int* qtr;
};

int a = 10, b = 20;

int main(void) {
    struct Student *sptr = malloc(sizeof(struct Student));

    sptr->ptr = &a;
    sptr->qtr = &b;

    int* rtr = sptr->ptr;
    
    print("%d\n", *rtr);

    return 0;
}
