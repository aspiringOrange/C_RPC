#ifndef _COROUTINE_SYNC_H_
#define _COROUTINE_SYNC_H_
#include <semaphore.h>
#include <pthread.h>
#include <atomic>
#include <memory>
#include <queue>
#include "coroutine.h"
#include "scheduler.h"
namespace C_RPC {

/**
 * @brief 自旋锁
 */
class SpinLock {
public:
    SpinLock(){
        pthread_spin_init(&m_mutex,0);
    }
    ~SpinLock(){
        pthread_spin_destroy(&m_mutex);
    }
    void lock(){
        pthread_spin_lock(&m_mutex);
    }
    bool tryLock() {
        return !pthread_spin_trylock(&m_mutex);
    }
    void unlock(){
        pthread_spin_unlock(&m_mutex);
    }
private:
    pthread_spinlock_t m_mutex;
};

/**
 * @brief 协程锁
 */
class CoroutineMutex {
public:
    bool tryLock();

    void lock();

    void unlock();

    void setName(std::string name){
        m_name = name;
    }
private:
    //协程名，for debug
    std::string m_name;
    // 协程所持有的锁
    SpinLock m_mutex;
    // 保护等待队列的锁
    SpinLock m_gaurd;
    // 持有锁的协程id
    uint64_t m_coroutineId{0};
    // 协程等待队列
    std::queue<std::shared_ptr<Coroutine>> m_waitQueue;
};

/**
 * @brief 协程条件变量
 */
class CoroutineCV{
public:
    /**
     * @brief 唤醒一个等待的协程
     */
    void notify();
    /**
     * @brief 唤醒全部等待的协程
     */
    void notifyAll();
    /**
     * @brief 不加锁地等待唤醒
     */
    void wait();
    /**
     * @brief 等待唤醒
     */
    void wait(CoroutineMutex &mutex);

private:
    // 协程等待队列
    std::queue<std::shared_ptr<Coroutine>> m_waitQueue;
    // 保护协程等待队列
    SpinLock m_mutex;
};

}//namespace C_RPC 

#endif //_COROUTINE_SYNC_H_
