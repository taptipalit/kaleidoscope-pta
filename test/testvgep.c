#include<stdio.h>

struct Student {
    int id;
    int age;
    char name[10];
};

int main(void) {
    // Typical handling of array types
    struct Student *s = malloc(sizeof(struct Student));
    char* arbitraryPtr = s;
    char* myPtr = s;

    int k = 200;
    int i = 121;
    int id = *(arbitraryPtr + i);
    printf("hello world\n");
    int id2 = *(myPtr + i);
    return 0;
}
