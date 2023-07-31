#include <stdlib.h> 
#include <stdio.h>

struct Object {
    int id;
    int *p;
    int* q;
};

int main(void) {
    int a = 200, b = 300;
    struct Object *o = (struct Object*)malloc(sizeof(struct Object));
    o->id = 100;
    o->p = &a;
    o->q = &b;

    char *p = (char*)o;
    p = p + 2;
    p = p + 5;

    printf("%p\n", p);
    
}
