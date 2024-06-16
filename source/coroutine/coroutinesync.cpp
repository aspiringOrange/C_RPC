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
        //修改当前协程的状态
        Coroutine::GetCurrentCoroutine()->setState(Coroutine::State::BLOCKED);
        m_gaurd.unlock();
        // 让出协程
        Coroutine::GetCurrentCoroutine()->yield();
        //这里存在并发冲突
        //事实上必须要求当前协程yield后才能被占用锁的协程释放锁时加入调度器，
        //一种解决方案是占用锁的协程可以通过协程id查询协程等待队列里的协程是否yield，
        //主协程应该保存所有协程是否yield了，因为当前协程yiled后会返回主协程，而主协程上一个调度的协程就是当前协程
    }
    // 成功获取锁将当前拥有锁的协程id改成自己的id
    m_gaurd.lock();
    m_coroutineId = Coroutine::GetCurrentCoroutine()->Getid();
    m_gaurd.unlock();
    //std::cout<<m_name<<" lock()"<<std::endl;
}

void CoroutineMutex::unlock() {
    // 如果本协程没有持有锁就退出
    if (Coroutine::GetCurrentCoroutine()->Getid() != m_coroutineId) {
        return;
    }
    //释放锁
    m_gaurd.lock();
    m_coroutineId = 0;
    std::shared_ptr<Coroutine> wait_coroutine = nullptr;
    if (!m_waitQueue.empty()) {
        // 获取一个等待的协程
        wait_coroutine = m_waitQueue.front();
        m_waitQueue.pop();
    }
    m_gaurd.unlock();
    //释放锁
    m_mutex.unlock();

    if (wait_coroutine!=nullptr) {
        // 将等待的协程重新加入调度
        wait_coroutine->setState(Coroutine::State::READY);
        Scheduler::GetInstance()->addTask(std::move(wait_coroutine));
    }
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

void CoroutineCV::wait(CoroutineMutex &mutex) {//如果已经持有锁的情况下调用，避免死锁or资源无法及时通知自己
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

}//namespace C_RPC