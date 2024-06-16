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
/**
 * @brief 协程调度器，每个调度线程的主协程从任务队列取出任务执行，当前任务执行完后返回主协程
 */
class Scheduler{
public:
    /**
     * @brief 构造函数
     * @param threads 线程数量
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

    /**
     * @brief 添加一个协程至协程队列
     */
    bool addTask(Coroutine&& coroutine);

    bool addTask(std::shared_ptr<Coroutine>&& coroutine);

    /**
     * @brief 添加一个任务至协程队列
     */
    bool addTask(std::function<void()> &&cb);

    /**
     * @brief 获取当前调度器
     */
    static Scheduler* GetInstance();

    /**
     * @brief 获取定时器
     */
    static Timer* GetTimer();

private:

    /**
     * @brief 主协程执行的协程调度函数
     */
    void run();

    /**
     * @brief 主协程从timer处取已经到达定时时间的任务
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
    //定时器
    std::shared_ptr<Timer> m_timer;
};

}//namespace C_RPC
#endif //_SCHEDULER_H_
