#ifndef _RPC_CLIENT_H_
#define _RPC_CLIENT_H_

#include "socket/socket.h"
#include "rpc_connection_pool.h"
#include "coroutine/coroutinesync.h"
#include "coroutine/coroutinechannal.h"
namespace C_RPC{
static std::shared_ptr<spdlog::logger> rpc_client_logger = LOG_NAME("rpc_client");
class RpcCLient{
public:

    RpcCLient():m_timeout(1000){}

    void start();

    void print() {std::cout <<std::endl;}

    template<typename T, typename... Types>
    void print(const T& firstArg, const Types&... args) {
        std::cout << firstArg << " " ;
        print(args...);
    }

    void subscribe(std::string fname) {
        //发送服务发现消息
        m_mutex.lock();
        std::shared_ptr<RpcSession> registerSession = m_connection_pool.getAllRegister()[0];
        m_mutex.unlock();
        Message message(C_RPC::Message::RPC_SUBSCRIBE);
        Serializer s;
        //先写函数名称
        s<<fname;
        message.encode(s);
        //registerSession->setConnectionPool(&m_connection_pool,m_mutex);
        registerSession->send(message);
        //接收协程自己处理
        
    }


    template<typename Ret ,typename... Args>
    Ret call(std::string f_name,Args ... params){
        print(f_name,params...);
        auto args = std::make_tuple(params ...);
        m_mutex.lock();
        std::shared_ptr<RpcSession> server = m_connection_pool.getRpcSession(f_name);
        m_mutex.unlock();
        if(server == nullptr){
            //发送服务发现消息
            m_mutex.lock();
            std::shared_ptr<RpcSession> registerSession = m_connection_pool.getAllRegister()[0];
            m_mutex.unlock();
            Message message(C_RPC::Message::RPC_SERVICE_DISCOVER);
            Serializer s;
            //先写函数名称
            s<<f_name;
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
        Message message(C_RPC::Message::RPC_REQUEST);
        Serializer s;
        //先写文函数名称
        s<<f_name;
        s<<args;
        message.encode(s);
        std::shared_ptr<CoroutineChannal<Message>> recv_channal = server->send(message);

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
        server->earseResponseChannal(message.seq);

        if (timeout) {
            rpc_client_logger->info(fmt::format("Failed to registerFunc2Reg:time_out"));  
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
    
    //template<typename Fuc, typename ...Params>
    


private:
    bool m_isClose{true};
    CoroutineMutex m_mutex;
    ConnectionPool m_connection_pool;

    //负载均衡策略
    //注册中心
    uint64_t m_timeout{1000};

};

}


#endif