// 首先实现address类,保存服务器和注册中心的地址
// 然后实现TCPserver类，采用io多路复用，同时采用非阻塞scoket，这样使用accept接受连接时可以hook提交到schadule???
// 然后实现clinet类,拥有连接，收发信息功能
// 这时候可以测试了

//应该要设置一个专门的协程负责从队列中读数据进行发送，因为我们采用协程，使用了非阻塞socket，当不同协程复用连接时，可能导致数据流乱序，如果加锁，那么socket缓冲区满时会导致系统性能卡死
//应该要设置一个专门的协程负责接受数据放到channal，因为1、客户端：我们采用协程，不能让一个rpc调用的协程阻塞在网络io，要让他yield，然后等接受协程唤醒他
//                                                   2、多协程读取同一个socket时冲突
//参考：https://blog.csdn.net/JACK_SUJAVA/article/details/128682192

// 然后实现连接池，保存所有socket
// 然后实现连接中心
// 测试

// 然后实现protocal协议以及序列化类，实现基本rpc调用
// 测试
#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring> 
#include <vector>
#include <sys/epoll.h> 
#include "address.h"
#include  "util/serializer.h"
#include "rpc/rpc_protocol.h"
#include "coroutine/scheduler.h"
namespace C_RPC{
class Socket {
public:
    Socket() : m_sockfd(-1) {}

    ~Socket() {
        if (m_sockfd != -1) {
            close();
            m_is_connected = false;
        }
    }

    int32_t getSocketFD() { return m_sockfd;}

    bool isConnected(){ return m_is_connected;}

    bool create( int32_t family = AF_INET, int32_t type = SOCK_STREAM);

    bool setNonBlocking(bool nonBlocking);

    bool setSendTimeout(int seconds);

    bool setReceiveTimeout(int seconds);

    bool connect(const Address& address);

    bool bind(const Address& address);

    bool listen(int32_t backlog = SOMAXCONN);

    bool accept(Socket& newSocket);

    void close();

    ssize_t send(const std::string& data);

    ssize_t send(const void* buffer, size_t length);

    ssize_t send(const Serializer& s);

    ssize_t send(const Message& s);

    ssize_t receive(std::string& data, size_t size);

    ssize_t receive(void* buffer, size_t length);

    ssize_t receive(Message& s);

    ssize_t receive(Serializer& s, size_t size);



private:
    int32_t m_sockfd;
    bool m_is_connected{false};
};
}
#endif