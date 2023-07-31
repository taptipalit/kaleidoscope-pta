#include <stdio.h>

void g1() {printf("g1\n");}
void g2() {printf("g2\n");}
void g3() {printf("g3\n");}
void g4() {printf("g4\n");}
void g5() {printf("g5\n");}
void g6() {printf("g6\n");}
void g7() {printf("g7\n");}
void g8() {printf("g8\n");}

struct Zero {
    int id;
    void (*f1)();
    void (*f2)();
    int age;
};

struct One {
    int id;
    struct Zero* zeroes;
};

struct Two {
    int id;
    struct One* many;
    int age;
};

int main(void) {
    struct Two *two = malloc(sizeof (struct Two));
    two->many = malloc(4*sizeof(struct One));

    for (int i = 0; i < 4; i++) {
        two->many[i].zeroes = malloc(5*sizeof(struct Zero));
        for (int j = 0; j < 5; j++) {
            two->many[i].zeroes[j].f1 = g2;
            two->many[i].zeroes[j].f2 = g4;
        }
    }

    (*(two->many[2].zeroes[4].f2))();
    return 0;
}
