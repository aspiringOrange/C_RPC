#include "socket.h"
#include "config.h"

namespace C_RPC{
static std::shared_ptr<spdlog::logger> logger = LOG_NAME("socket");

bool Socket::create( int32_t family, int32_t type ) {
    m_sockfd = socket(family, type, 0);
    if (m_sockfd == -1) {
        logger->info(fmt::format("Failed to create socket: {}",strerror(errno)));  
        return false;
    }
    return true;
}

bool Socket::setNonBlocking(bool nonBlocking) {
    int32_t flags = fcntl(m_sockfd, F_GETFL, 0);
    if (flags == -1) {
        logger->info(fmt::format("Failed to get socket flags: {}",strerror(errno)));
        return false;
    }
    flags = nonBlocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    if (fcntl(m_sockfd, F_SETFL, flags) == -1) {
        logger->info(fmt::format("Failed to set socket flags: {}",strerror(errno)));
        return false;
    }
    return true;
}

bool Socket::setSendTimeout(int32_t seconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;
    if (setsockopt(m_sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        logger->info(fmt::format("Failed to set send timeout: {}",strerror(errno)));
        return false;
    }
    return true;
}

bool Socket::setReceiveTimeout(int32_t seconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;
    if (setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        logger->info(fmt::format("Failed to set receive timeout: {}",strerror(errno)));
        return false;
    }
    return true;
}

bool Socket::connect(const Address& address) {
    sockaddr_in sockaddress = address.getSockAddr();
    size_t n = ::connect(m_sockfd, (sockaddr *)&sockaddress, sizeof(sockaddress));
    while(errno == EINTR && n <0){
        n = ::connect(m_sockfd, (sockaddr *)&sockaddress, sizeof(address.getSockAddr()));
    }
    if (n < 0) {
        if (errno != EINPROGRESS) {
            logger->info(fmt::format("Failed to connect: {}",strerror(errno)));
            return false;
        }
    }
    
    epoll_event event;
    event.data.fd = m_sockfd;
    event.events = EPOLLOUT | EPOLLIN | EPOLLET;

    int32_t epollFd = epoll_create1(0); // 创建 epoll 实例
    epoll_ctl(epollFd, EPOLL_CTL_ADD, m_sockfd, &event);
    epoll_event events[1];
    int timeoutMs = 10000;
    int32_t nfds = epoll_wait(epollFd, events, 1, timeoutMs);
    ::close(epollFd);
    if (nfds == 0) {
        logger->info(fmt::format("Failed to connect {}: timeout",inet_ntoa(sockaddress.sin_addr)));
        return false;
    }


    if (events[0].events & EPOLLERR || events[0].events & EPOLLHUP) {
        logger->info(fmt::format("Failed to connect2"));
        return false;
    }
    {
    std::vector<sockaddr_in> LocalAddress;
    Address::getLocalAddress(LocalAddress);
    logger->info(fmt::format("{} connect to: {} , fd = {}",inet_ntoa(LocalAddress[0].sin_addr),inet_ntoa(sockaddress.sin_addr),m_sockfd));
    }
    m_is_connected = true;
    return true;
}

bool Socket::bind(const Address& address) {
    sockaddr_in sockaddress = address.getSockAddr();
    if (::bind(m_sockfd, (sockaddr *)&sockaddress, sizeof(sockaddress)) < 0) {
        logger->info(fmt::format("Failed to bind: {}",strerror(errno)));
        return false;
    }
    return true;
}

bool Socket::listen(int32_t backlog) {
    if (::listen(m_sockfd, backlog) < 0) {
        logger->info(fmt::format("Failed to listen: {}",strerror(errno)));
        return false;
    }
    return true;
}

bool Socket::accept(Socket& newSocket) {
    int32_t newSockfd = ::accept(m_sockfd, NULL, NULL);
    if (newSockfd < 0) {
        std::cerr << "Accept failed: " << strerror(errno) << "\n";
        return false;
        }
    newSocket.m_sockfd = newSockfd;
    return true;
}

void Socket::close() {
    if (m_sockfd != -1) {
        ::close(m_sockfd);
        m_sockfd = -1;
    }
}

ssize_t Socket::send(const std::string& data) {

    size_t totalBytesSent = 0;
    size_t dataLength = data.size();
    logger->info(fmt::format("to send: {}",data));
    while (totalBytesSent < dataLength) {
        ssize_t bytesSent = ::send(m_sockfd, data.c_str() + totalBytesSent, dataLength - totalBytesSent, 0);
        if (bytesSent < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR) {
                continue;
            }
            logger->info(fmt::format("Failed to send: {}",strerror(errno)));
            return -1;
        }
        totalBytesSent += bytesSent;
    }
    logger->info(fmt::format("end send: {}",data));
    return totalBytesSent;
}

ssize_t Socket::send(const void* buffer, size_t length) {

    size_t totalBytesSent = 0;
    size_t dataLength = length;
    logger->info(fmt::format("to send: {}",buffer));
    while (totalBytesSent < dataLength) {
        ssize_t bytesSent = ::send(m_sockfd, buffer + totalBytesSent, dataLength - totalBytesSent, 0);
        if (bytesSent < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR) {
                continue;
            }
            logger->info(fmt::format("Failed to send: {}",strerror(errno)));
            return -1;
        }
        totalBytesSent += bytesSent;
    }
    return totalBytesSent;
}

ssize_t Socket::send(const Message& s) {
    std::string str = s.toString();
    return send(str);
}

ssize_t Socket::send(const Serializer& s) {
    std::string str = s.toString();
    return send(str);
}

ssize_t Socket::receive(std::string& data, size_t size) {
    char buffer[size];
    ssize_t totalBytesReceived = 0;
    while (totalBytesReceived < size) {
        ssize_t bytesReceived = ::recv(m_sockfd, buffer + totalBytesReceived, size - totalBytesReceived, 0);
        //logger->info(fmt::format("try to receive: "));  
        if (bytesReceived < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR) {
                //此刻应该把协程挂起
                if(Scheduler::GetInstance()!=nullptr)
                    Coroutine::GetCurrentCoroutine()->yield();
                continue;
            }
            return -1;
        }
        if (bytesReceived == 0) {
            m_is_connected = false;
            break;  // Connection closed
        }
        totalBytesReceived += bytesReceived;
    }
    data.assign(buffer, totalBytesReceived);
    return totalBytesReceived;
}

ssize_t Socket::receive(void* buffer, size_t length) {
    ssize_t totalBytesReceived = 0;
    while (totalBytesReceived < length) {
        ssize_t bytesReceived = ::recv(m_sockfd, buffer + totalBytesReceived, length - totalBytesReceived, 0);
        //logger->info(fmt::format("try to receive: ")); 
        if (bytesReceived < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR) {
                //此刻应该把协程挂起
                if(Scheduler::GetInstance()!=nullptr)
                    Coroutine::GetCurrentCoroutine()->yield();
                continue;
            }
            return -1;
        }
        if (bytesReceived == 0) {
            m_is_connected = false;
            break;  // Connection closed
        }
        totalBytesReceived += bytesReceived;
    }
    return totalBytesReceived;
}

ssize_t Socket::receive(Message& s) {
    Serializer* serializerPtr = s.getSerializer().get();

    receive(*serializerPtr,1);

    receive(*serializerPtr,1);

    receive(*serializerPtr,1);

    receive(*serializerPtr,1);

    receive(*serializerPtr,8);

    receive(*serializerPtr,8);

    serializerPtr->reset();
    if(serializerPtr->size()==0) 
        return serializerPtr->size();
    uint32_t tmp;
    uint64_t size;
    (*serializerPtr)>>tmp>>size>>size;//读出contenlenth
    //std::cout<<"receive"<<serializerPtr->size()<<std::endl;
    receive(*serializerPtr,size);
    
    serializerPtr->reset();

    return serializerPtr->size();
}

ssize_t Socket::receive(Serializer& s, size_t size) {
    std::string str;
    receive(str, size);
    s.write(str.data(),str.size());
    return s.size();
}
}