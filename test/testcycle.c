#include <stdio.h>

struct Student {
    int id;
    void (*ptr)();
    void (*qtr)();
};

void func() {}
void gunc() {}

void dothis(void (*variantFptr)()) {
    (*variantFptr)();
}

int main(void) {

    //int a = 200, b = 400;
    struct Student* sptr = malloc(10*sizeof(struct Student));

    int k = 199;
    void* q = sptr;
    void* p, *r;

    sptr[0]->id = 500;
    sptr[0]->ptr = func;
    sptr[0]->qtr = gunc;

    p = q;
    r = &(((struct Student*)p)->ptr);

    // Strong update
    //r = &k; // Remove this to trigger invariant flip
    q = r;
    
    printf("%d\n", *(int*)r);

    void (*myFuncPtr)() = sptr->ptr;

    dothis(myFuncPtr);
    return 0;
}

