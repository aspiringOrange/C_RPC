
#include "scheduler.h"
#include "config.h"
#include<iostream>
namespace C_RPC{
//debug 日志
static std::shared_ptr<spdlog::logger> logger = LOG_NAME("scheduler");
//调度器指针
static Scheduler* SchedulerPtr = nullptr;
//定时器指针
static Timer* TimerPtr = nullptr;

Scheduler::Scheduler(size_t threads)
        : m_threads(threads){
    SchedulerPtr = this;
    m_timer = std::shared_ptr<Timer>(new Timer());
    TimerPtr = m_timer.get();
    logger->info(fmt::format("Scheduler"));   
}

Scheduler::~Scheduler() {
    //先暂停定时器
    stop();
    logger->info(fmt::format("~Scheduler "));   
    if(SchedulerPtr == this){
        SchedulerPtr = nullptr;
    }
    m_timer = nullptr;
    //回收工作线程
    for(auto& t: workerThreads){
        if(t.joinable())
            t.join();
    }
}
void Scheduler::start() {
    std::lock_guard<std::mutex> lock(m_mutex);
    //调度器没有停止就直接返回
    if (m_stop == false){
        return;
    }
    //初始化工作线程池
    workerThreads.resize(m_threads);
    m_stop = false;
    for(size_t t_i = 0; t_i < m_threads; ++t_i){
        workerThreads.emplace_back([this, t_i]() {
            std::string threadName("scheduleworker_" + std::to_string(t_i));
            pthread_setname_np(pthread_self(), threadName.c_str());
            this->run();
            m_runningThreads--;
        });
    }
}

void Scheduler::stop() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stop == true){
            return;
        }
        m_stop = true;
    }
    //等待所有工作线程结束
    while(m_runningThreads!=0);
    ENSURE(m_runningThreads==0);
    std::lock_guard<std::mutex> lock(m_mutex);
    logger->info(fmt::format("Scheduler stop but m_tasks = {}",m_tasks.size()));   
}

Scheduler* Scheduler::GetInstance() {
    return SchedulerPtr;
}

Timer* Scheduler::GetTimer() {
    return TimerPtr;
}

bool Scheduler::addTask(Coroutine&& coroutine){
    //如果调度器暂停执行了，那么返回false
    if(m_runningThreads!=m_threads || m_stop) 
        return false;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tasks.push(coroutine.shared_from_this()); 
    return true;
}

bool Scheduler::addTask(std::shared_ptr<Coroutine>&& coroutine){
    //如果调度器暂停执行了，那么返回false
    if(m_runningThreads!=m_threads || m_stop) 
        return false;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tasks.push(std::move(coroutine)); 
    return true;
}

bool Scheduler::addTask(std::function<void()> &&cb){
    //如果调度器暂停执行了，那么返回false
    if(m_runningThreads!=m_threads || m_stop) 
        return false;
    //创建一个协程
    std::shared_ptr<Coroutine> coroutine(new Coroutine(std::move(cb)) );  
    std::lock_guard<std::mutex> lock(m_mutex);   
    m_tasks.push(std::move(coroutine));  
    return true;
}

void Scheduler::run() {
    //线程启动主协程
    Coroutine::StartCoroutine();
    m_runningThreads++;
    //当前调度执行的协程
    std::shared_ptr<Coroutine> cur_coroutine = nullptr;
    bool m_tasks_empty = false;
    while (!m_stop){
        //线程取出任务
        m_tasks_empty = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if(!m_tasks.empty()){
                cur_coroutine = m_tasks.front();
                m_tasks.pop();
            }else {
                m_tasks_empty=true;
            }
        }
        
        if(!m_tasks_empty){  
            //执行协程
            cur_coroutine->resume();
            //协程未执行完函数，状态为ready，可以被立即调度
            if(cur_coroutine->getState()==Coroutine::State::READY){
                //防止1个协程占用1个线程
                getTaskFromTimer();
                //重新加入任务队列
                addTask(std::move(cur_coroutine));
            }else 
                cur_coroutine = nullptr;
        }else{
            //从定时器取任务
            getTaskFromTimer();
        }
    }
    logger->info(fmt::format("Scheduler main_coroutine{} thread end",Coroutine::GetMainCoroutine()->Getid()));   
}

void Scheduler::getTaskFromTimer() {
    //logger->info(fmt::format("Scheduler getTaskFromTimer by timer_coroutine{}",Coroutine::GetCurrentCoroutine()->Getid()));   
    std::vector<std::function<void()>> cbs;
    //从定时器取出可以执行的任务
    m_timer->getExpiredcbs(cbs);
    if(cbs.size()>0)
        logger->info(fmt::format("Scheduler getTask size{}",cbs.size())); 
    //加入协程调度器任务队列
    for(size_t i=0;i<cbs.size();i++ ){
        std::function<void()> cb = cbs[i];
        //logger->info(fmt::format("Scheduler getTask by coroutine{}",Coroutine::GetCurrentCoroutine()->Getid()));   
        addTask(std::move(cb)); 
    }
}

}//namespace C_RPC