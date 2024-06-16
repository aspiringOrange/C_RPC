#include "coroutine.h"

namespace C_RPC{
//debug日志
static std::shared_ptr<spdlog::logger> logger = LOG_NAME("coroutine");
//当前协程指针
static thread_local std::shared_ptr<Coroutine> current_coroutine = nullptr;
//当前线程主协程指针
static thread_local std::shared_ptr<Coroutine> main_coroutine = nullptr;
//全局协程数量
static std::atomic<uint64_t> m_coroutine_id{0};

Coroutine::Coroutine( std::function<void()> cb, size_t stacksize , bool isMainCoroutine)
        : m_id(++m_coroutine_id)
        , m_cb(cb)
        , m_stacksize(stacksize){

    logger->info(fmt::format("Coroutine({},{},{}),main_coroutine{}",!!cb,stacksize,!!isMainCoroutine,!!main_coroutine));  
    //一个线程只对应一个主协程
    ENSURE((isMainCoroutine && main_coroutine==nullptr)||(!isMainCoroutine));
    //获取当前上下文
    getcontext(&m_ctx);
    //普通协程
    if(!isMainCoroutine){
        ENSURE(stacksize>0)
        m_stack = malloc(m_stacksize);
        // ENSURE(m_stack!=nullptr);
        m_ctx.uc_link = nullptr;//可以link别的上下文，执行完本协程的函数后恢复该上下文
        m_ctx.uc_stack.ss_size = m_stacksize;//栈大小
        m_ctx.uc_stack.ss_sp = m_stack;//栈
        makecontext(&m_ctx,MainFunc,0);//绑定函数入口
    }
    //当前协程可以立即被调度
    m_state = READY;
    logger->info(fmt::format("Coroutine{}",m_id));  
}

Coroutine::~Coroutine() {
    logger->info(fmt::format("~Coroutine{}",m_id));  
    //释放栈
    free(m_stack);
}


void Coroutine::resume() {
    //确定当前主协程存在
    ENSURE(main_coroutine!=nullptr);
    //修改当前执行协程
    current_coroutine = this->shared_from_this();
    //主协程与本协程交换上下文
    swapcontext(&main_coroutine->m_ctx,&m_ctx);
}

void Coroutine::yield() {
    //确定当前主协程存在
    ENSURE(main_coroutine!=nullptr);
    //修改当前执行协程为主协程
    current_coroutine = main_coroutine;
    //主协程与本协程交换上下文
    swapcontext(&m_ctx,&main_coroutine->m_ctx);
}

std::shared_ptr<Coroutine> Coroutine::GetCurrentCoroutine(){
    return current_coroutine;
}

std::shared_ptr<Coroutine> Coroutine::GetMainCoroutine(){
    return main_coroutine;
}

void Coroutine::MainFunc(){
    //保证当前执行协程不为空
    ENSURE(current_coroutine!=nullptr);
    //执行函数
    current_coroutine->m_cb();
    current_coroutine->m_cb = nullptr;
    current_coroutine->m_state = TERMINATED;
    //出让协程，恢复到主协程
    current_coroutine->yield();
}

void Coroutine::StartCoroutine(){
    //如果当前线程没有主协程
    if(!main_coroutine){
        //初始化主协程
        main_coroutine = current_coroutine = std::shared_ptr<Coroutine>(new Coroutine(nullptr, 0, true));
    }
}

}//namespace C_RPC
