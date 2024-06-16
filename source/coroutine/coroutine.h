#ifndef _COROUTINE_H_
#define _COROUTINE_H_

#include <ucontext.h>
#include <memory>
#include <atomic>
#include <functional>
#include "../util/config.h"
#include "../util/spdlog/spdlog.h"
namespace C_RPC {

class Scheduler;

/**
 * @brief 协程类，继承std::enable_shared_from_this
 */
class Coroutine: public std::enable_shared_from_this<Coroutine>{

public:
    /**
    * @brief 协程状态，READY代表可以被立即调度，BLOCKED代表只能被协程锁（source/coroutine/coroutinesync.h）unlock调度，
    *        WAITING代表只能被协程条件变量notify调度，TERMINATED代表协程终止         
    */
    enum State{
        READY,
        BLOCKED,
        WAITING,
        TERMINATED
    };
    /**
     * @brief 构造函数
     * @param cb 协程执行的函数
     * @param stacksize 协程栈大小
     * @param isMainCoroutine 是否为线程主协程（调度协程） 整个系统实现的是非对称协程
     */
    Coroutine(std::function<void()> cb, size_t stacksize = Coroutine_stacksize, bool isMainCoroutine = false);

    /**
     * @brief 析构函数
     */
    ~Coroutine();

    /**
     * @brief 协程暂停执行，出让给主协程
     */
    void yield();

    /**
     * @brief 主协程调度，协程恢复执行
     */
    void resume();

    /**
     * @brief 获得当前协程状态
     */
    State getState(){ return m_state;}

    /**
     * @brief 设置当前协程状态
     */
    void setState(State state){ m_state=state;}

    /**
     * @brief 获得当前协程id
     */
    uint64_t Getid(){ return m_id;}
public:

    /**
     * @brief 协程所执行的函数，结束后返回到线程主协程
     */
    static void MainFunc();

    /**
     * @brief 获取当前运行协程的智能指针
     */
    static std::shared_ptr<Coroutine> GetCurrentCoroutine();

    /**
     * @brief 获取当前线程主协程的智能指针
     */
    static std::shared_ptr<Coroutine> GetMainCoroutine();

    /**
     * @brief 在当前线程启动协程，当前线程成为主协程
     */
    static void StartCoroutine( );

private:
    //协程id
    uint64_t m_id{0};
    //协程栈大小
    uint32_t m_stacksize{0};
    //协程栈
    void* m_stack{nullptr};
    //协程上下文
    ucontext_t m_ctx;
    //协程所执行的函数
    std::function<void()> m_cb;
    //协程状态
    State m_state{READY};
};

}//namespace C_RPC

#endif //_COROUTINE_H_