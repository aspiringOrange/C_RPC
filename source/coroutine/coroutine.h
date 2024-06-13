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
 * @brief 协程类
 */
class Coroutine: public std::enable_shared_from_this<Coroutine>{

public:

    enum State{
        READY,
        BLOCKED,
        WAITING,
        TERMINATED
    };
    /**
     * @brief 协程构造函数
     * @param id 协程id
     * @param cb 协程执行的函数
     * @param stacksize 协程栈大小
     * @param isMainCoroutine 是否为线程主协程（调度协程）
     */
    Coroutine(std::function<void()> cb, size_t stacksize = Coroutine_stacksize, bool isMainCoroutine = false);

    /**
     * @brief 析构函数
     */
    ~Coroutine();


    void yield();

    void resume();

    State getState(){ return m_state;}

    void setState(State state){ m_state=state;}

    uint64_t Getid(){ return m_id;}
public:

    /**
     * @brief 协程执行函数，结束后返回到线程主协程
     */
    static void MainFunc();

    /**
     * @brief 获取当前运行协程id
     * @return 协程id
     */
    static std::shared_ptr<Coroutine> GetCurrentCoroutine();

    /**
     * @brief 获取当前线程主协程id
     * @return 协程id
     */
    static std::shared_ptr<Coroutine> GetMainCoroutine();

    //在当前线程启用协程
    static void StartCoroutine( );
private:
    uint64_t m_id{0};
    uint32_t m_stacksize{0};
    ucontext_t m_ctx;
    void* m_stack{nullptr};
    std::function<void()> m_cb;
    State m_state{READY};
};

}

#endif