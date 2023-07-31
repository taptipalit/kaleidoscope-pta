#include<stdio.h>

struct Student {
    int id;
    int age;
    void (*fptr)();
    char name[10];
};

void func() {
    printf("func\n");
}

void gunc() {
    printf("gunc\n");
}

void aunc() {
    printf("aunc\n");
}

struct Student sarr[] = {
    { 10,
        20,
        &func,
        "john"},
    { 20,
        30,
        &gunc,
        "peter"},
    { 30,
        40,
        &aunc,
        "theil"}};

int main(void) {
    // Typical handling of array types
    struct Student *s = &sarr;


    for (int i = 0; i < 3; i++) {
        (*(s[i].fptr))();
    }
    return 0;
}
