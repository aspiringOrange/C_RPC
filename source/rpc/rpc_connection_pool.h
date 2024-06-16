#ifndef _RPC_CONNECTION_POOL_
#define _RPC_CONNECTION_POOL_

#include "socket/socket.h"
#include "rpc_session.h"
#include <map>
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <random>
#include "util/utils.h"
namespace C_RPC {
/**
  * @brief RPC连接池
  */
class ConnectionPool {
public:
    ConnectionPool();
    // 添加远程函数
    bool addFunc(const std::string& funcName, const std::vector<std::string>& params);

    // 删除远程函数
    bool eraseFunc(const std::string& funcName);

    // 修改远程函数
    bool changeFunc(const std::string& funcName, const std::vector<std::string>& params);

    // 检查本地调用参数是否匹配远程函数
    bool checkFuncParams(const std::string& funcName, const std::vector<std::string>& params);

    // 添加远程函数地址
    bool addAddress(const std::string& funcName, std::shared_ptr<ip_port> address);

    // 删除远程函数地址
    bool eraseAddress(const std::string& funcName, std::shared_ptr<ip_port> address);

    // 修改远程函数地址
    bool changeAddress(const std::string& funcName, std::shared_ptr<ip_port> oldAddress, std::shared_ptr<ip_port> newAddress);

    // 添加服务器 RpcSession
    bool addServer(const std::string& serverName, std::shared_ptr<RpcSession> session);

    // 删除服务器 RpcSession
    bool eraseServer(const std::string& serverName);

    // 添加注册中心 RpcSession
    bool addRegister(const std::string& registerName, std::shared_ptr<RpcSession> session);
    // 删除注册中心 RpcSession
    bool eraseRegister(const std::string& registerName) ;

    // 根据函数名返回服务器地址
    std::vector<std::shared_ptr<RpcSession>> getAllRegister();

    // 根据函数名返回服务器地址
    std::vector<std::shared_ptr<ip_port>> getAllAddress(const std::string& funcName);

    // 根据函数名返回已经建立的服务器连接
    std::vector<std::shared_ptr<RpcSession>> getAllRpcSession(const std::string& funcName);

    // 根据函数名返回已经建立的服务器连接，但可以采用不同的负载均衡策略
    std::shared_ptr<RpcSession> getRpcSession(const std::string& funcName);
private:
    // 假设函数名全局唯一
    std::map<std::string, std::vector<std::string>> m_funcs;
    // 一个函数对应的 session 连接
    std::multimap<std::string, std::shared_ptr<RpcSession>> m_servers;
    // 一个函数可能对应多个服务端地址
    std::multimap<std::string, std::shared_ptr<ip_port>> m_addresses;
    // 一个服务器可能对应多个服务端连接
    std::multimap<std::shared_ptr<ip_port>,std::shared_ptr<RpcSession>> m_addr_to_session;
    // 注册中心集群
    std::map<std::string, std::shared_ptr<RpcSession>> m_registers;
};

} // namespace C_RPC

#endif // _RPC_CONNECTION_POOL_
