#ifndef _TIMER_H_
#define _TIMER_H_
#include <functional>
#include <memory>
#include <set>
#include <mutex>
#include "utils.h"
#include "config.h"
namespace C_RPC{

class Timer;
class TimeEvent : public std::enable_shared_from_this<TimeEvent>{
friend class Timer;
public:

  TimeEvent(uint64_t interval, std::function<void()> cb, Timer* timerptr, bool is_repeated = false);

  std::shared_ptr<TimeEvent> getThis();

  bool resetTime() ;

  bool resetTime(uint64_t interval);

  void wake();

  void cancel();

  void stop();

  void cancleRepeated ();

  void setRepeated ();
  struct Compare{
      bool operator()(const std::shared_ptr<TimeEvent> &l, const std::shared_ptr<TimeEvent> &r) const {
        return l->m_occur_time<r->m_occur_time;
      }
  };
private:
    Timer* timer;
    uint64_t m_occur_time;  
    uint64_t m_interval; 
    bool m_is_repeated {false};
	  bool m_is_cancled {false};
    bool m_is_stoped {false};
    std::function<void()> m_cb;
};

class Timer{
friend class TimeEvent;
public:
    std::shared_ptr<TimeEvent> addTimeEvent(uint64_t interval, std::function<void()> cb, bool is_repeated = false);
    void getExpiredcbs(std::vector<std::function<void()>>& cbs);
    uint64_t getLatestTEOccur();//epoll_wait不能阻塞超过这个时间，但是epoll_wait阻塞的线程可以不是调度协程吧。。。
private:
    std::mutex m_mutex;
    std::set<std::shared_ptr<TimeEvent>,TimeEvent::Compare> m_timeEvents;
};




}
#endif
