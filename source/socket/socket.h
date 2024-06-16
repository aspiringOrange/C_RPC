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
/**
  * @brief Socket类,封装了基本socket编程，使用了非阻塞模式
  */
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
    //文件描述符
    int32_t m_sockfd;
    bool m_is_connected{false};
}; 
} //namespace C_RPC
#endif //_SOCKET_H_