#include "coroutine.hpp"

void mainCo() {
    Schedule* s = Schedule::getInstance();
    if(s->curCo == -1) return;

    Coroutine* c = s->data[s->curCo];
    c->func(c->args);
    c->state = Coroutine::Stopped;
    s->curCo = -1;
}

Coroutine::Coroutine(CoFunc func, void* args) : state(State::Ready), func(func), args(args), stack { 0 } { 
    Schedule* s = Schedule::getInstance();
    getcontext(&ctx);
    ctx.uc_stack.ss_sp    = stack;
    ctx.uc_stack.ss_size  = kSTACK_SIZE;
    ctx.uc_stack.ss_flags = 0;
    ctx.uc_link           = &(s->main);
    makecontext(&ctx, mainCo, 0);
}

Schedule* Schedule::getInstance() {
    static Schedule* s = nullptr;
    if(!s) {
        s        = new Schedule;
        s->curCo = -1;
        getcontext(&(s->main));
    }
    return s;
}

cid Schedule::push(CoFunc func, void* args) {
    int i = 0;
    for(; i < data.size(); ++i) {
        if(data[i]->state == Coroutine::Stopped) {
            delete data[i];
            data[i] = new Coroutine(func, args);
        }
    }
    if(i == data.size()) {
        Coroutine* c = new Coroutine(func, args);
        data.push_back(c);
    }
    return i;
}

void yield() {
    Schedule* s = Schedule::getInstance();
    if(s->curCo == -1) return;

    Coroutine* c = s->data[s->curCo];
    c->SaveContext();
    c->state = Coroutine::Suspend;
    s->curCo = -1;
    swapcontext(&(c->ctx), &(s->main));
}

void resume(int id) {
    Schedule* s = Schedule::getInstance();
    if(id < 0 || id > s->data.size()) return;

    Coroutine* c = s->data[id];
    if(!c || c->state != Coroutine::Suspend) return;
    c->state = Coroutine::Running;
    s->curCo = id;
    swapcontext(&(s->main), &(c->ctx));
}

Coroutine::State Schedule::state(cid id) {
    if(id < 0 || id >= data.size()) return Coroutine::Stopped;
    Coroutine* c = data[id];
    return c ? c->state : Coroutine::Stopped;
}
bool Schedule::fished() {
    if(this->curCo != -1) return -1;
    for(Coroutine const* it: data) {
        if(it && it->state != Coroutine::Stopped) return false;
    }
    return true;
}

void Schedule::run(cid id) {
    if(state(id) == Coroutine::Stopped) return;
    Coroutine* c = this->data[id];
    c->state     = Coroutine::Running;
    this->curCo  = id;
    swapcontext(&main, &(c->ctx));
}

Schedule::~Schedule() {
    for(int i = 0; i < data.size(); ++i) {
        Coroutine* c = data[i];
        if(!c) continue;
        delete c;
        data[i] = nullptr;
    }
}
