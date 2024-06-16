
#include "timer.h"
namespace C_RPC{
//debug 日志
static std::shared_ptr<spdlog::logger> logger = LOG_NAME("timer");

TimeEvent::TimeEvent(uint64_t interval, std::function<void()> cb, Timer* timerptr, bool is_repeated)
    : m_interval(interval), m_is_repeated(is_repeated), m_cb(cb),timer(timerptr) {
    //事件发生时间=当前时间+时间间隔
    m_occur_time = GetCurrentMS() + m_interval;  	
    logger->info(fmt::format("timeevent will occur at{}",m_occur_time));   
}

std::shared_ptr<TimeEvent> TimeEvent::getThis() {
    return this->shared_from_this();
}

bool TimeEvent::resetTime() {
    std::lock_guard<std::mutex> lock(timer->m_mutex);
    //从定时器里找到当前定时器事件
    auto it = timer->m_timeEvents.find(getThis());
    if(it!=timer->m_timeEvents.end()){  
        timer->m_timeEvents.erase(it);
        //修改事件发生时间
        m_occur_time = GetCurrentMS() + m_interval; 
        //logger->info(fmt::format("reset time:timeevent will occur at{}",m_occur_time));    	
        m_is_cancled = false;
        timer->m_timeEvents.insert(getThis());
        return true;
    }
    return false;
}

bool TimeEvent::resetTime(uint64_t interval) {
    std::lock_guard<std::mutex> lock(timer->m_mutex);
    //从定时器里找到当前定时器事件
    auto it = timer->m_timeEvents.find(getThis());
    if(it!=timer->m_timeEvents.end()){
        timer->m_timeEvents.erase(it);
        //修改事件发生时间
        m_interval = interval;
        m_occur_time = GetCurrentMS() + m_interval;  	
        m_is_cancled = false;
        timer->m_timeEvents.insert(getThis());
        return true;
    }
    return false;
}

void TimeEvent::wake() {
    std::lock_guard<std::mutex> lock(timer->m_mutex);
    m_is_cancled = false;
    m_is_stoped = false;
  }

void TimeEvent::cancel () {
    std::lock_guard<std::mutex> lock(timer->m_mutex);
    m_is_cancled = true;
}

void TimeEvent::stop () {
    std::lock_guard<std::mutex> lock(timer->m_mutex);
    m_is_stoped = true;
}

void TimeEvent::cancleRepeated () {
    std::lock_guard<std::mutex> lock(timer->m_mutex);
    m_is_repeated = false;
}

void TimeEvent::setRepeated () {
    std::lock_guard<std::mutex> lock(timer->m_mutex);
    m_is_repeated = true;
}
  
std::shared_ptr<TimeEvent> Timer::addTimeEvent(uint64_t interval, std::function<void()> cb, bool is_repeated){
    //创建一个新的定时器事件
    std::shared_ptr<TimeEvent> timeEvent(new TimeEvent(interval, cb, this,is_repeated));
    std::lock_guard<std::mutex> lock(m_mutex);
    //加入定时器事件集合
    m_timeEvents.insert(timeEvent);  
    logger->info(fmt::format("addTimeEvent:m_timeEvents.size()={}",m_timeEvents.size()));   
    return timeEvent;
}

uint64_t Timer::getLatestTEOccur() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if(m_timeEvents.empty()){
        return ~0ull;
    }
    std::shared_ptr<TimeEvent> timeEvent = nullptr;
    //找到最近的定时器事件
    for(std::set<std::shared_ptr<TimeEvent>>::iterator it=m_timeEvents.begin();it!=m_timeEvents.end();++it){
        if((*it)->m_is_cancled!=true){
            timeEvent=*it;
            break;
        }
    }
    if(timeEvent==nullptr)
        return  ~0ull; 
    //返回距离该事件发生还有多久
    uint64_t now = GetCurrentMS();
    if(now >= timeEvent->m_occur_time){
        return 0;
    } else {
        return timeEvent->m_occur_time - now;
    }
}

void Timer::getExpiredcbs(std::vector<std::function<void()>>& cbs) {
    //获取当前时间
    uint64_t now = GetCurrentMS();
    std::vector<std::shared_ptr<TimeEvent>> expired;

    std::lock_guard<std::mutex> lock(m_mutex);
    if(m_timeEvents.empty() ) {
        return;
    }
    //logger->info(fmt::format("getExpiredcbs:m_timeEvents.size()={}",m_timeEvents.size()));   
    //找到所有发生时间小于当前时间的定时器事件
    std::set<std::shared_ptr<TimeEvent>>::iterator it;
    for(it=m_timeEvents.begin();it!=m_timeEvents.end();++it){
        if((*it)->m_occur_time<=now){ 
            if((*it)->m_is_cancled!=true && (*it)->m_is_stoped!=true){
                logger->info(fmt::format("getExpiredcbs:cb occur at{}",now));
                cbs.push_back((*it)->m_cb);
            }
        }else{
            break;
        }
    }
    expired.insert(expired.begin(), m_timeEvents.begin(), it);
    m_timeEvents.erase(m_timeEvents.begin(), it);
    //如果这些定时器事件是循环的，重新加入定时器事件集合
    for(auto &i : expired){
        if(i->m_is_repeated&&i->m_is_cancled!=true){
            i->m_occur_time = now + i->m_interval;
            m_timeEvents.insert(i);
        }
    }

}

}//namespace C_RPC 


