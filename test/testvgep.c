#include<stdio.h>

struct Student {
    int id;
    int age;
    char name[10];
};

int main(void) {
    // Typical handling of array types
    struct Student *s = malloc(10*sizeof(struct Student));

    int k = 200;
    int id = s[k].id;
    return 0;
}
