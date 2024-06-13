
#include "timer.h"
namespace C_RPC{
static std::shared_ptr<spdlog::logger> logger = LOG_NAME("timer");

TimeEvent::TimeEvent(uint64_t interval, std::function<void()> cb, Timer* timerptr, bool is_repeated)
    : m_interval(interval), m_is_repeated(is_repeated), m_cb(cb),timer(timerptr) {
    m_occur_time = GetCurrentMS() + m_interval;  	
    logger->info(fmt::format("timeevent will occur at{}",m_occur_time));   
}

std::shared_ptr<TimeEvent> TimeEvent::getThis() {
    return this->shared_from_this();
}

bool TimeEvent::resetTime() {
    std::lock_guard<std::mutex> lock(timer->m_mutex);
    auto it = timer->m_timeEvents.find(getThis());
    if(it!=timer->m_timeEvents.end()){  
        timer->m_timeEvents.erase(it);
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
    auto it = timer->m_timeEvents.find(getThis());
    if(it!=timer->m_timeEvents.end()){
        timer->m_timeEvents.erase(it);
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
    std::shared_ptr<TimeEvent> timeEvent(new TimeEvent(interval, cb, this,is_repeated));
    std::lock_guard<std::mutex> lock(m_mutex);
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
    for(std::set<std::shared_ptr<TimeEvent>>::iterator it=m_timeEvents.begin();it!=m_timeEvents.end();++it){
        if((*it)->m_is_cancled!=true){
            timeEvent=*it;
            break;
        }
    }
    if(timeEvent==nullptr)
        return  ~0ull; 
    uint64_t now = GetCurrentMS();
    if(now >= timeEvent->m_occur_time){
        return 0;
    } else {
        return timeEvent->m_occur_time - now;
    }
}

void Timer::getExpiredcbs(std::vector<std::function<void()>>& cbs) {

    uint64_t now = GetCurrentMS();
    std::vector<std::shared_ptr<TimeEvent>> expired;

    std::lock_guard<std::mutex> lock(m_mutex);
    if(m_timeEvents.empty() ) {
        return;
    }
    //logger->info(fmt::format("getExpiredcbs:m_timeEvents.size()={}",m_timeEvents.size()));   
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

    for(auto &i : expired){
        if(i->m_is_repeated&&i->m_is_cancled!=true){
            i->m_occur_time = now + i->m_interval;
            m_timeEvents.insert(i);
        }
    }

}
}


