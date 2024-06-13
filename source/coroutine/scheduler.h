#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_
#include <atomic>
#include <memory>
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <vector>
#include "coroutine.h"
#include "config.h"
#include "timer.h"
namespace C_RPC{

class Scheduler{
public:
    /**
     * @brief 构造函数
     * @param[in] threads 线程数量
     */
    Scheduler(size_t threads = Scheduler_threads);

    ~Scheduler();

    /**
     * @brief 启动协程调度器
     */
    void start();

    /**
     * @brief 停止协程调度器
     */
    void stop();


    bool addTask(Coroutine&& coroutine);

    bool addTask(std::shared_ptr<Coroutine>&& coroutine);

    bool addTask(std::function<void()> &&cb);

    static Scheduler* GetInstance();

    static Timer* GetTimer();

private:

    /**
     * @brief 协程调度函数
     */
    void run();

    /**
     * @brief 协程无任务可调度时从timer处取任务
     */
    void getTaskFromTimer();


    
    
private:
    //互斥量
    std::mutex m_mutex;
    //调度器线程池
    std::vector<std::thread> workerThreads;
    //调度器任务
    std::queue<std::shared_ptr<Coroutine>> m_tasks;
    //线程数量
    size_t m_threads{0};
    //活跃线程数
    std::atomic<size_t> m_runningThreads{0};
    //调度器是否停止
    bool m_stop = true;
    //Coroutine id
    std::atomic<uint64_t> m_coroutine_id{0};
    std::shared_ptr<Timer> m_timer;
};
}
#endif 
