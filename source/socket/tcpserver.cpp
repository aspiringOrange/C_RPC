


#include "tcpserver.h"
#include <algorithm>
#include "config.h"
namespace C_RPC{

static std::shared_ptr<spdlog::logger> logger = LOG_NAME("tcpserver");
bool TCPServer::start() {
    // 创建监听套接字
    if (!m_listenSocket.create()) { 
        return false; 
    }
    // 设置监听套接字为非阻塞模式
    if (!m_listenSocket.setNonBlocking(true)) {
        return false; 
    }

    // 绑定服务器地址
    Address serverAddress(m_ip, m_port);
    if (!m_listenSocket.bind(serverAddress)) { 
        return false; 
    }

    if (!m_listenSocket.listen()) { 
        return false; // 如果监听失败，返回 false
    }

    // 创建 epoll 实例
    m_epollFD = epoll_create1(0);
    if (m_epollFD == -1) { 
        std::cerr << "Failed to create epoll instance: " << strerror(errno) << "\n";
        return false; 
    }

    epoll_event event;
    // 设置事件类型为可读
    event.events = EPOLLIN;
    event.data.fd = m_listenSocket.getSocketFD();

    // 将监听套接字添加到 epoll 实例
    if (epoll_ctl(m_epollFD, EPOLL_CTL_ADD, m_listenSocket.getSocketFD(), &event) == -1) {
        std::cerr << "Failed to add listen socket to epoll: " << strerror(errno) << "\n"; 
        return false; 
    }

    // 打印服务器启动信息
    std::cout << "Server started on " << m_ip << ":" << m_port << "\n"; 
    return true; 
}

void TCPServer::run() {
    // 最大事件数
    const int MAX_EVENTS = 1024;
    epoll_event events[MAX_EVENTS];

    while (true) { 
        // 等待事件发生
        int eventCount = epoll_wait(m_epollFD, events, MAX_EVENTS, -1); 
        if (eventCount == -1) { // 检查 epoll_wait 是否成功
            std::cerr << "epoll_wait error: " << strerror(errno) << "\n";
            break; 
        }
        // 遍历发生的事件
        for (int i = 0; i < eventCount; ++i) { 
            if (events[i].data.fd == m_listenSocket.getSocketFD()) { // 如果是监听套接字的事件
                logger->info(fmt::format("new connection"));  
                std::shared_ptr<Socket> newSocket(new Socket);
                if (m_listenSocket.accept(*(newSocket.get()))) { // 接受新连接
                    logger->info(fmt::format("new connection accepted"));  
                    newSocket->setNonBlocking(true); // 设置新套接字为非阻塞模式
                    epoll_event event; // 新的 epoll 事件
                    event.events = EPOLLIN; // 设置事件类型为可读
                    event.data.fd = newSocket->getSocketFD(); // 设置事件关联的文件描述符

                    if (epoll_ctl(m_epollFD, EPOLL_CTL_ADD, newSocket->getSocketFD(), &event) == -1) { // 将新套接字添加到 epoll 实例
                        std::cerr << "Failed to add new socket to epoll: " << strerror(errno) << "\n"; // 打印错误信息
                        newSocket->close(); // 关闭新套接字
                    } else {
                        std::cout << "New connection accepted: " << newSocket->getSocketFD() << "\n"; // 打印新连接信息
                        m_clientSockets[newSocket->getSocketFD()]=newSocket; // 将新套接字添加到客户端套接字列表中
                    }
                }
            } else { // 如果是客户端套接字的事件
                logger->info(fmt::format("new data"));  
                auto it = m_clientSockets.find(events[i].data.fd);
                if (it != m_clientSockets.end()) { // 如果找到客户端套接字
                    logger->info(fmt::format("new data accepted"));  
                    std::string data; // 接收数据的缓冲区
                    ssize_t bytesReceived = it->second->receive(data, 4); // 接收数据
                    if (bytesReceived <= 0) { // 如果接收失败或连接关闭
                        if (bytesReceived < 0) { // 如果接收失败
                            std::cerr << "Receive error: " << strerror(errno) << "\n"; // 打印错误信息
                        } else { // 如果连接关闭
                            std::cout << "Connection closed: " << it->second->getSocketFD() << "\n"; // 打印连接关闭信息
                            epoll_ctl(m_epollFD, EPOLL_CTL_DEL, it->second->getSocketFD(), nullptr); // 从 epoll 实例中移除套接字
                            it->second->close(); // 关闭套接字
                            m_clientSockets.erase(it); // 从客户端列表中删除套接字
                        }
                    } else { // 如果接收成功
                        logger->info(fmt::format("new data accessed"));  
                        std::cout << "Received data: " << data << "\n"; // 打印接收到的数据
                        it->second->send(data); // 回显数据到客户端
                    }
                }
            }
        }
    }
}

}