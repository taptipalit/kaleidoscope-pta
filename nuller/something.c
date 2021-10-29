#include <stdio.h>

#define SENSITIVE __attribute__((annotate("sensitive")))

struct Student {
    SENSITIVE int a;
    int b;
    int c;
};


int main(void) {
    struct Student s;
    s.a = 10;
    s.b = 10;
    s.c = 10;
    printf("hello!\n");
    return 0;
}
