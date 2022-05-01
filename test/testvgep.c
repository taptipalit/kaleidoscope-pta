#include<stdio.h>

struct Student {
    int id;
    int age;
    char name[10];
};

int main(void) {
    // Typical handling of array types
    struct Student *s = malloc(10*sizeof(struct Student));
    char* arbitraryPtr = s;

    int k = 200;
    int i = 121;
    int id = *(arbitraryPtr + i);
    return 0;
}
