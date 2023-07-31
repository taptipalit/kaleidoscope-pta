#include <stdio.h>

struct Student {
    int num;
    int *ptr;
    int *qtr;
};

/*
int main(void) {
    int m = 100, n = 200;
    int a = 1000, b = 2000, c = 3000;

    struct Student s = {111, &m, &n};
    struct Student k = {222, &m, &n};

    void *p, *q;
    
    p = &k;

    struct Student* r = &s;

    q = p;

    q = &s;
    r = q;
    p = &(r->ptr);

    printf("%d\n", **(int**)p);

    return 0;
}

*/

void* dothis() {
    int a = 10, b = 20;
    int m = 200, n = 300;
    struct Student s = {111, &m, &n};
    int **dd;

    void *p, *q, *r;

    p = &a;
    q = &s;
    r = &dothis;

    p = &(((struct Student*)q)->ptr);

    dd = &p;
    dd = &r;

    q = *dd;

    
    return q;
}

int main(void) {
    void (*fptr)() = dothis();
    (*fptr)();
}
