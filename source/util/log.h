#ifndef _LOG_H_
#define _LOG_H_

#include <mutex>
#include <string>
#include <map>
#include "spdlog/sinks/basic_file_sink.h"
namespace C_RPC{
/**
  * @brief spd日志
  */
class LogManager {
public:
    std::shared_ptr<spdlog::logger> getLogger(const std::string &name){
        std::lock_guard<std::mutex> lock(mutex);
        auto it = m_loggers.find(name);
        if(it != m_loggers.end())
            return it->second;
        std::string currentFile = __FILE__;
        std::string abstract_filename = currentFile.substr(0, currentFile.find_last_of("/\\") + 1);
        std::string logFilePath = abstract_filename + "../../logfile/"+name+"Log.txt";
        std::shared_ptr<spdlog::logger> logger = spdlog::basic_logger_mt(name, logFilePath);
        logger->set_level(spdlog::level::info);
        logger->flush_on(spdlog::level::info);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
        m_loggers[name] = logger;
        return logger;
    }
private:
    std::mutex mutex;
    std::map<std::string ,std::shared_ptr<spdlog::logger>> m_loggers;
};

}
#endif