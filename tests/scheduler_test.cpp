#include "coroutine/scheduler.h"
#include <iostream>
void test1(){
    std::cout<<"this is test1"<<std::endl;
    std::cout<<"now is coroutine"<<C_RPC::Coroutine::GetCurrentCoroutine()->Getid()<<std::endl;
    std::cout<<"maincoroutine is coroutine"<<C_RPC::Coroutine::GetMainCoroutine()->Getid()<<std::endl;
    std::cout<<"yield\n";
    C_RPC::Coroutine::GetCurrentCoroutine()->yield();
    std::cout<<"yield_end\n";
    std::cout<<"now is coroutine"<<C_RPC::Coroutine::GetCurrentCoroutine()->Getid()<<std::endl;
}

void test2(){
    std::cout<<"this is test2"<<std::endl;
    std::cout<<"now is coroutine"<<C_RPC::Coroutine::GetCurrentCoroutine()->Getid()<<std::endl;
    std::cout<<"maincoroutine is coroutine"<<C_RPC::Coroutine::GetMainCoroutine()->Getid()<<std::endl;
}


int main(){
    
    std::shared_ptr<C_RPC::Scheduler> ptr(new C_RPC::Scheduler(3));
    ptr->start();
    sleep(1);
    while(!ptr->addTask(test1));
    ptr->addTask(test2);
    ptr->addTask(test2);
    sleep(2);
    return 0;
}