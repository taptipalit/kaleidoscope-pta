#include <stdio.h>

typedef struct Student {
    int id;
    int age;
    char name[100];
} Student;

void* wrapper(int sz) {
    int* p = malloc(sz);
    return p;
    //return malloc(sz);
}

int main(void) {
    int sz = 10*sizeof(Student);
    int unisz = sizeof(Student);

    Student* sptr = malloc(sizeof(Student));
    int* p = malloc(sizeof(int));

    sptr->id = 100;
    Student* marr = malloc(sz);
    Student* parr = wrapper(unisz);

    Student* myStudentOnHeap = wrapper(sizeof(Student));
    Student* sarr = wrapper(10*sizeof(Student));
    int* myIntArr = wrapper(100*sizeof(char));
    printf("%d\n", sptr->id);
    return 0;
}
