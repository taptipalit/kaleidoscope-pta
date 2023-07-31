#include <stdio.h>

typedef struct protocol {
    int id;
    void (*open_callback)();
    void (*close_callback)();
    int end;
} protocol;


void ssl_open() { printf("ssl open\n"); }
void ssl_close() { printf("ssl close\n"); }

void nossl_open() { printf("nossl open\n"); }
void nossl_close() { printf("nossl close\n"); }

void http_open() { printf("http open\n"); }
void http_close() { printf("http close\n"); }

protocol ssl_prot = {
    10,
    ssl_open,
    ssl_close
};

protocol nossl_prot = {
    20,
    nossl_open,
    nossl_close
};


protocol http_prot = {
    30,
    http_open,
    http_close
};

protocol* protocols[] = {
    &ssl_prot,
    &nossl_prot,
    &http_prot,
    NULL
};

protocol* get_proto(int id) {
    for (protocol** pp = protocols; *pp != (protocol*) NULL; pp++) {
        protocol* p = *pp;
        if (p->id == id) {
            return p;
        }
    }
    return NULL;
}


int main(void) {

    protocol* p = get_proto(30);
    (*(p->open_callback))();
    return 0;
}
