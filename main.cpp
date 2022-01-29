#include "coroutine.hpp"

#include <stdio.h>

void func1(Schedule *s, void *args) {
    puts("11");
    yield();
    int *a = (int *)args;
    printf("func1:%d\n", a[0]);
}

void func2(Schedule *s, void *args) {
    puts("22");

    yield();
    puts("22");
    int *a = (int *)args;
    printf("func2:%d\n", a[0]);
}

int main() {
    int       args1[] = { 1 };
    int       args2[] = { 2 };
    Schedule *s       = Schedule::getInstance();
    cid id1 = s->push(func1, args1);
    cid id2 = s->push(func2, args2);
    s->run(id2);
    s->run(id1);
    while(!s->finished()) {
        resume(id2);
        resume(id1);
    }
    puts("main over");
    return 0;
}
