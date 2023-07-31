#include <stdio.h>

struct A {
    int id;
    char data[1024];
};


void process(char* data, int len) {
    for (int i = 0; i < len; i++) {
        *(data + i) = i;
    }
}

int main(void) {
    struct A objA;
    char* ptr = &objA;

    for (int i = 0; i < sizeof(objA); i++) {
        char c = ptr[i];
        printf("%c\n", c);
    }

    process(objA.data, sizeof(objA.data));
    return 0;
}
