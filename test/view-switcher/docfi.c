#include <stdio.h>
#include <stdlib.h>

//int startCFI = 0;

//extern int serving_started;

//void annotate(void* ch) {}
void checkCFI(unsigned long* arr, int len_arr, unsigned long* tgt) {
/*
    if (!startCFI) {
        return;
    }
*/

    int found = 0;
    for (int i = 0; i < len_arr; i++) {
        if (tgt == *arr) {
            found = 1;
            break;
        } else {
            arr++;
        }
    }
    if (!found) {
        fprintf(stderr, "CFI error\n");
        exit (-1);
    }
}

void blockCFI() {
/*
    if (!startCFI) {
        return;
    }
*/
    fprintf(stderr, "CFI error. This callsite shouldn't be triggered at all\n");
    exit (-1);
}

/*
void start_serving_phase() {
//    serving_started = 1;
}
*/

