#include <stdio.h>
#include <stdlib.h>

struct Complex {
    int *ptr1;
    int *ptr2;
    int *ptr3;
};

struct Complex complexArr[1024];

int main(void) {
    int a = 10, b = 20, c = 30;

    int *pointer = (void*) 0;
    //complexArr = malloc(1024*sizeof(struct Complex));

    complexArr[0].ptr1 = &a;
    complexArr[0].ptr2 = &b;
    complexArr[0].ptr3 = &c;

    complexArr[1].ptr1 = &a;
    complexArr[1].ptr2 = &b;
    complexArr[1].ptr3 = &c;

    complexArr[2].ptr1 = &a;
    complexArr[2].ptr2 = &b;
    complexArr[2].ptr3 = &c;

    for (int i = 0; i < 1024; i++) {
        complexArr[i].ptr1 = malloc(sizeof(int));
        *(complexArr[i].ptr1) = i;
        complexArr[i].ptr2 = malloc(sizeof(int));
        *(complexArr[i].ptr2) = i + 1;
        complexArr[i].ptr3 = malloc(sizeof(int));
        *(complexArr[i].ptr3) = i + 2;
    }

    int i = a + 10;
    *(int*)(complexArr + i) = 30;

    for (int i = 0; i < 1024; i++) {
        printf("\n");
        pointer = complexArr[i].ptr1;
        printf("%d\n", *pointer);
        pointer = complexArr[i].ptr2;
        printf("%d\n", *pointer);
        pointer = complexArr[i].ptr3;
        printf("%d\n", *pointer);
    }

    return 0;
}
