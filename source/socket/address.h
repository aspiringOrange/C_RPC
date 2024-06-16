#ifndef _ADDRESS_H_
#define _ADDRESS_H_

#include <vector>
#include <cstring>
#include <arpa/inet.h> 
#include <ifaddrs.h> 
#include <netinet/in.h> 
namespace C_RPC{

/**
  * @brief 地址类,封装了sockaddr_in
  */
class Address{
public:
    Address() {
        memset(&m_address, 0, sizeof(m_address));
        m_address.sin_family = AF_INET;
    }

    /**
     * @brief 构造函数
     */
    Address(const std::string &ip, uint16_t port,int family = AF_INET) {
        memset(&m_address, 0, sizeof(m_address));
        m_address.sin_family = family;
        inet_pton(family, ip.c_str(), &m_address.sin_addr);
        m_address.sin_port = htons(port);
    }

    /**
     * @brief 获取本地网卡地址
     * @param[out] addresses 本地地址
     */
    static void getLocalAddress(std::vector<sockaddr_in>& addresses,int32_t family = AF_INET) {
        struct ifaddrs *ifap, *ifa;
        struct sockaddr_in *sa;

        getifaddrs(&ifap);
        for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr->sa_family == family) {
                sa = (struct sockaddr_in *) ifa->ifa_addr;
                if (sa->sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {  // Filter out 127.0.0.1
                    addresses.push_back(*sa);
                }
            }
        }
        freeifaddrs(ifap);
    }

    sockaddr_in getSockAddr() const {
        return m_address;
    }

    void setSockAddr(const sockaddr_in& address) {
        m_address = address;
    }

    std::string getIp() const {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(m_address.sin_family, &(m_address.sin_addr), ip, INET_ADDRSTRLEN);
        return std::string(ip);
    }

    uint16_t getPort() const {
        return ntohs(m_address.sin_port);
    }

    void setIp(const std::string& ip) {
        inet_pton(m_address.sin_family, ip.c_str(), &m_address.sin_addr);
    }

    void setPort(uint16_t port) {
        m_address.sin_port = htons(port);
    }

    bool operator==(const Address &b) const{
        return (inet_ntoa(this->m_address.sin_addr)==inet_ntoa(b.m_address.sin_addr))&&this->m_address.sin_port==b.m_address.sin_port;
    }

private:
    sockaddr_in m_address;
};

}//namespace C_RPC

#endif //_ADDRESS_H_