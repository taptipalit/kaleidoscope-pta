#include <stdio.h>
#include <stdarg.h>
 
void vfn1(int id, ...)
{
    va_list args;
    va_start(args, id);
 
    void* p = va_arg(args, void*);
    void (*fptr)() = p;

    (*fptr)();
    /*
    while (*fmt != '\0') {
        if (*fmt == 'd') {
            int i = va_arg(args, int);
            printf("%d\n", i);
        } else if (*fmt == 'c') {
            // A 'char' variable will be promoted to 'int'
            // A character literal in C is already 'int' by itself
            int c = va_arg(args, int);
            printf("%c\n", c);
        } else if (*fmt == 'f') {
            double d = va_arg(args, double);
            printf("%f\n", d);
        }
        ++fmt;
    }
    */
 
    va_end(args);
}

void vfn2(int id, ...) {
    va_list args;
    va_start(args, id);
 
    void* p = va_arg(args, void*);
    void (*fptr)(int) = p;

    (*fptr)(20);
    /*
    while (*fmt != '\0') {
        if (*fmt == 'd') {
            int i = va_arg(args, int);
            printf("%d\n", i);
        } else if (*fmt == 'c') {
            // A 'char' variable will be promoted to 'int'
            // A character literal in C is already 'int' by itself
            int c = va_arg(args, int);
            printf("%c\n", c);
        } else if (*fmt == 'f') {
            double d = va_arg(args, double);
            printf("%f\n", d);
        }
        ++fmt;
    }
    */
 
    va_end(args);

}
 
void func() {
    printf("Happy Wednesday\n");
}

void func2(int a) {
    printf("Happy birthday: %d\n", a);
}

int main(void)
{
    vfn1(10, func);
    vfn2(10, func2);
    return 0;
}

