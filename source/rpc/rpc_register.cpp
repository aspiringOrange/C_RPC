#include "rpc_register.h" 
#include "config.h"
//应当实现一个协程用于监听listenfd，将clientfd注册到sub协程，sub协程专门监听clientfd
namespace C_RPC{

static std::shared_ptr<spdlog::logger> logger = LOG_NAME("rpc_register");


// 添加远程函数
bool RpcRegister::addFunc(const std::string& funcName, const std::vector<std::string>& params){
    m_funcs[funcName] = params;
    return true;
}
// 删除远程函数
bool RpcRegister::eraseFunc(const std::string& funcName){
    auto it = m_funcs.find(funcName);
    if (it != m_funcs.end()) {
        m_funcs.erase(it);
        return true;
    }
    return false;
}
// 添加远程函数地址
bool RpcRegister::addAddress(const std::string& funcName, ip_port address){
    m_addresses.emplace(funcName, address);
    return true;
}

// 删除远程函数地址
bool RpcRegister::eraseAddress(const std::string& funcName, ip_port address){
    auto range = m_addresses.equal_range(funcName);
    for (auto it = range.first; it != range.second; ++it) {
        if ((it->second) == address) {
           m_addresses.erase(it);
           return true;
       }
    }
    return false;
}

void RpcRegister::run(){
    const int MAX_EVENTS = 1024; // 最大事件数
    epoll_event events[MAX_EVENTS]; // 用于存储事件的数组

    while (true) { // 主循环
        int eventCount = epoll_wait(m_epollFD, events, MAX_EVENTS, -1); // 等待事件发生
        if (eventCount == -1) { // 检查 epoll_wait 是否成功
            std::cerr << "epoll_wait error: " << strerror(errno) << "\n"; // 打印错误信息
            break; // 退出循环
        }

        for (int i = 0; i < eventCount; ++i) { // 遍历发生的事件
            if (events[i].data.fd == m_listenSocket.getSocketFD()) { // 如果是监听套接字的事件
                std::shared_ptr<Socket> newSocket(new Socket);
                //Socket newSocket; // 创建新套接字
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
                handleMsg(events[i].data.fd);               
                
                
                
            }
        }
    }
}
void RpcRegister::handleMsg(int32_t sockfd){
    logger->info(fmt::format("new data from fd{}",int(sockfd)));  
    auto it = m_clientSockets.find(sockfd);
    if (it != m_clientSockets.end()) { // 如果找到客户端套接字
        Message receive_message;
        ssize_t bytesReceived = it->second->receive(receive_message); // 接收数据
        if (bytesReceived <= 0) { // 如果接收失败或连接关闭
            if (bytesReceived < 0) { // 如果接收失败
                std::cerr << "Receive error: " << strerror(errno) << "\n"; // 打印错误信息
            } else { // 如果连接关闭
                std::cout << "Connection closed: " << it->second->getSocketFD() << "\n"; // 打印连接关闭信息
                epoll_ctl(m_epollFD, EPOLL_CTL_DEL, it->second->getSocketFD(), nullptr); // 从 epoll 实例中移除套接字
                it->second->close(); // 关闭套接字
                m_clientSockets.erase(it); // 从客户端列表中删除套接字
            }
            //epoll_ctl(m_epollFD, EPOLL_CTL_DEL, it->getSocketFD(), nullptr); // 从 epoll 实例中移除套接字
            //it->close(); // 关闭套接字
            //m_clientSockets.erase(it); // 从客户端列表中删除套接字
        } else { // 如果接收成功
            receive_message.decode();
            std::cout << "new data accessed:" << receive_message.toString() << "\n"; 
            logger->info(fmt::format("new data accessed:{}",receive_message.toString()));  
            switch(receive_message.type){
                case Message::Type::RPC_CLIENT_INIT:
                    handleCLientInit(receive_message,it->second);
                    break;
                case Message::Type::RPC_SERVER_INIT:
                    handleServerInit(receive_message,it->second);
                    break;
                case Message::Type::RPC_SERVICE_REGISTER:
                    handleServiceRegister(receive_message,it->second);
                    break;
                case Message::Type::HEART_BEAT:
                    handleHeartBeat(receive_message,it->second);
                    break;
                default:break;
            }
            
        }
    }
}

void RpcRegister::handleCLientInit(Message &receive_message,std::shared_ptr<Socket> socket){
    logger->info(fmt::format("msg type:RPC_CLIENT_INIT"));
    Message message(C_RPC::Message::RPC_SERVICE_DISCOVER_RESPONSE);
    message.seq = receive_message.seq;
    Serializer s;
    for(auto &i:m_addresses){
        //先写文函数名称
        std::string f_name = i.first;
        s<<f_name;
        //再写函数地址
        std::string f_ip = i.second.m_ip;
        s<<f_ip<<i.second.m_port;
    }
    message.encode(s);
    socket->send(message);
}

void RpcRegister::handleServiceDiscover(Message &receive_message,std::shared_ptr<Socket> socket){
    logger->info(fmt::format("msg type:RPC_SERVICE_DISCOVER"));
    std::shared_ptr<Serializer> s =receive_message.getSerializer();
    std::string f_name;
    while(!s->isReadEnd()){
        (*s)>>f_name;
        std::cout<<"fname:"<<f_name<<std::endl;
        logger->info(fmt::format("RPC_SERVICE_DISCOVER:{}",f_name));
    }
    Message message(C_RPC::Message::RPC_SERVICE_DISCOVER_RESPONSE);
    message.seq = receive_message.seq;
    Serializer res;
    auto range = m_addresses.equal_range(f_name);
    for (auto it = range.first; it != range.second; ++it) {
        std::string f_name = it->first;
        res<<f_name;
        //再写函数地址
        std::string f_ip = it->second.m_ip;
        res<<f_ip<<it->second.m_port;
    }
    
    message.encode(res);
    socket->send(message);
}

void RpcRegister::handleServerInit(Message &receive_message,std::shared_ptr<Socket> socket){
    //解析返回值
    logger->info(fmt::format("msg type:RPC_SERVER_INIT"));  
    std::shared_ptr<Serializer> s =receive_message.getSerializer();
    
    while(!s->isReadEnd()){
        std::string f_name,f_ip;
        uint16_t f_port;
        (*s)>>f_name>>f_ip>>f_port;
        std::cout<<"fname:"<<f_name<<" f_ip:"<<f_ip<<" f_port:"<<f_port<<std::endl;
        logger->info(fmt::format("RPC_SERVER_INIT:{},{},{}",f_name,f_ip,f_port));
        ip_port ip_port_(f_ip,f_port);
        m_addresses.emplace(f_name,ip_port_);
    }
    auto timer = Scheduler::GetTimer()->addTimeEvent(1500,[=](){
        m_heartbeat_timers.erase(socket->getSocketFD());
        std::cout << "Connection closed by heartbeat: " << socket->getSocketFD() << "\n"; // 打印连接关闭信息
        socket->close();
    },false);
    m_heartbeat_timers.emplace(socket->getSocketFD(),timer);
    Message message(C_RPC::Message::RPC_SERVICE_REGISTER_RESPONSE);
    message.seq = receive_message.seq;
    Serializer res;
    message.encode(res);
    socket->send(message);
}

void RpcRegister::handleServiceRegister(Message &receive_message,std::shared_ptr<Socket> socket){
    //解析返回值
    logger->info(fmt::format("msg type:RPC_SERVICE_REGISTER"));  
    std::shared_ptr<Serializer> s =receive_message.getSerializer();
    
    while(!s->isReadEnd()){
        std::string f_name,f_ip;
        uint16_t f_port;
        (*s)>>f_name>>f_ip>>f_port;
        std::cout<<"fname:"<<f_name<<" f_ip:"<<f_ip<<" f_port:"<<f_port<<std::endl;\
        logger->info(fmt::format("RPC_SERVICE_REGISTER:{},{},{}",f_name,f_ip,f_port));
        ip_port ip_port_(f_ip,f_port);
        m_addresses.emplace(f_name,ip_port_);
    }
    Message message(C_RPC::Message::RPC_SERVICE_REGISTER_RESPONSE);
    message.seq = receive_message.seq;
    Serializer res;
    message.encode(res);
    socket->send(message);
}

void RpcRegister::handleServiceDelete(Message &receive_message,std::shared_ptr<Socket> socket){
    //解析返回值
    logger->info(fmt::format("msg type:RPC_SERVICE_DELETE"));  
    std::shared_ptr<Serializer> s =receive_message.getSerializer();
    
    while(!s->isReadEnd()){
        std::string f_name,f_ip;
        uint16_t f_port;
        (*s)>>f_name>>f_ip>>f_port;
        std::cout<<"fname:"<<f_name<<" f_ip:"<<f_ip<<" f_port:"<<f_port<<std::endl;\
        logger->info(fmt::format("RPC_SERVICE_DELETE:{},{},{}",f_name,f_ip,f_port));
        ip_port ip_port_(f_ip,f_port);
        auto range = m_addresses.equal_range(f_name);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second == ip_port_) {
                m_addresses.erase(it);
            }
        }
    }
    Message message(C_RPC::Message::RPC_SERVICE_REGISTER_RESPONSE);
    message.seq = receive_message.seq;
    Serializer res;
    message.encode(res);
    socket->send(message);
}

void RpcRegister::handleSubscribe(Message &receive_message,std::shared_ptr<Socket> socket){
    //解析返回值
    logger->info(fmt::format("msg type:RPC_SUBSCRIBE"));  
    std::shared_ptr<Serializer> s =receive_message.getSerializer();
    
    while(!s->isReadEnd()){
        std::string f_name;
        (*s)>>f_name;
        std::cout<<"subscribe fname:"<<f_name<<std::endl;
        logger->info(fmt::format("RPC_SUBSCRIBE:{}",f_name));
        
        m_subscribes.emplace(f_name,socket);
    }
    Message message(C_RPC::Message::RPC_SUBSCRIBE_RESPONSE);
    message.seq = receive_message.seq;
    Serializer res;
    message.encode(res);
    socket->send(message);
}

void RpcRegister::publish(std::string f_name,std::string f_ip,uint16_t f_port ,bool is_add ){
    Message message(C_RPC::Message::RPC_SERVICE_DISCOVER_RESPONSE);
    Serializer res;
    res<<is_add<<f_name<<f_ip<<f_port; 
    message.encode(res);
    auto range = m_subscribes.equal_range(f_name);
    for (auto it = range.first; it != range.second; ++it) {
        //to do: socket.send函数也要加协程锁，不然会冲突
        it->second->send(message);
    }
}

void RpcRegister::handleHeartBeat(Message &receive_message,std::shared_ptr<Socket> socket){
    //解析返回值
    logger->info(fmt::format("msg type:HEART_BEAT"));  
    std::shared_ptr<Serializer> s =receive_message.getSerializer();
    
    while(!s->isReadEnd()){
    }
    //reset定时器
    auto timer = m_heartbeat_timers[socket->getSocketFD()];
    if(timer!=nullptr){
        logger->info(fmt::format("HEART_BEAT"));
        timer->resetTime();
        return;
    }
    m_heartbeat_timers.erase(socket->getSocketFD());
}
}


