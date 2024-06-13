#include "coroutine/coroutine.h"
#include "coroutine/coroutinesync.h"
#include <iostream>

static int cnt =0;
C_RPC::CoroutineMutex mutex;
C_RPC::CoroutineCV cv;
void test1(){
    std::cout<<"this is test1: now is coroutine"<<C_RPC::Coroutine::GetCurrentCoroutine()->Getid()<<std::endl;
    mutex.lock();
    int x = ++cnt;
    std::cout<<"this is test1:cnt="<<x<<std::endl;
    if(cnt == 3){
        cv.notify();
    }
    mutex.unlock();
}

void test2(){
    std::cout<<"this is test2: now is coroutine"<<C_RPC::Coroutine::GetCurrentCoroutine()->Getid()<<std::endl;
    mutex.lock();

    if(cnt<3) 
        cv.wait(mutex);
    int x = ++cnt;
    std::cout<<"this is test2:cnt="<<x<<std::endl;
     if(cnt == 4){
        cv.notifyAll();
    }
    mutex.unlock();
}
void test(){
    std::shared_ptr<C_RPC::Coroutine> coroutine1(new C_RPC::Coroutine(test1));
    std::shared_ptr<C_RPC::Coroutine> coroutine2(new C_RPC::Coroutine(test1));
    std::shared_ptr<C_RPC::Coroutine> coroutine3(new C_RPC::Coroutine(test1));
    std::shared_ptr<C_RPC::Coroutine> coroutine4(new C_RPC::Coroutine(test2));
    std::shared_ptr<C_RPC::Coroutine> coroutine5(new C_RPC::Coroutine(test2));
    std::shared_ptr<C_RPC::Coroutine> coroutine6(new C_RPC::Coroutine(test2));

    std::shared_ptr<C_RPC::Scheduler> ptr(new C_RPC::Scheduler(3));
    ptr->start();
    sleep(1);
    while(!ptr->addTask(std::move(coroutine4)));
    ptr->addTask(std::move(coroutine5));
    ptr->addTask(std::move(coroutine6));
    ptr->addTask(std::move(coroutine1));
    ptr->addTask(std::move(coroutine2));
    ptr->addTask(std::move(coroutine3));
    sleep(10);
}

int main(){
    test();
    return 0;
}