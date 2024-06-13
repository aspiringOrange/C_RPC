#ifndef _COROUTINE_CHANNAL_H_
#define _COROUTINE_CHANNAL_H_
#include <semaphore.h>
#include <pthread.h>
#include <atomic>
#include <memory>
#include <queue>
#include "coroutine.h"
#include "coroutinesync.h"
namespace C_RPC {

template<typename T>
class CoroutineChannal{
public:
    CoroutineChannal(size_t capacity)
            : m_is_close(false)
            , m_capacity(capacity){
    }

    ~CoroutineChannal() {
        close();
    }
    /**
     * @brief 发送数据到 Channel
     * @param[in] t 发送的数据
     * @return 返回调用结果
     */
    bool push(const T& t) {
        m_mutex.lock();
        if (m_is_close) {
            m_mutex.unlock();
            return false;
        }
        // 如果缓冲区已满，等待m_pushCv唤醒
        while (m_queue.size() >= m_capacity) {
            m_pushCv.wait(m_mutex);
            if (m_is_close) {
                m_mutex.unlock();
                return false;
            }
        }
        m_queue.push(t);
        m_mutex.unlock();
        // 唤醒m_popCv
        m_popCv.notify();
        return true;
    }
    /**
     * @brief 从 Channel 读取数据
     * @param[in] t 读取到 t
     * @return 返回调用结果
     */
    bool pop(T& t) {
        m_mutex.lock();
        if (m_is_close) {
            m_mutex.unlock();
            return false;
        }
        // 如果缓冲区为空，等待m_pushCv唤醒
        while (m_queue.empty()) {
            m_popCv.wait(m_mutex);
            if (m_is_close) {
                m_mutex.unlock();
                return false;
            }
        }
        t = m_queue.front();
        m_queue.pop();
        m_mutex.unlock();
        // 唤醒 m_pushCv
        m_pushCv.notify();
        return true;
    }

    CoroutineChannal& operator>>(T& t) {
        pop(t);
        return *this;
    }

    CoroutineChannal& operator<<(const T& t) {
        push(t);
        return *this;
    }
    /**
     * @brief 关闭 Channel
     */
    void close() {
        m_mutex.lock();
        if (m_is_close) {
            m_mutex.unlock();
            return;
        }
        m_is_close = true;
        // 唤醒等待的协程
        m_pushCv.notifyAll();
        m_popCv.notifyAll();
        m_mutex.unlock();
    }

    size_t capacity() const {
        return m_capacity;
    }

    size_t size() {
        m_mutex.lock();
        size_t size = m_queue.size();
        m_mutex.unlock();
        return size;
    }

    bool empty() {
        return !size();
    }
private:
    bool m_is_close;
    // Channel 缓冲区大小
    size_t m_capacity;
    // 协程锁和协程条件变量配合使用保护消息队列
    CoroutineMutex m_mutex;
    // 入队条件变量
    CoroutineCV m_pushCv;
    // 出队条件变量
    CoroutineCV m_popCv;
    // 消息队列
    std::queue<T> m_queue;
};




}

#endif 
