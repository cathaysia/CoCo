#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>

#include "coroutine.hpp"

void func1(void* args) {
    puts("11");
    yield();
    int* a = (int*)args;
    printf("func1:%d\n", a[0]);
}
void func2(void* args) {
    puts("22");
    yield();
    int* a = (int*)args;
    printf("func2:%d\n", a[0]);
}

void func(void* args) {
    puts("hello, world");
}

int main(int argc, char* argv[]) {
    int       args1[] = { 1 };
    int       args2[] = { 2 };
    Schedule* s       = Schedule::getInstance();
    cid       id1     = s->push(func1, &args1);
    cid       id2     = s->push(func2, &args2);
    s->run(id1);
    s->run(id2);
    while(!s->fished()) {
        resume(id2);
        resume(id1);
    }
    puts("over");
    return 0;
}
