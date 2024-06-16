#ifndef _RAFT_CONFIG_H_
#define _RAFT_CONFIG_H_

#include <string>
#include <vector>
#include "util/utils.h"
namespace C_RPC{
    namespace Raft{
        //raft集群
        const std::vector<ip_port> Nodes{
            ip_port("127.0.0.1",5829),
            ip_port("127.0.0.1",6829),
            ip_port("127.0.0.1",7829)
        };
        //raft投票参数
        struct RaftVoteParams {
            uint64_t term;          // 候选人的任期
            uint64_t candidateId;   // 请求选票的候选人的 ID
            uint64_t lastLogIndex;  // 候选人的最后日志条目的索引值
            uint64_t lastLogTerm;   // 候选人的最后日志条目的任期号
        };
        //raft投票返回值
        struct RaftVoteRets {
            uint64_t term;       // 当前任期号大于候选人id时，候选人更新自己的任期号，并切换为追随者;等于候选人的任期，选票有效
            bool voteGranted;   // true表示候选人赢得了此张选票
        };
        //raft日志
        struct Entry{
            // 日志索引
            uint64_t index{0};
            // 日志任期
            uint64_t term{0};
            //日志内容
            std::string op;
            std::string f_name;
            std::string f_ip;
            uint16_t f_port;
        };
        //raft日志同步参数
        struct AppendEntriesParams {
            uint64_t term;               // 领导人的任期
            uint64_t leaderId;           // 领导id
            uint64_t prevLogIndex;       // 紧邻新日志条目之前的那个日志条目的索引
            uint64_t prevLogTerm;        // 紧邻新日志条目之前的那个日志条目的任期
            Entry entry;                 // 需要被保存的日志条目（被当做心跳使用时，则日志条目内容为空；为了提高效率可能一次性发送多个）但是先简化
            uint64_t leaderCommit;	     // 领导人的已知已提交的最高的日志条目的索引
        };
        //raft日志同步返回值
        struct AppendEntriesRets {
            bool success{false};          // 如果跟随者所含有的条目和 prevLogIndex 以及 prevLogTerm 匹配上了，则为 true
            uint64_t term{0};             // 当前任期，如果大于领导人的任期则切换为追随者；小于于领导人的任期，过期反馈
        };
    }//namespace Raft
}

#endif //_RAFT_CONFIG_H_