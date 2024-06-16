#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <sstream>
#include <string>
#include "log.h"
#include "singleton.h"
//断言
#define ENSURE_ENABLED 1
#ifdef ENSURE_ENABLED
#define ENSURE(e) if (!(e)) {                                        \
            std::stringstream ss;                                               \
            ss << __func__ << " in " << __FILE__ << ":" << __LINE__ << '\n';      \
            ss << " msg: " << std::string(#e);                                 \
            throw std::runtime_error(ss.str());                                 \
        }          
#else
#define ENSURE(e) ;
#endif
//日志
#define LOG_NAME(name) C_RPC::SingletonPtr<C_RPC::LogManager>::GetInstance()->getLogger(name)

//打印错误信息
#define ERROR(msg) {\
            std::stringstream ss;                                               \
            ss << __func__ << " in " << __FILE__ << ":" << __LINE__ << '\n';      \
            ss << " error: " << std::string(#msg);                                 \
            throw std::runtime_error(ss.str());                                 \
} 
//协程栈大小
constexpr size_t Coroutine_stacksize = 1024*1024;
//协程调度器线程数量
constexpr size_t Scheduler_threads = 4;
//默认注册中心
std::string const register_ip("127.0.0.1");
constexpr uint16_t register_port = 5829;
#endif