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
    cid       id1     = s->push(func1, args1);
    cid       id2     = s->push(func2, args2);
    while(!s->finished()) {
        for(int i = 0; i < s->coroutines_.size(); ++i) {
            switch(s->coroutines_[i]->state_) {
                case Coroutine::Ready: s->run(i); break;
                case Coroutine::Suspend: resume(i); break;
                default: break;
            }
        }
    }
    puts("main over");
    return 0;
}
