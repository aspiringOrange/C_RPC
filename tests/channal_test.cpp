#include "coroutine/coroutine.h"
#include "coroutine/coroutinesync.h"
#include "coroutine/coroutinechannal.h"
#include <iostream>
#include <string>
C_RPC::CoroutineChannal<std::string> channal(3);
void test1(){
    std::cout<<"this is test1: now is coroutine"<<C_RPC::Coroutine::GetCurrentCoroutine()->Getid()<<std::endl;
    std::string a="aaa";
    std::string b="bbb";
    std::string c="ccc";
    channal<<a<<b<<c;
}

void test2(){
    std::cout<<"this is test2: now is coroutine"<<C_RPC::Coroutine::GetCurrentCoroutine()->Getid()<<std::endl;
    std::string a,b,c;
    channal>>a>>b>>c;
    std::cout<<"read:"<<a<<" "<<b<<" "<<c<<std::endl;
}
void test(){
    C_RPC::Coroutine::StartCoroutine();//确保channal析构成功
    std::shared_ptr<C_RPC::Coroutine> coroutine1(new C_RPC::Coroutine(test1));
    std::shared_ptr<C_RPC::Coroutine> coroutine2(new C_RPC::Coroutine(test2));

    std::shared_ptr<C_RPC::Scheduler> ptr(new C_RPC::Scheduler(1));
    ptr->start();
    sleep(1);
    while(!ptr->addTask(std::move(coroutine1)));
    ptr->addTask(std::move(coroutine2));
    sleep(10);
}

int main(){
    test();
    return 0;
}