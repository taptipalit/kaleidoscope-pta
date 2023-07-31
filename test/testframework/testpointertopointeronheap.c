#include <stdio.h>

struct Student {
    int id;
    int* ptr;
    int* qtr;
};

int a = 10, b = 20;

int main(void) {
    struct Student *sptr = malloc(sizeof(struct Student));

    int** doublepointer = &(sptr->ptr);
    sptr->ptr = &a;
    sptr->qtr = &b;

    int* rtr = sptr->ptr;
    
    printf("%d\n", *rtr);

    *doublepointer = &b;

    int* str = *doublepointer;

    printf("%d\n", *str);

    return 0;
}
