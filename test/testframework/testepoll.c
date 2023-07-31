#define MAX_EVENTS 5
#define READ_SIZE 10
#include <stdio.h>     // for fprintf()
#include <unistd.h>    // for close(), read()
#include <sys/epoll.h> // for epoll_create1(), epoll_ctl(), struct epoll_event
#include <string.h>    // for strncmp

void func() { printf("this is good\n"); }

struct epoll_event* events;

struct epoll_event* evt1;

typedef struct Object {
    int id;
    void (*fptr)(); // function pointer
} Object;

int fun2(int epoll_fd) {
    int k = 10;
    fun(epoll_fd);

    printf("%d\n", k);
    return 0;

}

int fun(int epoll_fd) {
    int running = 1, event_count, i;
    char read_buffer[READ_SIZE + 1];
    void* p, *q, *r;
    size_t bytes_read;
    while(running)
    {
        printf("\nPolling for input...\n");
        event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, 30000);
        printf("%d ready events\n", event_count);
        for (int i = 0; i < event_count; i++) {
            printf("Reading file descriptor '%d' -- ", events[i].data.fd);
            bytes_read = read(0, read_buffer, READ_SIZE);
            printf("%zd bytes read.\n", bytes_read);
            read_buffer[bytes_read] = '\0';
            printf("Read '%s'\n", read_buffer);

            // calling func
            Object* o2 = events[i].data.ptr;
            void (*fptr)() = o2->fptr;
            (*fptr)();

            if(!strncmp(read_buffer, "stop\n", 5))
                running = 0;
        }
    }
    if(close(epoll_fd))
    {
        fprintf(stderr, "Failed to close epoll file descriptor\n");
        return 1;
    }
    return 0;
}

int main()
{
    struct epoll_event event, event1;
    Object* obj;

    events = malloc(MAX_EVENTS * sizeof(struct epoll_event));
    evt1 = malloc(sizeof(struct epoll_event));

    int epoll_fd = epoll_create1(0);

    if(epoll_fd == -1)
    {
        fprintf(stderr, "Failed to create epoll file descriptor\n");
        return 1;
    }

    event.events = EPOLLIN;
    obj = malloc(sizeof(struct Object));
    Object* optr = obj;
    obj->id = 100;
    obj->fptr = func;



    event.data.ptr = obj;

    void* myPtr = event.data.ptr;

    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &event))
    {
        fprintf(stderr, "Failed to add file descriptor to epoll\n");
        close(epoll_fd);
        return 1;
    }

    fun(epoll_fd);
}
