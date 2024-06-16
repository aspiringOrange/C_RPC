#include "rpc_client.h"

namespace C_RPC{
static std::shared_ptr<spdlog::logger> logger = LOG_NAME("rpc_client");
void RpcCLient::start(){
    //i'm client
    Message message(C_RPC::Message::RPC_CLIENT_INIT);
    Serializer s;
    s<<"i'm client.";
    message.encode(s);

    // 向 send 协程的 Channel 发送消息
    // 添加注册中心 RpcSession
    std::shared_ptr<RpcSession> register_session(new RpcSession);
    register_session->connect(Address(register_ip,register_port));
    m_mutex.lock();
    m_connection_pool.addRegister("defalut register", register_session);
    m_mutex.unlock();
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
        logger->info(fmt::format("responce accessed:{}",receive_message.toString()));  
        //解析函数名称加地址
        std::shared_ptr<Serializer> s =receive_message.getSerializer();
        while(!s->isReadEnd()){
            std::string f_name,f_ip;
            uint16_t f_port;
            (*s)>>f_name>>f_ip>>f_port;
            std::cout<<"fname:"<<f_name<<" f_ip:"<<f_ip<<" f_port:"<<f_port<<std::endl;
            std::shared_ptr<ip_port> address(new ip_port(f_ip,f_port));
            m_mutex.lock();
            m_connection_pool.addAddress(f_name,address);
            m_mutex.unlock();
        }
    }

}



}
