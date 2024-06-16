#include "rpc_server.h"

namespace C_RPC{
//debug 日志
static std::shared_ptr<spdlog::logger> logger = LOG_NAME("rpc_server");

void RpcServer::start(){
    TCPServer::start();
    //将本地提供的RPC服务注册到注册中心
    Message message(C_RPC::Message::RPC_SERVER_INIT);
    Serializer s;
    for(auto &i:m_funcs){
        std::string f_name = i.first;
        s<<f_name;
        s<<m_ip<<m_port;
    }
    message.encode(s);
    // 连接到注册中心，发送数据
    std::shared_ptr<RpcSession> register_session(new RpcSession);
    register_session->connect(Address(register_ip,register_port));
    m_connection_pool.addRegister("defalut register", register_session);
    std::shared_ptr<CoroutineChannal<Message>> recv_channal = register_session->send(message);
    //超时定时器
    bool timeout = false;
    std::shared_ptr<C_RPC::TimeEvent> timer = C_RPC::Scheduler::GetTimer()->addTimeEvent(m_timeout,[&timer,&recv_channal,&timeout](){
        timeout = true;
        recv_channal->close();
    });
    //接受响应
    Message receive_message;
    //Channel内部会挂起协程，如果有消息到达或者被关闭则会被唤醒
    (*recv_channal) >> receive_message;
    if(timer){
        timer->cancel();
    }
    // 删除序列号与 Channel 的映射
    register_session->earseResponseChannal(message.seq);
    if (timeout) {
        logger->info(fmt::format("Failed to connet register:time_out"));  
    }
    else{
        //解析返回值
        logger->info(fmt::format("init_responce accessed:{}",receive_message.toString()));  
    }
    //心跳
    Scheduler::GetTimer()->addTimeEvent(1000,[=](){
        Message heart_message(C_RPC::Message::HEART_BEAT);
        logger->info(fmt::format("HEART_BEAT to reg"));  
        Serializer temp;
        heart_message.encode(temp);
        register_session->send(heart_message);
        register_session->earseResponseChannal(heart_message.seq);
    },true);
}
    
void RpcServer::run(){
    const int MAX_EVENTS = 1024; // 最大事件数
    epoll_event events[MAX_EVENTS]; // 用于存储事件的数组

    while (true) { 
        int eventCount = epoll_wait(m_epollFD, events, MAX_EVENTS, -1);
        if (eventCount == -1) {
            std::cerr << "epoll_wait error: " << strerror(errno) << "\n";
            break; 
        }

        for (int i = 0; i < eventCount; ++i) { // 遍历发生的事件
            if (events[i].data.fd == m_listenSocket.getSocketFD()) { // 如果是监听套接字的事件 
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
                  handleMsg(events[i].data.fd);
            }
        }
    }
}

void RpcServer::handleMsg(int32_t sockfd){
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
        } else { // 如果接收成功
            receive_message.decode();
            std::cout << "new data accessed:" << receive_message.toString() << "\n"; 
            logger->info(fmt::format("new data accessed:{}",receive_message.toString()));  
            switch(receive_message.type){
                case Message::Type::RPC_REQUEST:
                    handleCLientRequest(receive_message,it->second);
                    break;
                //case Message::Type::RAFT_REDIRECT:

                default:break;
            }
            
        }
    }
}



void RpcServer::handleCLientRequest(Message &receive_message,std::shared_ptr<Socket> socket){
    logger->info(fmt::format("msg type:RPC_REQUEST"));  
    std::shared_ptr<Serializer> s =receive_message.getSerializer();
        
    std::string f_name;
    (*s)>>f_name;
    logger->info(fmt::format("call fname:{}",f_name));  
    Serializer returns;
    call(f_name,*s,returns);

    Message message(C_RPC::Message::RPC_RESPONSE);
    message.seq = receive_message.seq;
    message.encode(returns);
    socket->send(message);
}

void RpcServer::registerFunc2Reg(const std::string& name){ 
Redirect:
    //提交函数信息到注册中心
    std::shared_ptr<RpcSession> registerSession = m_connection_pool.getAllRegister()[0];
    Message message(C_RPC::Message::RPC_SERVICE_REGISTER);
    Serializer s;
    std::string f_name = name;
    s<<f_name<<m_ip<<m_port;
    message.encode(s);
    std::shared_ptr<CoroutineChannal<Message>> recv_channal = registerSession->send(message);
    //超时处理
    bool timeout = false;
    std::shared_ptr<C_RPC::TimeEvent> timer = C_RPC::Scheduler::GetTimer()->addTimeEvent(m_timeout,[&timer,&recv_channal,&timeout](){
        timeout = true;
        recv_channal->close();
    });
    Message receive_message;
    //Channel内部会挂起协程，如果有消息到达或者被关闭则会被唤醒
    (*recv_channal) >> receive_message;
    if(timer){
        timer->cancel();
    }
    // 删除序列号与 Channel 的映射
    registerSession->earseResponseChannal(message.seq);
    if (timeout) {
        logger->info(fmt::format("Failed to registerFunc2Reg:time_out"));  
    }
    else{
        //解析返回值
        logger->info(fmt::format("registerFunc_responce accessed:{}",receive_message.toString()));  
        //解析函数名称加地址
        std::shared_ptr<Serializer> s =receive_message.getSerializer(); 
        if(receive_message.type == Message::Type::RAFT_REDIRECT){
            std::string ip;
            uint16_t port;
            (*s)>>ip>>port;
            //重新连接
            if(!RedirectRegister(ip,port))
                goto Redirect;
        }else{
            uint8_t c;
            while(!s->isReadEnd()){
                (*s)>>c;
            }
        }
        logger->info(fmt::format("registerFunc success"));  
    }
}

void RpcServer::deleteFunc2Reg(const std::string& name){
    std::shared_ptr<RpcSession> registerSession = m_connection_pool.getAllRegister()[0];
    Message message(C_RPC::Message::RPC_SERVICE_DELETE);
    Serializer s;
    std::string f_name = name;
    s<<f_name;
    s<<m_ip<<m_port;
    message.encode(s);
    std::shared_ptr<CoroutineChannal<Message>> recv_channal = registerSession->send(message);
    bool timeout = false;
    std::shared_ptr<C_RPC::TimeEvent> timer = C_RPC::Scheduler::GetTimer()->addTimeEvent(m_timeout,[&timer,&recv_channal,&timeout](){
        timeout = true;
        recv_channal->close();
    });
    Message receive_message;
    // 等待 response，Channel内部会挂起协程，如果有消息到达或者被关闭则会被唤醒
    (*recv_channal) >> receive_message;
    if(timer){
        timer->cancel();
    }
    // 删除序列号与 Channel 的映射
    registerSession->earseResponseChannal(message.seq);

    if (timeout) {
        logger->info(fmt::format("Failed to deleteFunc2Reg:time_out"));  
    }
    else{
        //解析返回值
        logger->info(fmt::format("deleteFunc2Reg_responce accessed:{}",receive_message.toString()));  
        //解析函数名称加地址
        std::shared_ptr<Serializer> s =receive_message.getSerializer();
        while(!s->isReadEnd()){
        }
    }
}


bool RpcServer::RedirectRegister(std::string ip, uint16_t port){
    // 添加注册中心 RpcSession
    logger->info(fmt::format("RedirectRegister ip:{}, port:{}",ip,port));  
    Message message(C_RPC::Message::RPC_SERVICE_REGISTER);
    Serializer s;
    message.encode(s);
    std::shared_ptr<RpcSession> register_session(new RpcSession);
    register_session->connect(Address(ip,port));
    m_connection_pool.addRegister("raft register", register_session);
    std::shared_ptr<CoroutineChannal<Message>> recv_channal = register_session->send(message);
    bool timeout = false;
    std::shared_ptr<C_RPC::TimeEvent> timer = C_RPC::Scheduler::GetTimer()->addTimeEvent(m_timeout,[&timer,&recv_channal,&timeout](){
        timeout = true;
        recv_channal->close();
    });

    Message receive_message;
    //Channel内部会挂起协程，如果有消息到达或者被关闭则会被唤醒
    (*recv_channal) >> receive_message;
    if(timer){
        timer->cancel();
    }
    // 删除序列号与 Channel 的映射
    register_session->earseResponseChannal(message.seq);

    if (timeout) {
        logger->info(fmt::format("Failed to connet register:time_out"));  
    }
    else{
        //解析返回值
        logger->info(fmt::format("RedirectRegister accessed:{}",receive_message.toString()));  
        //解析函数名称加地址
        std::shared_ptr<Serializer> s =receive_message.getSerializer();
        return true;
    }
}
}