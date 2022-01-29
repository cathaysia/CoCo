#pragma once

#include <deque>
extern "C" {
#include <ucontext.h>
}

constexpr const int kSTACK_SZIE       = 1024 * 128;
constexpr const int kMAX_UTHREAD_SIZE = 1024;

struct Schedule;
using CoFun = void (*)(Schedule *s, void *args);
using cid   = int;

struct Coroutine {
    enum State { Stopped, Ready, Running, Suspend };

    CoFun      func_;
    void      *args_;
    ucontext_t ctx_;
    State      state_;
    char       stack_[kSTACK_SZIE];
    Coroutine(CoFun func, void *args);
};

struct Schedule {
    ucontext_t              main_;
    int                     cur_co_;
    std::deque<Coroutine *> coroutines_;

    static Schedule *getInstance();
    static void      mainCo();

    cid              push(CoFun fun, void *args);
    bool             finished();
    void             run(cid id);
    Coroutine::State state(cid id);
    ~Schedule();

private:
    Schedule() = default;
};

void yield();
void resume(cid id);
