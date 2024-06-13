

#include "rpc_connection_pool.h"


namespace C_RPC {

ConnectionPool::ConnectionPool(){
    
}

bool ConnectionPool::addFunc(const std::string& funcName, const std::vector<std::string>& params) {
    m_funcs[funcName] = params;
    return true;
}

// 删除远程函数
bool ConnectionPool::eraseFunc(const std::string& funcName) {
    auto it = m_funcs.find(funcName);
    if (it != m_funcs.end()) {
        m_funcs.erase(it);
        return true;
    }
    return false;
}

// 修改远程函数
bool ConnectionPool::changeFunc(const std::string& funcName, const std::vector<std::string>& params) {
    auto it = m_funcs.find(funcName);
    if (it != m_funcs.end()) {
        it->second = params;
        return true;
    }
    return false;
}

// 检查本地调用参数是否匹配远程函数
bool ConnectionPool::ConnectionPool::checkFuncParams(const std::string& funcName, const std::vector<std::string>& params) {
    auto it = m_funcs.find(funcName);
    if (it != m_funcs.end() && it->second == params) {
        return true;
    }
    return false;
}

// 添加远程函数地址
bool ConnectionPool::addAddress(const std::string& funcName, std::shared_ptr<ip_port> address) {
    m_addresses.emplace(funcName, address);
    return true;
}

// 删除远程函数地址
bool ConnectionPool::eraseAddress(const std::string& funcName, std::shared_ptr<ip_port> address) {
    auto range = m_addresses.equal_range(funcName);
    for (auto it = range.first; it != range.second; ++it) {
        if (*it->second == *address) {
           m_addresses.erase(it);
           return true;
       }
    }
    return false;
}

// 修改远程函数地址
bool ConnectionPool::changeAddress(const std::string& funcName, std::shared_ptr<ip_port> oldAddress, std::shared_ptr<ip_port> newAddress) {
    if (eraseAddress(funcName, oldAddress)) {
        return addAddress(funcName, newAddress);
    }
    return false;
}

// 添加服务器 RpcSession
bool ConnectionPool::addServer(const std::string& serverName, std::shared_ptr<RpcSession> session) {
    m_servers.emplace(serverName, session);
    return true;
}

// 删除服务器 RpcSession
bool ConnectionPool::eraseServer(const std::string& serverName) {
    auto it = m_servers.find(serverName);
    if (it != m_servers.end()) {
        m_servers.erase(it);
        return true;
    }
    return false;
}

// 添加注册中心 RpcSession
bool ConnectionPool:: addRegister(const std::string& registerName, std::shared_ptr<RpcSession> session) {
    m_registers.emplace(registerName, session);
    return true;
}

// 删除注册中心 RpcSession
bool ConnectionPool::eraseRegister(const std::string& registerName) {
    auto it = m_registers.find(registerName);
    if (it != m_registers.end()) {
        m_registers.erase(it);
        return true;
    }
    return false;
}

// 根据函数名返回服务器地址
std::vector<std::shared_ptr<RpcSession>> ConnectionPool::getAllRegister(){
    std::vector<std::shared_ptr<RpcSession>> registers;
    for (auto it:m_registers) {
        registers.push_back(it.second);
    }
    if(m_registers.find("raft register")!=m_registers.end())
        registers[0]=m_registers["raft register"];
    return registers;
}

// 根据函数名返回服务器地址
std::vector<std::shared_ptr<ip_port>> ConnectionPool::getAllAddress(const std::string& funcName) {
    std::vector<std::shared_ptr<ip_port>> addresses;
    auto range = m_addresses.equal_range(funcName);
    for (auto it = range.first; it != range.second; ++it) {
        addresses.push_back(it->second);
    }
    return addresses;
}

// 根据函数名返回已经建立的服务器连接
std::vector<std::shared_ptr<RpcSession>> ConnectionPool::getAllRpcSession(const std::string& funcName) {
    std::vector<std::shared_ptr<RpcSession>> sessions;
    auto range = m_servers.equal_range(funcName);
    for (auto it = range.first; it != range.second; ++it) {
        sessions.push_back(it->second);
    }

    if(!sessions.empty())
        return sessions;

    
    //根据address查找所有的已建立连接
    std::vector<std::shared_ptr<ip_port>> addresses = getAllAddress(funcName);
    if(addresses.empty()) 
        return  sessions;

    for( auto add : addresses){
        auto range = m_addr_to_session.equal_range(add);
        for (auto it = range.first; it != range.second; ++it) {
            sessions.push_back(it->second);
        }
    }

    if(!sessions.empty())
        return sessions;

    std::shared_ptr<RpcSession> newsession (new RpcSession);
    Address address(addresses[0]->m_ip,addresses[0]->m_port);
    if (newsession->connect(address)) {
        addServer(funcName, newsession);
        sessions.push_back(newsession);
    }
    
    return sessions;
}

// 根据函数名返回已经建立的服务器连接，但可以采用不同的负载均衡策略
std::shared_ptr<RpcSession> ConnectionPool::getRpcSession(const std::string& funcName) {
    auto sessions = getAllRpcSession(funcName);
    if (sessions.empty()) {
        return nullptr;
    }

    // 使用随机选择一个 RpcSession 作为负载均衡策略
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, sessions.size() - 1);
    return sessions[dist(gen)];
}


} // namespace C_RPC


