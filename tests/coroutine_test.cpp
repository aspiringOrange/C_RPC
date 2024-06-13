#include "coroutine/coroutine.h"
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



int main(){
    std::shared_ptr<spdlog::logger> logger = LOG_NAME("coroutine");
    C_RPC::Coroutine::StartCoroutine();
    std::shared_ptr<C_RPC::Coroutine> coroutine1(new C_RPC::Coroutine(test1));
    std::shared_ptr<C_RPC::Coroutine> coroutine2(new C_RPC::Coroutine(test1));
    coroutine1->resume();
    printf("main\n");
    coroutine2->resume();
    printf("main\n");
    coroutine2->resume();
    coroutine1->resume();
    return 0;
}