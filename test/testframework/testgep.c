#include <stdio.h>

struct A {
    int a;
    void (*fptr)();
    void (*gptr)();
};

void func1() {printf("1\n");}
void func2() {printf("2\n");}
void func3() {printf("3\n");}

void gunc1() {printf("g1\n");}
void gunc2() {printf("g2\n");}
void gunc3() {printf("g3\n");}


void (* funcs[3])(void) = {func1, func2, func3}; 
void (* guncs[3])(void) = {gunc1, gunc2, gunc3}; 


int main(void) {
    struct A* arrA = malloc(3*sizeof(struct A));

    for (int i = 0; i < 3; i++) {
        (arrA + i)->gptr = guncs[i];
        (arrA + i)->fptr = funcs[i];
    }
    /*
    (arrA + 2)->gptr = gunc2;
    (arrA + 2)->fptr = func2;
    */

    void (*myfptr)() = (arrA + 2)->fptr;
    (*myfptr)();
    void (*yourfptr)() = (arrA + 2)->gptr;
    (*yourfptr)();
 
    return 0;
}
