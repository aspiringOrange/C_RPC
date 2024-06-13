#ifndef _RPC_SESSION_H
#define _RPC_SESSION_H
#include "coroutine/coroutinesync.h"
#include "coroutine/coroutinechannal.h"
//#include "rpc_connection_pool.h"
namespace C_RPC{
static std::shared_ptr<spdlog::logger> rpc_session_logger = LOG_NAME("rpc_session");
class RpcCLient;
class RpcSession{
public:
    RpcSession():m_send_channal(1024){
        m_has_connect=false;
    }
    ~RpcSession(){
        m_is_close = true;
    }

    bool connect(const Address& address){
        m_socket.create();
        m_socket.setNonBlocking(true);
        if(!m_socket.connect(address))
            return false;
        m_has_connect = true;
        Scheduler::GetInstance()->addTask(std::bind(&RpcSession::handle_send, this));
        Scheduler::GetInstance()->addTask(std::bind(&RpcSession::handle_receive, this));

        return true;
    }

    bool testAndreConnect(const Address& address){
        if(m_socket.isConnected())
            return true;
        m_socket.close();
        m_socket.create();
        m_socket.setNonBlocking(true);
        if(!m_socket.connect(address))
            return false;
        if(m_has_connect) 
            return true;
        m_has_connect = true;
        Scheduler::GetInstance()->addTask(std::bind(&RpcSession::handle_send, this));
        Scheduler::GetInstance()->addTask(std::bind(&RpcSession::handle_receive, this));

        return true;
    }

    std::shared_ptr<CoroutineChannal<Message>> send(const Message& s){
        std::shared_ptr<CoroutineChannal<Message>> recv_channal(new CoroutineChannal<Message>(1));
        m_mutex.lock();
        m_response_channals[s.seq]= recv_channal;
        m_mutex.unlock();
        m_send_channal<<s;
        return recv_channal;
    }

    void handle_send(){
        while(!m_is_close){
            Message send_message(Message::RPC_DEFAULT);
            m_send_channal>>send_message;//client发送一个关闭协程的msg
            rpc_session_logger->info(fmt::format("RPC_session sed::{}",send_message.toString()));
            if(send_message.type==Message::RPC_DEFAULT){
                rpc_session_logger->info(fmt::format("RPC_session RPC_DEFAULT"));
                if(m_is_close) break;
                else continue;
            }
            m_socket.send(send_message);
        }
    }
    void handle_receive(){
        while (!m_is_close) {
            Message receive_message;
            m_socket.receive(receive_message);
            if(!receive_message.getSerializer()->size()){//connection close
                m_socket.close();
                break;
            }
            rpc_session_logger->info(fmt::format("RPC_session receive::{}",receive_message.toString()));
            m_mutex.lock();
            receive_message.decode();
            if(receive_message.type==Message::Type::RPC_PUBLISH){
                std::shared_ptr<Serializer> s =receive_message.getSerializer();
                while(!s->isReadEnd()){
                    std::string op,f_name,f_ip;
                    uint16_t f_port;
                    (*s)>>op>>f_name>>f_ip>>f_port;
                    std::cout<<"op:"<<op<<" fname:"<<f_name<<" f_ip:"<<f_ip<<" f_port:"<<f_port<<std::endl;
                    std::shared_ptr<ip_port> address(new ip_port(f_ip,f_port));
                    // connectionPool_mutex.lock();
                    // if(op == "add" )
                    //     connectionPoolPtr->conaddAddress(f_name,address);
                    // else(op == "delete")
                    //     connectionPoolPtr->eraseAddress(f_name,address);
                    // connectionPool_mutex.unlock();
                }
                m_mutex.unlock();
                earseResponseChannal(receive_message.seq);
                m_mutex.lock();
            }
            else if(m_response_channals.find(receive_message.seq)!=m_response_channals.end())  
                    (*m_response_channals[receive_message.seq])<<receive_message;
            
            m_mutex.unlock();
        }
    }

    void earseResponseChannal(uint64_t seq){
        m_mutex.lock();
        m_response_channals.erase(seq);
        m_mutex.unlock();
    }

    // void setConnectionPool(ConnectionPool *ptr , CoroutineMutex & mutex){
    //     connectionPoolPtr=ptr;
    //     connectionPool_mutex=mutex;
    // }
private:
    Socket m_socket;
    bool m_is_close{false};
    CoroutineMutex m_mutex;
    //发送队列
    CoroutineChannal<Message> m_send_channal;
    //接受队列
    std::map<uint64_t, std::shared_ptr<CoroutineChannal<Message>>> m_response_channals;

    bool m_has_connect;
    // ConnectionPool *connectionPoolPtr{nullptr};
    // CoroutineMutex &connectionPool_mutex;
};


}


#endif