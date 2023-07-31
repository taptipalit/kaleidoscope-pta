#include <stdio.h>

void func1() { printf("func 1\n"); }
void func2() { printf("func 2\n"); }
void func3() { printf("func 3\n"); }
void func4() { printf("func 4\n"); }
void func5() { printf("func 5\n"); }
void func6() { printf("func 6\n"); }
void func7() { printf("func 7\n"); }
void func8() { printf("func 8\n"); }
void func9() { printf("func 9\n"); }
void func10() { printf("func 10\n"); }

struct Object {
    void (*f1)();
    void (*f2)();
    void (*f3)();
    void (*f4)();
    void (*f5)();
    void (*f6)();
    void (*f7)();
    void (*f8)();
    void (*f9)();
    void (*f10)();
};

struct Object o = {
    func1,
    func2,
    func3,
    func4,
    func5,
    func6,
    func7,
    func8,
    func9,
    func10
};


int main(void) {

    // Is PWC and causes infinite derivations
    struct Object* p = &o; //= malloc(sizeof(struct Object));
    
    p = (struct Object*) &(p->f4);

    (*(p->f3))();

    /*
    // Is not a PWC, and doesn't cause infinite derivations
    char* p = &o;
    p = p + 8;

    (*(((struct Object*)p)->f1))();
    */

    return 0;
}
