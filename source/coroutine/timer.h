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
/**
 * @brief 定时器事件
 */
class TimeEvent : public std::enable_shared_from_this<TimeEvent>{
friend class Timer;
public:
  /**
   * @brief 定时器事件构造函数
   * @param interval 定时间隔，多久后执行
   * @param cb，回调函数
   * @param timerptr，定时器
   * @param is_repeated，是否循环
   */
  TimeEvent(uint64_t interval, std::function<void()> cb, Timer* timerptr, bool is_repeated = false);

  /**
   * @brief 获取定时器事件指针
   */
  std::shared_ptr<TimeEvent> getThis();

  /**
   * @brief 重置定时器事件
   */
  bool resetTime() ;

  /**
   * @brief 重置定时器事件
   * @param interval 重置的时间间隔
   */
  bool resetTime(uint64_t interval);

  /**
   * @brief 唤醒定时器事件
   */
  void wake();

  /**
   * @brief 取消定时器事件
   */
  void cancel();

   /**
   * @brief 暂停定时器事件
   */
  void stop();

  /**
   * @brief 取消定时器事件循环
   */
  void cancleRepeated ();

  /**
   * @brief 设置定时器事件循环
   */
  void setRepeated ();

  /**
   * @brief 比较定时器事件优先级
   */
  struct Compare{
      bool operator()(const std::shared_ptr<TimeEvent> &l, const std::shared_ptr<TimeEvent> &r) const {
        return l->m_occur_time<r->m_occur_time;
      }
  };
private:
    //定时器指针
    Timer* timer;
    //定时器事件发生的时间点
    uint64_t m_occur_time;  
    //定时器事件的定时间隔，多久后执行
    uint64_t m_interval; 
    bool m_is_repeated {false};
	  bool m_is_cancled {false};
    bool m_is_stoped {false};
    //定时器事件的回调函数
    std::function<void()> m_cb;
};

/**
  * @brief 定时器
  */
class Timer{
friend class TimeEvent;
public:
    /**
      * @brief 添加定时器事件
      * @param interval 定时间隔
      * @param cb 回调函数
      * @param is_repeated 是否循环
      * @return 定时器事件指针
      */
    std::shared_ptr<TimeEvent> addTimeEvent(uint64_t interval, std::function<void()> cb, bool is_repeated = false);
    /**
      * @brief 获取可执行的定时器事件回调函数
      * @param[out] cbs 回调函数数组
      */
    void getExpiredcbs(std::vector<std::function<void()>>& cbs);
    /**
      * @brief 获取距离下一个定时器事件的时间间隔
      */
    uint64_t getLatestTEOccur();
private:
    std::mutex m_mutex;
    //定时器事件集合
    std::set<std::shared_ptr<TimeEvent>,TimeEvent::Compare> m_timeEvents;
};

}//namespace C_RPC 
#endif //_TIMER_H_
