#include <stdio.h>

struct A {
    int a;
    int b;
    int c;
};

int main(void) {
    struct A* objA = malloc(sizeof(struct A));
    int *arr = malloc(1024);

    objA->a = 100;
    objA->b = 200;
    objA->c = 300;

    for (int i = 0; i < 1024; i++) {
        arr[i] = i;
    }
    printf("%d %d %d\n", objA->a, arr[10]);
    return 0;
}
