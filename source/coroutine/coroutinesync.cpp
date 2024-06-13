#include "coroutinesync.h"
#include "scheduler.h"
#include <iostream>
namespace C_RPC{

bool CoroutineMutex::tryLock() {
    return m_mutex.tryLock();
}

void CoroutineMutex::lock() {
    // 如果本协程已经持有锁就退出
    if (Coroutine::GetCurrentCoroutine()->Getid() == m_coroutineId) {
        return;
    }
    // 第一次尝试获取锁
    while (!tryLock()) {
        // 加锁保护等待队列
        m_gaurd.lock();
        // 将自己加入协程等待队列
        m_waitQueue.push(Coroutine::GetCurrentCoroutine());
        std::cout<<m_name<<" m_waitQueue.size()"<<m_waitQueue.size()<<std::endl;
        Coroutine::GetCurrentCoroutine()->setState(Coroutine::State::BLOCKED);
        m_gaurd.unlock();
        // 让出协程
        Coroutine::GetCurrentCoroutine()->yield();
        //事实上必须要求yield后才能被占用锁的协程加入调度器，一种解决方案是占用锁的协程可以通过协程id查询是否yield，yield协程对应的主协程应该知道它调度了谁，那么就可以保存谁yield了
        //主协程来二次确定一个协程已经yield了，2PC的思想。
    }
    // 成功获取锁将m_fiberId改成自己的id
    m_coroutineId = Coroutine::GetCurrentCoroutine()->Getid();
    //std::cout<<m_name<<" lock()"<<std::endl;
}

void CoroutineMutex::unlock() {
    if (Coroutine::GetCurrentCoroutine()->Getid() != m_coroutineId) {
        return;
    }
    m_coroutineId = 0;
    m_gaurd.lock();
    std::shared_ptr<Coroutine> wait_coroutine = nullptr;
    if (!m_waitQueue.empty()) {
        // 获取一个等待的协程
        wait_coroutine = m_waitQueue.front();
        m_waitQueue.pop();
    }
    m_gaurd.unlock();

    m_mutex.unlock();

    if (wait_coroutine!=nullptr) {
        // 将等待的协程重新加入调度
        //std::cout<<m_name<<" 将等待的协程重新加入调度"<<std::endl;
        wait_coroutine->setState(Coroutine::State::READY);
        Scheduler::GetInstance()->addTask(std::move(wait_coroutine));
    }
    //std::cout<<m_name<<" unlock()"<<std::endl;
}


void CoroutineCV::notify() {
    std::shared_ptr<Coroutine> wait_coroutine;
    
    // 获取一个等待的协程
    m_mutex.lock();
    if (m_waitQueue.empty()) {
        m_mutex.unlock();
        return;
    }
    wait_coroutine = m_waitQueue.front();
    m_waitQueue.pop();
    m_mutex.unlock();
    // 将协程重新加入调度
    if (wait_coroutine!=nullptr) {
        // 将等待的协程重新加入调度
        wait_coroutine->setState(Coroutine::State::READY);
        Scheduler::GetInstance()->addTask(std::move(wait_coroutine));
    }
}

void CoroutineCV::notifyAll() {
    m_mutex.lock();
    // 将全部等待的协程重新加入调度
    while (!m_waitQueue.empty()) {
        std::shared_ptr<Coroutine> wait_coroutine = m_waitQueue.front();
        m_waitQueue.pop();
        if (wait_coroutine!=nullptr) {
            // 将等待的协程重新加入调度
            wait_coroutine->setState(Coroutine::State::READY);
            Scheduler::GetInstance()->addTask(std::move(wait_coroutine));
        }
    }
    m_mutex.unlock();
}

void CoroutineCV::wait() {
    std::shared_ptr<Coroutine> wait_coroutine = Coroutine::GetCurrentCoroutine();
    m_mutex.lock();
    // 将自己加入等待队列
    m_waitQueue.push(std::move(wait_coroutine));
    Coroutine::GetCurrentCoroutine()->setState(Coroutine::State::WAITING);
    m_mutex.unlock();
    // 让出协程
    Coroutine::GetCurrentCoroutine()->yield();
}

void CoroutineCV::wait(CoroutineMutex &mutex) {//如果已经持有锁，否则死锁or资源无法及时通知自己
    std::shared_ptr<Coroutine> wait_coroutine = Coroutine::GetCurrentCoroutine();
    m_mutex.lock();
    // 将自己加入等待队列
    m_waitQueue.push(std::move(wait_coroutine));
    mutex.unlock();
    Coroutine::GetCurrentCoroutine()->setState(Coroutine::State::WAITING);
    // 让出协程
    m_mutex.unlock();
    Coroutine::GetCurrentCoroutine()->yield();
    // 重新获取锁
    mutex.lock();
}

}