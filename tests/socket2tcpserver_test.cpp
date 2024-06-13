#include "socket/socket.h"
#include "socket/tcpserver.h"
#include <thread>
#include <iostream>
#include <string>
void testtpcserver(){
    C_RPC::TCPServer server("127.0.0.1",5829);
    server.start();
    server.run();
}

void testsocket(int id){
    C_RPC::Socket client;
    client.create();
    C_RPC::Address severaddr ("127.0.0.1",5829);
    client.connect(severaddr);
    std::string a("aaa");
    std::string b("bbb");
    std::string c("ccc");
    a+=(id+'0');
    b+=(id+'0');
    c+=(id+'0');
    std::string reca,recb,recc;

    std::cout<<"sendzise="<<client.send(a)<<std::endl;
    client.receive(reca,4);
    std::cout<<"client"<<id<<" receive:"<<reca<<std::endl;
    client.send(b);
    client.send(c);
    client.receive(recb,4);
    std::cout<<"client"<<id<<" receive:"<<recb<<std::endl;
    client.receive(recc,4);
    std::cout<<"client"<<id<<" receive:"<<recc<<std::endl;

}

int main(){
    //std::thread server(testtpcserver);
    std::thread client1(testsocket,1);
    std::thread client2(testsocket,2);
    std::thread client3(testsocket,3);
    sleep(10);
    return 0;
}