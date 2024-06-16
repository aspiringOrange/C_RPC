#ifndef _UTILS_H_
#define _UTILS_H_

#include <memory>
#include <string>
#include <sys/time.h>

namespace C_RPC{
/**
  * @brief 获得当前时间
  */
uint64_t GetCurrentMS();
/**
  * @brief 协程sleep，避免线程被阻塞
  */
void coroutine_sleep(uint64_t ms);


struct ip_port{
    std::string m_ip;
    uint16_t m_port;
    ip_port(std::string ip, uint16_t port):m_ip(ip),m_port(port){}
    bool operator==(const ip_port& b)const{
    return m_ip==b.m_ip&&m_port==b.m_port;
    }  
};
}
#endif