#ifndef _RAFT_LOGGER_H_
#define _RAFT_LOGGER_H_

#include <string>
#include <vector>
#include "coroutine/coroutinesync.h"
#include "raft_config.h"
#include "coroutine/scheduler.h"
namespace C_RPC{
    namespace Raft{
        class RaftLogger{
        public:
        
            bool is_newer(uint64_t term,uint64_t index){
                if(term > m_entries[m_entries.size()-1].term){
                    return true;
                }else if(term == m_entries[m_entries.size()-1].term){
                    return index >= m_entries[m_entries.size()-1].index;
                }
                return false;
            }

            RaftLogger(uint64_t id,std::map<std::string, std::vector<std::string>> &a,std::multimap<std::string, ip_port>  &b,std::multimap<std::string, std::shared_ptr<Socket>> &c):
            m_funcs(a),m_addresses(b),m_subscribes(c),m_node_id(id){
                Entry entry;
                m_entries.push_back(entry);
                m_lastApplied=0;
                m_commitIndex=0;

                std::string logger_name = "raft/theAppliedLogOfNode";
                logger_name += m_node_id+'0';
                logger = LOG_NAME(logger_name);
                //创建协程，异步应用日志
                Scheduler::GetInstance()->addTask([this](){
                    while(true){
                        m_mutex.lock();
                        if(m_lastApplied<m_commitIndex){
                            //应用日志
                            Entry entry = m_entries[m_lastApplied+1];
                            if(entry.op=="no-op"){

                            }else if(entry.op=="register"){
                                ip_port address(entry.f_ip,entry.f_port);
                                m_addresses.emplace(entry.f_name, address);
                            }
                            logger->info(fmt::format("entry{}: op{}, f_name{}, f_ip{}, f_port{}",m_lastApplied+1,entry.op,entry.f_ip,entry.f_port,entry.f_name)); 
                            m_lastApplied++;
                        }
                        m_mutex.unlock();
                        Coroutine::GetCurrentCoroutine()->yield();
                    }
                });

            }

            uint64_t getLastApplied(){
                m_mutex.lock();
                uint64_t index = m_lastApplied;
                m_mutex.unlock();
                return index;
            }

            uint64_t getCommitIndex(){
                m_mutex.lock();
                uint64_t index = m_commitIndex;
                m_mutex.unlock();
                return index;
            }

            void setCommitIndex(uint64_t index){
                m_mutex.lock();
                m_commitIndex = index;
                m_mutex.unlock();
            }

            uint64_t getLastTerm(){
                m_mutex.lock();
                uint64_t term = m_entries[m_entries.size()-1].term;
                m_mutex.unlock();
                return term;
            }

            uint64_t getLastIndex(){
                m_mutex.lock();
                uint64_t index = m_entries[m_entries.size()-1].index;
                m_mutex.unlock();
                return index;
            }

            Entry getEntry(uint64_t index){
                m_mutex.lock();
                Entry entry;
                if(index<m_entries.size())
                    entry = m_entries[index];
                else entry.op="null";
                m_mutex.unlock();
                return entry;
            }

            uint64_t size(){
                m_mutex.lock();
                uint64_t res = m_entries.size();
                m_mutex.unlock();
                return res;
            }

            bool appendEntry(Entry new_entry, uint64_t prevLogIndex, uint64_t prevLogTerm){
                m_mutex.lock();
                //匹配前置日志
                auto entry_iter = m_entries.end();
                for(entry_iter--;entry_iter>=m_entries.begin();entry_iter--){
                    if(entry_iter->index > prevLogIndex){
                        continue;
                    }else if(entry_iter->index == prevLogIndex){
                        break;
                    }
                }
                //找不到
                if(entry_iter->index != prevLogIndex || entry_iter->term != prevLogTerm)
                    return false;
                //找到了
                //先把后续日志删除
                m_entries.erase(entry_iter + 1, m_entries.end());
                //添加日志
                m_entries.push_back(new_entry);
                m_mutex.unlock();
                return true;
            }

            uint64_t appendEntry(Entry new_entry){
                m_mutex.lock();      
                new_entry.index = m_entries[m_entries.size()-1].index +1;
                m_entries.push_back(new_entry);
                uint64_t index = m_entries.size()-1;
                m_mutex.unlock();
                return index;
            }
        private:
            CoroutineMutex m_mutex;
            std::vector<Entry> m_entries;
            uint64_t m_lastApplied{0};
            uint64_t m_commitIndex{0};

            uint64_t m_node_id;
            // 假设函数名全局唯一
            std::map<std::string, std::vector<std::string>> &m_funcs;
            // 一个函数可能对应多个服务端地址
            std::multimap<std::string, ip_port> &m_addresses;
            //
            std::multimap<std::string, std::shared_ptr<Socket>> &m_subscribes;
            std::shared_ptr<spdlog::logger> logger;
        };
    }
}

#endif //_RAFT_LOGGER_H_