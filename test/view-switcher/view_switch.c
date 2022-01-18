#include <stdio.h>

void switch_view(void) {
    fprintf(stderr,"Invariant violated\n");
    fflush(stderr);
}
