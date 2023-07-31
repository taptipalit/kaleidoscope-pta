#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct plugin {
    int id;
    int *ptr;
    void (*cb_open)();
    void (*cb_close)();
} plugin;

typedef struct server {
    int id;
    struct { void *ptr; uint32_t used; uint32_t size; } plugins;
} server;

static void plugins_register(server *srv, plugin *p) {
    plugin **ps;
    ps = srv->plugins.ptr;
    ps[srv->plugins.used++] = p;
//    ps[0] = p;
}

void my_open() { printf("My open\n"); }
void my_close() { printf("My close\n"); }

void your_open() { printf("Your open\n"); }
void your_close() { printf("Your close\n"); }

int main(void) {
    // Initialize srv
    server* srv = malloc(sizeof(server));
    //srv->id = 100;
    srv->plugins.ptr = malloc(10*sizeof(plugin));

    // Another plugin
    plugin* p = malloc(sizeof(plugin));
    //q->id = 1;
    p->cb_open = my_open;
    p->cb_close = my_close;
    plugins_register(srv, p);

    // Another plugin
    plugin* q = malloc(sizeof(plugin));
    //q->id = 1;
    q->cb_open = your_open;
    q->cb_close = your_close;
    plugins_register(srv, q);

    // Sanity
    int i = 0;
    plugin* r = ((plugin**)(srv->plugins.ptr))[i]; // Vargep
//    plugin* p = ((plugin**)(srv->plugins.ptr))[0]; // Vargep
    (*(r->cb_open))();
    return 0;
}
