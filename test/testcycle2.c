#include <stdio.h>

struct Student {
    int id;
    int* ptr;
    int* qtr;
};

int main(void) {

    int a = 200, b = 400;
    struct Student* sptr = malloc(sizeof(struct Student));

    int k = 199;
    void* q = sptr;
    void* p, *r;


    sptr->id = 500;
    sptr->ptr = &a;
    sptr->qtr = &b;

        
    p = q;
    r = &(((struct Student*)p)->ptr);


    // Strong update
    r = &k;
    q = r;

    
    printf("%d\n", *(int*)r);
    
    return 0;
}

