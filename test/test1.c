#include <stdio.h>

void fun () {
    printf("This is fun\n");
}

void pun() {
    printf("This is a pun\n");
}

typedef struct module_t {
    int id;
    void (*fptr1)();
    void (*fptr2)();
} module_t;

static module_t myGlobal = {
    11,
    fun,
    pun,
};

int main(void) {
    int a = 10;
    int *p = &a;

    module_t* mod;
    mod = &myGlobal;

    mod->fptr1();
    return 0;
}
