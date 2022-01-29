#pragma once

#include <deque>
#include <functional>

extern "C" {
#include <ucontext.h>
}

const constexpr int kSTACK_SIZE       = 1024;
const constexpr int kMAX_UTHREAD_SIZE = 1024;

class Schedule;

using CoFunc = std::function<void(void*)>;
using cid    = int;

struct Coroutine {
    enum State { Stopped, Ready, Running, Suspend };
    State      state;
    CoFunc     func;
    void*      args;
    char       stack[kSTACK_SIZE];
    ucontext_t ctx;
    Coroutine(CoFunc func, void* args);
    Coroutine(Coroutine const&) = delete;
    Coroutine operator=(Coroutine const&) = delete;
};

struct Schedule {
    ucontext_t             main;
    int                    curCo;
    std::deque<Coroutine*> data;

    // 创建协程并返回协程号
    // 会复用停止的协程的协程号
    cid              push(CoFunc func, void* args);
    void             run(cid id);
    bool             fished();
    Coroutine::State state(cid id);

    static Schedule* getInstance();
    // 删除拷贝函数和移动函数
    Schedule(Schedule const&) = delete;
    Schedule(Schedule&&)      = delete;

    ~Schedule();

private:
    Schedule() = default;
};

static void main_co(Schedule* s);
void        yield();
void        resume(int id);
