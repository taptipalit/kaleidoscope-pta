#include <stdio.h>
#include <stdlib.h>

void* (*mallock_ptr)(size_t) = malloc;


struct Student {
    int id;
    int* aptr;
    int* bptr;
};

int main(void) {
    int m = 10;
    int n = 10;

    struct Student* sptr = (*mallock_ptr)(sizeof(struct Student));
    //struct Student* pptr = (*mallock_ptr)(sizeof(struct Student));

    sptr->id = 100;
    sptr->aptr = &m;
    sptr->bptr = &n;

    /*
    pptr->id = 300;
    pptr->aptr = &n;
    */
    /*
    int* nyptr = pptr->aptr;
    printf("%d\n", *nyptr);
    */

    char* p = sptr;
    int i = 4000;
    char d = *(p + i);

 
    int* myptr = sptr->aptr;
    printf("%d\n", *myptr);

   

    return 0;
}
