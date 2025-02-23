#ifndef _RPC_SERVER_H
#define _RPC_SERVER_H

#include <unordered_map>
#include <functional>
#include <string>
#include "util/serializer.h"
#include "utils.h"
#include "socket/tcpserver.h"
#include "coroutine/scheduler.h"
#include "rpc_connection_pool.h"
#include "config.h"

namespace C_RPC{
/**
  * @brief RPC服务提供者
  */
class RpcServer :public TCPServer{
private:
    /**
    * @brief RPC服务提交到注册中心注册
    */
    void registerFunc2Reg(const std::string& name);
    void deleteFunc2Reg(const std::string& name);
public:
    /**
    * @brief RPC服务注册
    */
    template<typename Ret, typename... Args>
    void registerFunc(const std::string name, Ret(*func)(Args...)) {
        //注册到本地
        m_funcs[name] = [func, name,this]( Serializer& params, Serializer& returns) {
            callFunc(func, params, returns);
        };
        //提交到注册中心
        registerFunc2Reg(name);
    }

    void deleteFunc(const std::string name) {
        m_funcs.erase(name);
        deleteFunc2Reg(name);
    }

    /**
    * @brief RPC服务本地调用
    */
    void call(const std::string& name,  Serializer& params, Serializer& returns) {
        if (m_funcs.find(name) != m_funcs.end()) {
            m_funcs[name](params, returns);
        } else {
            throw std::runtime_error("Function not found");
        }
    }


    RpcServer(const std::string& ip, uint16_t port,uint64_t time_out =1000):TCPServer(ip,port),m_timeout(time_out) {}
    /**
    * @brief RPC服务提供者初始化
    */
    void start();
    /**
    * @brief RPC服务提供者启动，开始提供服务
    */
    void run();
private:
    //注册函数池
    std::unordered_map<std::string, std::function<void(Serializer&, Serializer&)>> m_funcs;
    ConnectionPool m_connection_pool;
    uint64_t m_timeout{1000};
    /**
    * @brief RPC服务本地调用
    */
    template<typename Ret, typename Func, typename Tuple, std::size_t... I>
    void callFuncImpl(Func func, const Tuple& t, Ret& result, std::index_sequence<I...>)  {
        if constexpr (std::is_same_v<Ret, void>) {
            func(std::get<I>(t)...);
        } else {
            result = func(std::get<I>(t)...);
        }
    }
    /**
    * @brief RPC服务本地调用
    */
    template<typename Ret, typename... Args>
    void callFunc(Ret(*func)(Args...),  Serializer& params, Serializer& returns) {
        std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...> args;
        unpack(params, args);
        Ret result;
        callFuncImpl(func, args, result, std::index_sequence_for<Args...>{});
        if constexpr (!std::is_same_v<Ret, void>) {
            returns << result;
        }
    }
    /**
    * @brief tuple解析
    */
    template<typename Tuple>
    void unpack( Serializer& params, Tuple& t) {
        params>>t;
    }

    //处理消息
    void handleMsg(int32_t sockfd);
    //处理RPC请求
    void handleCLientRequest(Message &receive_message,std::shared_ptr<Socket> socket);
    //重定向注册中心
    bool RedirectRegister(std::string ip, uint16_t port);
};
}
#endif // RPC_SERVER_H
