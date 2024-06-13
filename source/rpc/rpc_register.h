#ifndef _RPC_RESISTER_H_
#define _RPC_RESISTER_H_

#include "utils.h"
#include "socket/tcpserver.h"
#include "coroutine/scheduler.h"
namespace C_RPC{

class RpcRegister : public TCPServer {
public:
    RpcRegister(const std::string& ip, uint16_t port):TCPServer(ip,port) {}

    void run();
     // 添加远程函数
    bool addFunc(const std::string& funcName, const std::vector<std::string>& params);
    // 删除远程函数
    bool eraseFunc(const std::string& funcName);
    // 添加远程函数地址
    bool addAddress(const std::string& funcName, ip_port address);
    // 删除远程函数地址
    bool eraseAddress(const std::string& funcName, ip_port address);

private:
    //这些表应该加协程锁
    // 假设函数名全局唯一
    std::map<std::string, std::vector<std::string>> m_funcs;
    // 一个函数可能对应多个服务端地址
    std::multimap<std::string, ip_port> m_addresses;
    //
    std::multimap<std::string, std::shared_ptr<Socket>> m_subscribes;
    //心跳定时器
    std::map<int32_t,std::shared_ptr<C_RPC::TimeEvent>> m_heartbeat_timers;

    void handleMsg(int32_t sockfd);
    void handleCLientInit(Message &receive_message,std::shared_ptr<Socket> socket);
    void handleServiceDiscover(Message &receive_message,std::shared_ptr<Socket> socket);
    void handleServerInit(Message &receive_message,std::shared_ptr<Socket> socket);
    void handleServiceRegister(Message &receive_message,std::shared_ptr<Socket> socket);
    void handleServiceDelete(Message &receive_message,std::shared_ptr<Socket> socket);

    void handleSubscribe(Message &receive_message,std::shared_ptr<Socket> socket);
    void publish(std::string f_name,std::string f_ip,uint16_t f_port ,bool is_add = true);

    void handleHeartBeat(Message &receive_message,std::shared_ptr<Socket> socket);
};

}
#endif