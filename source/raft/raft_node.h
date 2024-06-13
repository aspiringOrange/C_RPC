#ifndef _RAFT_NODE_H_
#define _RAFT_NODE_H_


#include "rpc/rpc_register.h"
#include "coroutine/coroutinesync.h"
#include "coroutine/scheduler.h"
#include "config.h"
#include "raft_logger.h"
#include "raft_config.h"
#include "rpc/rpc_session.h"
#include <random>
namespace C_RPC{

namespace Raft{

/**
 * @brief Raft 的状态
 */
enum RaftState {
    Follower,   // 追随者
    Candidate,  // 候选人
    Leader      // 领导者
};


class RaftNode : public TCPServer {
public:

    RaftNode(uint64_t id, std::vector<ip_port> nodes = Nodes);

    ~RaftNode(){};

    void start();

    void stop();

    bool isLeader();

    RaftState getState();

    uint64_t getLeaderId() const { return m_leaderId;}


    uint64_t getNodeId() const { return m_id;}
    
private:

    void broadcastLog();

    void startElection();
  
    void broadcastHeartbeat();

    RaftVoteRets raftVote(RaftVoteParams params);

    AppendEntriesRets appendEntries(AppendEntriesParams params);

    void replicateLog(uint64_t nodeId);

    void startReplicateLog();


    uint64_t genRandom(uint64_t left = 500 , uint64_t right = 1500){
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(left, right);
        uint64_t res = dist(gen);
        std::cout<<"random = "<<res<<std::endl;
        return res;
    }
    
private:
    //锁
    CoroutineMutex m_raft_mutex;
    //当前节点状态
    RaftState m_state {Follower};
    //当前节点id
    uint64_t m_id;
    //集群leaderid
    uint64_t m_leaderId = -1;
    //当前节点任期
    uint64_t m_currentTerm = 0;
    //当前节点投票
    uint64_t m_votedFor = -1;
    //远程节点
    std::map<uint64_t, std::shared_ptr<RpcSession>> m_remote_nodes;
    std::vector<ip_port> m_nodes;
    //下一个发送的日志的index
    std::map<uint64_t,uint64_t> m_nextIndexs;
    //已经提交的最大的日志index
    std::map<uint64_t,uint64_t> m_matchIndexs;

    std::shared_ptr<TimeEvent> m_electionTimer;
    std::shared_ptr<TimeEvent> m_heartbeatTimer;

    RaftLogger m_raft_logger;

    std::shared_ptr<spdlog::logger> logger;

//register
public:
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

    void handleServerRedirect(Message &receive_message,std::shared_ptr<Socket> socket);

//rpc server

    std::unordered_map<std::string, std::function<void(Serializer&, Serializer&)>> m_rpc_funcs;

    // template<typename Ret, typename... Args>
    // void registerFunc(const std::string name, Ret(*func)(Args...)) {
    //     m_rpc_funcs[name] = [func, name,this]( Serializer& params, Serializer& returns) {
    //         callFunc(func, params, returns);
    //     };
    // }

    template<typename Ret, typename... Args>
    void registerFunc(const std::string name, std::function<Ret(Args...)> func) {
        m_rpc_funcs[name] = [func, name,this]( Serializer& params, Serializer& returns) {
            callFunc(func, params, returns);
        };
    }

    void call(const std::string& name,  Serializer& params, Serializer& returns) {
        if (m_rpc_funcs.find(name) != m_rpc_funcs.end()) {
            m_rpc_funcs[name](params, returns);
        } else {
            throw std::runtime_error("Function not found");
        }
    }

    template<typename Ret, typename Func, typename Tuple, std::size_t... I>
    void callFuncImpl(Func func, const Tuple& t, Ret& result, std::index_sequence<I...>)  {
        if constexpr (std::is_same_v<Ret, void>) {
            func(std::get<I>(t)...);
        } else {
            result = func(std::get<I>(t)...);
        }
    }

    template<typename Ret, typename... Args>
    void callFunc(std::function<Ret(Args...)> func,  Serializer& params, Serializer& returns) {
        std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...> args;
        unpack(params, args);
        Ret result;
        callFuncImpl(func, args, result, std::index_sequence_for<Args...>{});
        if constexpr (!std::is_same_v<Ret, void>) {
            returns << result;
        }
    }

    template<typename Tuple>
    void unpack( Serializer& params, Tuple& t) {
        params>>t;
    }

    void handleCLientRequest(Message &receive_message,std::shared_ptr<Socket> socket);
};

}
}
#endif 
