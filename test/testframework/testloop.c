#include <stdio.h>
#include <stdint.h>

void run(void) {
    size_t i;	
	uint64_t* s, *d;
	uint64_t a, b;
	s = &a;
	d = &b;
    int n = 10;
    uint64_t c, z;

    for( i = c = 0; i < n; i++, s++, d++ )
    {
        z = ( *d <  c );     *d -=  c;
        c = ( *d < *s ) + z; *d -= *s;
    }

    while( c != 0 )
    {
        z = ( *d < c ); *d -= c;
        c = z; d++;
    }


    printf("This is horrible\n");
}

int main (void) {
   run();
}
