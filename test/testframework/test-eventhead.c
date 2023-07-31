#include <stdio.h>

typedef struct event {
    int id;
    int age;
    void (*fptr)();
} event;

typedef struct eventloop {
    int id;
    event* ehead;
} eventloop;

typedef struct server {
    int id;
    eventloop* el;
} server;

void func() {
    printf("here!\n");
}

void initEl(server* s) {
    s->el->ehead = malloc(sizeof(event));
    s->el->ehead->fptr = func;
}

int main(void) {
    server* s = malloc(sizeof(server));
    s->el = malloc(sizeof(eventloop));
    initEl(s);
    (*(s->el->ehead->fptr))();
    return 0;
}
