#include "raft/raft_node.h"
void test(){
    C_RPC::Raft::RaftNode node(0);
    node.run();
}


int main(){
    std::shared_ptr<C_RPC::Scheduler> ptr(new C_RPC::Scheduler(2));
    ptr->start();
    sleep(1);
    while(!ptr->addTask(test));
    sleep(100);
    return 0;
}