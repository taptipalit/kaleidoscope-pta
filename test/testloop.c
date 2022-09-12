#include <stdio.h>

void run(void) {
    printf("This is horrible\n");
}

int main (void) {
    for (int i = 0; i < 100; i++) {
        printf("This is something that happens\n");
    }
    run();
}
