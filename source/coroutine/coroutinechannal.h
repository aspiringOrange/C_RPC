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

/**
 * @brief 协程通信消息队列
 */
template<typename T>
class CoroutineChannal{
public:
    /**
     * @brief 协程通信消息队列构造函数
     * @param capacity 队列最大长度
     */
    CoroutineChannal(size_t capacity)
            : m_is_close(false)
            , m_capacity(capacity){
    }

    ~CoroutineChannal() {
        close();
    }

    /**
     * @brief 发送数据到 Channel
     * @param t 发送的数据
     * @return 返回调用结果
     */
    bool push(const T& t) {
        m_mutex.lock();
        //如果消息队列关闭
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
        //写入数据
        m_queue.push(t);
        m_mutex.unlock();
        // 唤醒m_popCv，唤醒需要接受消息的协程
        m_popCv.notify();
        return true;
    }

    /**
     * @brief 从 Channel 读取数据
     * @param t 读取到 t
     * @return 返回调用结果
     */
    bool pop(T& t) {
        m_mutex.lock();
        //如果消息队列关闭
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
        //读出数据
        t = m_queue.front();
        m_queue.pop();
        m_mutex.unlock();
        // 唤醒 m_pushCv，唤醒需要写入数据的协程
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

    /**
     * @brief 返回队列容量
     */
    size_t capacity() const {
        return m_capacity;
    }

    /**
     * @brief 返回队列当前长度
     */
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
    // Channel 消息队列大小
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




}//namespace C_RPC 

#endif //_COROUTINE_CHANNAL_H_
