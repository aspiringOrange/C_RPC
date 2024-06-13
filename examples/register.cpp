#include "rpc/rpc_register.h"
#include <iostream>
#include <string>

void test(){
    C_RPC::RpcRegister Register("127.0.0.1",5829);
    Register.start();
    Register.run();
}


int main(){
    std::shared_ptr<C_RPC::Scheduler> ptr(new C_RPC::Scheduler(3));
    ptr->start();
    sleep(1);
    while(!ptr->addTask(test));
    sleep(10);
    return 0;
}