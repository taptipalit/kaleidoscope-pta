#include<stdio.h>

struct Student {
    int id;
    int age;
    char name[10];
    void (*fptr)();
    void (*gptr)();
};

void func() { printf("func\n"); }
void gunc() { printf("gunc\n"); } 

int main(void) {
    // Typical handling of array types
    struct Student *s = malloc(sizeof(struct Student));
    s->fptr = func;
    s->gptr = gunc;

    char* arbitraryPtr = s;
    char* myPtr = s;

    int k = 200;
    int i = 121;
    int id = *(arbitraryPtr + i);
    printf("hello world\n");
    int id2 = *(myPtr + i);

    (*(s->fptr))();
    return 0;
}
