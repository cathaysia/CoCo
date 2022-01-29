#include "coroutine.hpp"
#include <cstdlib>
#include <cstring>

void Schedule::mainCo() {
    Schedule *s = Schedule::getInstance();
    if(s->cur_co_ == -1) return;

    Coroutine *c = s->coroutines_[s->cur_co_];
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
Coroutine::Coroutine(CoFun func, void *args) : state_(Coroutine::Ready), func_(func), args_(args) {
    getcontext(&ctx_);
    ctx_.uc_stack.ss_sp    = stack_;
    ctx_.uc_stack.ss_size  = kSTACK_SZIE;
    ctx_.uc_stack.ss_flags = 0;
    ctx_.uc_link           = &(Schedule::getInstance()->main_);
    makecontext(&ctx_, &Schedule::mainCo, 0);
}

cid Schedule::push(CoFun func, void *args) {
    Coroutine *c = nullptr;
    int        i = 0;
    for(i = 0; i < this->max_index_; i++) {
        c = this->coroutines_[i];
        if(c->state_ == Coroutine::Stopped) {
            delete c;
            c = new Coroutine(func, args);
            break;
        }
    }
    if(this->max_index_ == i) {
        c = new Coroutine(func, args);
        ++this->max_index_;
    }
    this->coroutines_[i] = c;
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
