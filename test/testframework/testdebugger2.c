#include <stdio.h>

struct Obj {
    int id;
    int id2;
    int *ptr;
};

int main(void) {
    int a = 10;
    struct Obj obj;
    obj.ptr = &a;

    int d = *(obj.ptr);
    printf("%d\n", d);
    return 0;
}
