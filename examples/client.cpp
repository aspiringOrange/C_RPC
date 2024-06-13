#include "rpc/rpc_client.h"
#include <iostream>
#include <string>

void test(){
    C_RPC::RpcCLient client;
    client.start();
    std::string fname="add";
    int x=10000;
    int y=2300;
    int cnt=10;
    while(cnt--){
        int res = client.call<int>(fname,x,--y);
        std::cout<<"res = "<<res<<std::endl;
    }
}


int main(){
    std::shared_ptr<C_RPC::Scheduler> ptr(new C_RPC::Scheduler(3));
    ptr->start();
    sleep(1);
    while(!ptr->addTask(test));
    sleep(100);
    return 0;
}