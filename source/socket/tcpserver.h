#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_

#include <sys/epoll.h> // epoll 系统调用相关库
#include <iostream> // 标准输入输出库
#include <map> 
#include <memory>
#include <sys/epoll.h> // epoll 系统调用相关库
#include <cstring> // 字符串处理库，包括 strerror
#include "socket.h" // Socket 类
#include "address.h" // Address 类

namespace C_RPC{
class TCPServer {
public:
    TCPServer(const std::string& ip, uint16_t port) : m_listenSocket(), m_ip(ip), m_port(port) {}

    ~TCPServer(){
        m_listenSocket.close();
    }

    bool start();

    void run();


    Socket m_listenSocket; // 监听套接字
    std::string m_ip; // 服务器 IP 地址
    uint16_t m_port; // 服务器端口号
    uint16_t m_epollFD; // epoll 文件描述符
    std::map<int32_t,std::shared_ptr<Socket>> m_clientSockets; // 客户端套接字列表
};
}
#endif