#include "rpc/rpc_client.h"
#include <iostream>
#include <string>

void test(){
    C_RPC::RpcCLient client;
    client.start();
}


int main(){
    std::shared_ptr<C_RPC::Scheduler> ptr(new C_RPC::Scheduler(3));
    ptr->start();
    sleep(1);
    while(!ptr->addTask(test));
    sleep(100);
    return 0;
}