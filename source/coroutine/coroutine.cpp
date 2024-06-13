#include "coroutine.h"

namespace C_RPC{

static std::shared_ptr<spdlog::logger> logger = LOG_NAME("coroutine");
//当前协程指针
static thread_local std::shared_ptr<Coroutine> current_coroutine = nullptr;
//当前线程主协程指针
static thread_local std::shared_ptr<Coroutine> main_coroutine = nullptr;

static std::atomic<uint64_t> m_coroutine_id{0};

Coroutine::Coroutine( std::function<void()> cb, size_t stacksize , bool isMainCoroutine)
        : m_id(++m_coroutine_id)
        , m_cb(cb)
        , m_stacksize(stacksize){
    logger->info(fmt::format("Coroutine({},{},{}),main_coroutine{}",!!cb,stacksize,!!isMainCoroutine,!!main_coroutine));  
    ENSURE((isMainCoroutine && main_coroutine==nullptr)||(!isMainCoroutine));

    getcontext(&m_ctx);

    if(!isMainCoroutine){
        ENSURE(stacksize>0)
        m_stack = malloc(m_stacksize);
       // ENSURE(m_stack!=nullptr);
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_size = m_stacksize;
        m_ctx.uc_stack.ss_sp = m_stack;
        makecontext(&m_ctx,MainFunc,0);
    }
    m_state = READY;
    logger->info(fmt::format("Coroutine{}",m_id));  
}

Coroutine::~Coroutine() {
    logger->info(fmt::format("~Coroutine{}",m_id));  
    free(m_stack);
}


void Coroutine::resume() {
    ENSURE(main_coroutine!=nullptr);
    current_coroutine = this->shared_from_this();
    swapcontext(&main_coroutine->m_ctx,&m_ctx);
}

void Coroutine::yield() {
    ENSURE(main_coroutine!=nullptr);
    current_coroutine = main_coroutine;
    swapcontext(&m_ctx,&main_coroutine->m_ctx);
}

std::shared_ptr<Coroutine> Coroutine::GetCurrentCoroutine(){
    return current_coroutine;
}

std::shared_ptr<Coroutine> Coroutine::GetMainCoroutine(){
    return main_coroutine;
}


void Coroutine::MainFunc(){
    ENSURE(current_coroutine!=nullptr);
    current_coroutine->m_cb();
    current_coroutine->m_cb = nullptr;
    current_coroutine->m_state = TERMINATED;
    current_coroutine->yield();
}


void Coroutine::StartCoroutine(){
    if(!main_coroutine){
        main_coroutine = current_coroutine = std::shared_ptr<Coroutine>(new Coroutine(nullptr, 0, true));
    }
}


}
