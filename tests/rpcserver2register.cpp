#include "rpc/rpc_server.h"
#include <iostream>
#include <string>

int add(int x,int y){
    return x-y;
}

void test(){
    std::vector<sockaddr_in> LocalAddress;
    C_RPC::Address::getLocalAddress(LocalAddress);
    C_RPC::RpcServer server(inet_ntoa(LocalAddress[0].sin_addr),5830);
    server.start();
    server.registerFunc("add",add);
    server.run();
}


int main(){
    std::shared_ptr<C_RPC::Scheduler> ptr(new C_RPC::Scheduler(3));
    ptr->start();
    sleep(1);
    while(!ptr->addTask(test));
    sleep(100);
    return 0;
}