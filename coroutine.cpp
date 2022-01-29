#include "coroutine.hpp"
#include <cstdlib>
#include <cstring>

void Schedule::mainCo() {
    Schedule *s  = Schedule::getInstance();
    int       id = s->cur_co_;
    if(id == -1) return;

    Coroutine *c = s->coroutines_[id];
    c->func_(s, c->args_);
    c->state_  = Coroutine::Stopped;
    s->cur_co_ = -1;
}
Schedule *Schedule::getInstance() {
    static Schedule *s = nullptr;
    if(s) return s;

    s              = new Schedule;
    s->cur_co_     = -1;
    s->max_index_  = 0;
    s->coroutines_ = (Coroutine **)malloc(sizeof(Coroutine *) * kMAX_UTHREAD_SIZE);
    memset(s->coroutines_, 0, sizeof(Coroutine *) * kMAX_UTHREAD_SIZE);
    s->max_index_ = 0;
    return s;
}
cid Schedule::push(CoFun func, void *args) {
    Schedule  *s = Schedule::getInstance();
    Coroutine *c = NULL;
    int        i = 0;
    for(i = 0; i < s->max_index_; i++) {
        c = s->coroutines_[i];
        if(c->state_ == Coroutine::Stopped) { break; }
    }
    if(s->max_index_ == i || c == NULL) { s->coroutines_[i] = new Coroutine; }
    c         = s->coroutines_[i];
    c->state_ = Coroutine::Ready;
    c->func_  = func;
    c->args_  = args;
    s->max_index_++;
    getcontext(&(c->ctx_));

    c->ctx_.uc_stack.ss_sp    = c->stack_;
    c->ctx_.uc_stack.ss_size  = kSTACK_SZIE;
    c->ctx_.uc_stack.ss_flags = 0;
    c->ctx_.uc_link           = &(s->main_);

    makecontext(&(c->ctx_), &Schedule::mainCo, 0);
    return i;
}

void yield() {
    Schedule *s = Schedule::getInstance();
    if(s->cur_co_ == -1) return;

    Coroutine *c = s->coroutines_[s->cur_co_];
    c->state_    = Coroutine::Suspend;
    s->cur_co_   = -1;

    swapcontext(&(c->ctx_), &(s->main_));
}

void resume(cid id) {
    Schedule *s = Schedule::getInstance();
    if(id >= 0 && id < s->max_index_) {
        Coroutine *c = s->coroutines_[id];
        if(c != NULL && c->state_ == Coroutine::Suspend) {
            c->state_  = Coroutine::Running;
            s->cur_co_ = id;
            swapcontext(&(s->main_), &(c->ctx_));
        }
    }
}

Coroutine::State Schedule::state(cid id) {
    if(id < 0 || id >= this->max_index_) return Coroutine::Stopped; 
    Coroutine *c = this->coroutines_[id];
    return c ? c->state_ : Coroutine::Stopped;
}

bool Schedule::finished() {
    Schedule *s = Schedule::getInstance();
    int       i = 0;
    if(s->cur_co_ != -1) return false;
    for(i = 0; i < s->max_index_; i++) {
        Coroutine *c = s->coroutines_[i];
        if(c != NULL && c->state_ != Coroutine::Stopped) { return 0; }
    }
    return true;
}

void Schedule::run(cid id) {
    Coroutine::State state = this->state(id);
    if(state == Coroutine::Stopped) { return; }

    Coroutine *c  = this->coroutines_[id];
    c->state_     = Coroutine::Running;
    this->cur_co_ = id;
    swapcontext(&(this->main_), &(c->ctx_));
}

Schedule::~Schedule() {
    int i = 0;
    for(i = 0; i < this->max_index_; i++) {
        Coroutine *c = this->coroutines_[i];
        if(!c) continue;
        this->coroutines_[i] = nullptr;
        delete c;
    }
    free(this->coroutines_);
}
