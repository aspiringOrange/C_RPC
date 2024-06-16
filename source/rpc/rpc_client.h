#ifndef _RPC_CLIENT_H_
#define _RPC_CLIENT_H_

#include "socket/socket.h"
#include "rpc_connection_pool.h"
#include "coroutine/coroutinesync.h"
#include "coroutine/coroutinechannal.h"
namespace C_RPC{
static std::shared_ptr<spdlog::logger> rpc_client_logger = LOG_NAME("rpc_client");
/**
  * @brief 服务消费者
  */
class RpcCLient{
public:

    RpcCLient():m_timeout(1000){}

    void start();

    void print() {std::cout <<std::endl;}

    /**
    * @brief 打印函数调用参数
    */
    template<typename T, typename... Types>
    void print(const T& firstArg, const Types&... args) {
        std::cout << firstArg << " " ;
        print(args...);
    }
    /**
    * @brief 服务订阅
    */
    void subscribe(std::string fname) {
        //发送服务订阅消息
        m_mutex.lock();
        std::shared_ptr<RpcSession> registerSession = m_connection_pool.getAllRegister()[0];
        m_mutex.unlock();
        Message message(C_RPC::Message::RPC_SUBSCRIBE);
        Serializer s;
        s<<fname;
        message.encode(s);
        //registerSession->setConnectionPool(&m_connection_pool,m_mutex);
        registerSession->send(message);
        //接收协程自己处理
        
    }

    /**
    * @brief RPC调用
    */
    template<typename Ret ,typename... Args>
    Ret call(std::string f_name,Args ... params){
        print(f_name,params...);
        auto args = std::make_tuple(params ...);
        //从RPC连接池获取PRC连接
        m_mutex.lock();
        std::shared_ptr<RpcSession> server = m_connection_pool.getRpcSession(f_name);
        m_mutex.unlock();
        //如果获取不到连接，发送服务发现消息到注册中心，获得相关远程地址，然后进行连接
        if(server == nullptr){
            //获取注册中心连接
            m_mutex.lock();
            std::shared_ptr<RpcSession> registerSession = m_connection_pool.getAllRegister()[0];
            m_mutex.unlock();
            //发送服务发现消息
            Message message(C_RPC::Message::RPC_SERVICE_DISCOVER);
            Serializer s;
            s<<f_name;
            message.encode(s);
            std::shared_ptr<CoroutineChannal<Message>> recv_channal = registerSession->send(message);
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
                rpc_client_logger->info(fmt::format("RPC_SERVICE_DISCOVER Failed : time_out"));  
                return Ret{};
            }
            else{
                //解析返回值
                rpc_client_logger->info(fmt::format("RPC_SERVICE_DISCOVER_RESPONSE :{}",receive_message.toString()));  
                //解析函数名称加地址
                std::shared_ptr<Serializer> s =receive_message.getSerializer();
                m_mutex.lock();
                while(!s->isReadEnd()){
                    std::string f_name,f_ip;
                    uint16_t f_port;
                    (*s)>>f_name>>f_ip>>f_port;
                    std::cout<<"fname:"<<f_name<<" f_ip:"<<f_ip<<" f_port:"<<f_port<<std::endl;
                    std::shared_ptr<ip_port> address(new ip_port(f_ip,f_port));
                    m_connection_pool.addAddress(f_name,address);
                }
                server = m_connection_pool.getRpcSession(f_name);
                m_mutex.unlock();
            }
            

        }
        //发送RPC请求消息
        Message message(C_RPC::Message::RPC_REQUEST);
        Serializer s;
        s<<f_name;
        s<<args;
        message.encode(s);
        std::shared_ptr<CoroutineChannal<Message>> recv_channal = server->send(message);

        bool timeout = false;
        std::shared_ptr<C_RPC::TimeEvent> timer = C_RPC::Scheduler::GetTimer()->addTimeEvent(m_timeout,[&timer,&recv_channal,&timeout](){
            timeout = true;
            recv_channal->close();
        });
        //接受RPC响应消息
        Message receive_message;
        //如果有消息到达或者被关闭则会被唤醒
        (*recv_channal) >> receive_message;
        if(timer){
            timer->cancel();
        }
        // 删除序列号与 Channel 的映射
        server->earseResponseChannal(message.seq);
        if (timeout) {
            rpc_client_logger->info(fmt::format("Failed to callFunc:time_out"));  
        }
        else{
            //解析返回值
            rpc_client_logger->info(fmt::format("callFunc_responce accessed:{}",receive_message.toString()));  
            //解析函数名称加地址
            std::shared_ptr<Serializer> s =receive_message.getSerializer();
            Ret result;
            while(!s->isReadEnd()){
                (*s)>>result;
            }
            return result;
        }
        
    }
    

private:
    bool m_isClose{true};
    CoroutineMutex m_mutex;
    //连接池
    ConnectionPool m_connection_pool;
    uint64_t m_timeout{1000};

};

}

#endif //_RPC_CLIENT_H_