#include "coroutine.hpp"

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

    s          = new Schedule;
    s->cur_co_ = -1;
    return s;
}
Coroutine::Coroutine(CoFun func, void *args) : state_(Coroutine::Ready), func_(func), args_(args), stack_ { 0 } {
    getcontext(&ctx_);
    ctx_.uc_stack.ss_sp    = stack_;
    ctx_.uc_stack.ss_size  = kSTACK_SZIE;
    ctx_.uc_stack.ss_flags = 0;
    ctx_.uc_link           = &(Schedule::getInstance()->main_);
    makecontext(&ctx_, &Schedule::mainCo, 0);
}

cid Schedule::push(CoFun func, void *args) {
    int i = 0;
    for(i = 0; i < coroutines_.size(); i++) {
        if(coroutines_[i]->state_ == Coroutine::Stopped) {
            delete coroutines_[i];
            Coroutine *c   = new Coroutine(func, args);
            coroutines_[i] = c;
            break;
        }
    }
    if(i == coroutines_.size()) {
        Coroutine *c = new Coroutine(func, args);
        coroutines_.push_back(c);
    }
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
    if(id < 0 || id > s->coroutines_.size()) return;

    Coroutine *c = s->coroutines_[id];
    if(c && c->state_ == Coroutine::Suspend) {
        c->state_  = Coroutine::Running;
        s->cur_co_ = id;
        swapcontext(&(s->main_), &(c->ctx_));
    }
}

Coroutine::State Schedule::state(cid id) {
    if(id < 0 || id >= coroutines_.size()) return Coroutine::Stopped;
    Coroutine *c = coroutines_[id];
    return c ? c->state_ : Coroutine::Stopped;
}

bool Schedule::finished() {
    if(cur_co_ != -1) return false;
    if(std::any_of(coroutines_.begin(), coroutines_.end(), [](Coroutine const *c) {
           return c && c->state_ != Coroutine::Stopped;
       }))
        return false;
    return true;
}

void Schedule::run(cid id) {
    if(state(id) == Coroutine::Stopped) return;

    Coroutine *c = coroutines_[id];
    c->state_    = Coroutine::Running;
    cur_co_      = id;
    swapcontext(&main_, &(c->ctx_));
}

Schedule::~Schedule() {
    Coroutine *c = nullptr;
    for(int i = 0; i < coroutines_.size(); i++) {
        c = coroutines_[i];
        if(!c) continue;
        coroutines_[i] = nullptr;
        delete c;
    }
}
