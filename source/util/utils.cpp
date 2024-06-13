
#include "utils.h"
#include "../coroutine/scheduler.h"
namespace C_RPC{
uint64_t GetCurrentMS(){
    struct timeval tm;
    gettimeofday(&tm,0);
    return tm.tv_sec * 1000ul + tm.tv_usec / 1000;
}

void coroutine_sleep(uint64_t ms){
    ENSURE(Coroutine::GetCurrentCoroutine()!=nullptr);
    ENSURE(Coroutine::GetCurrentCoroutine()!=Coroutine::GetCurrentCoroutine());

    std::shared_ptr<Coroutine> coroutine = Coroutine::GetCurrentCoroutine();
    ENSURE(Scheduler::GetInstance()!=nullptr);
    Scheduler::GetInstance()->GetTimer()->addTimeEvent(ms,[coroutine]() {
        std::shared_ptr<Coroutine> tmp = coroutine;
        Scheduler::GetInstance()->addTask(std::move(tmp));
    });
    coroutine->yield();
}

}