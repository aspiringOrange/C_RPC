#include "coroutine/scheduler.h"
#include <iostream>
static std::shared_ptr<spdlog::logger> logger = LOG_NAME("test");
void test1(){
    logger->info(fmt::format("this is test1:now is coroutine{},maincoroutine is coroutine{}",C_RPC::Coroutine::GetCurrentCoroutine()->Getid(),C_RPC::Coroutine::GetMainCoroutine()->Getid()));
    logger->info(fmt::format("this is test1:yield"));
    C_RPC::Coroutine::GetCurrentCoroutine()->yield();
    logger->info(fmt::format("this is test1:yield_end"));
    logger->info(fmt::format("this is test1:now is coroutine{},maincoroutine is coroutine{}",C_RPC::Coroutine::GetCurrentCoroutine()->Getid(),C_RPC::Coroutine::GetMainCoroutine()->Getid()));
 }

void test2(){
    logger->info(fmt::format("this is test2:now is coroutine{},maincoroutine is coroutine{}",C_RPC::Coroutine::GetCurrentCoroutine()->Getid(),C_RPC::Coroutine::GetMainCoroutine()->Getid()));
}


int main(){
    
    std::shared_ptr<C_RPC::Scheduler> ptr(new C_RPC::Scheduler(3));
    ptr->start();
    sleep(1);
    C_RPC::Scheduler::GetTimer()->addTimeEvent(1000,test1);
    C_RPC::Scheduler::GetTimer()->addTimeEvent(2000,test2);
    std::shared_ptr<C_RPC::TimeEvent> timer3 = C_RPC::Scheduler::GetTimer()->addTimeEvent(1500,[&timer3](){
        static int cnt=0;
        cnt++;
        logger->info(fmt::format("this is test3:now is coroutine{},maincoroutine is coroutine{},cnt={}",C_RPC::Coroutine::GetCurrentCoroutine()->Getid(),C_RPC::Coroutine::GetMainCoroutine()->Getid(),cnt));

        if(cnt==3){
            timer3->cancel();
        }
    },true);
    sleep(10);
    return 0;
}