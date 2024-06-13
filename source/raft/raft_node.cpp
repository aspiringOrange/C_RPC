#include "raft_node.h"

namespace C_RPC{

    namespace Raft{
    RaftNode::RaftNode(uint64_t id, std::vector<ip_port> nodes):m_nodes(nodes),m_id(id),TCPServer(nodes[id].m_ip,nodes[id].m_port),
    m_raft_logger(id,m_funcs,m_addresses,m_subscribes){
        m_raft_mutex.setName("m_raft_mutex");

        std::string logger_name = "raft/node";
        logger_name += m_id+'0';
        logger = LOG_NAME(logger_name);
        for(size_t i;i<m_nodes.size();i++){
            if(i == m_id) continue;
            m_nextIndexs[i]=0;
            m_matchIndexs[i]=0;
        }
        //绑定本地
        while(!TCPServer::start()) 
            sleep(1);
        //连接remote
        Scheduler::GetInstance()->addTask([this](){
            //连接remote
            for(size_t i=0;i<m_nodes.size(); i++){
                if(i == m_id) continue;
                logger->info(fmt::format("try to connect raft_node {}",i)); 
                std::shared_ptr<RpcSession> session(new RpcSession);
                if(session->connect(Address(m_nodes[i].m_ip,m_nodes[i].m_port))){
                    logger->info(fmt::format("connect raft_node {}",i)); 
                }
                m_raft_mutex.lock();
                m_remote_nodes[i]=session;
                m_raft_mutex.unlock();
            }
            logger->info(fmt::format("raft_node {} init",m_id)); 
        });
        //开启选举超时定时器,如果规定时间内没有收到心跳则转变为候选者
        m_electionTimer = Scheduler::GetTimer()->addTimeEvent(genRandom(),[this](){
            std::cout<<"start eletion"<<std::endl;
            m_raft_mutex.lock();
            if (m_state != RaftState::Leader) {
                //变成候选者
                m_state = RaftState::Candidate;
                //自增任期
                ++m_currentTerm;
                m_votedFor = m_id;
                m_leaderId = -1;
                m_raft_mutex.unlock();
                //发起选举
                startElection();
                m_raft_mutex.lock();
            }
            m_raft_mutex.unlock();
        },true);
        //leader开启心跳计时器
        m_heartbeatTimer = Scheduler::GetTimer()->addTimeEvent(300,[this](){
            logger->info(fmt::format("HEART_BEAT to remote node")); 

            for(auto &node : m_remote_nodes){
                //std::cout<<"try get m_heartbeatTimer"<<std::endl;
                m_raft_mutex.lock();
                //std::cout<<"try get m_heartbeatTimer end"<<std::endl;
                if (m_state != RaftState::Leader) {
                    m_raft_mutex.unlock();
                    return;
                }
                Entry send_entry;
                send_entry.op = "heartbeat";

                AppendEntriesParams request{};
                request.term = m_currentTerm;
                request.leaderId = m_id;
                request.leaderCommit = m_raft_logger.getCommitIndex();
                request.prevLogIndex = m_nextIndexs[node.first] - 1;
                request.prevLogTerm = m_raft_logger.getEntry(request.prevLogIndex).term;
                request.entry = send_entry;
                Message heart_message(C_RPC::Message::RPC_REQUEST);
                Serializer s;
                //先写文函数名称
                std::string f_name = "appendEntries";
                s<<f_name;
                s<<request;
                heart_message.encode(s);
                if(!node.second->testAndreConnect(Address(m_nodes[node.first].m_ip,m_nodes[node.first].m_port))){
                    logger->info(fmt::format("heartbeat Failed to connect node{}",node.first));  
                    m_raft_mutex.unlock();
                    continue;
                }
                std::shared_ptr<CoroutineChannal<Message>> recv_channal = node.second->send(heart_message);
                m_raft_mutex.unlock();

                bool timeout = false;
                std::shared_ptr<C_RPC::TimeEvent> timer = C_RPC::Scheduler::GetTimer()->addTimeEvent(100,[&timer,&recv_channal,&timeout](){
                    timeout = true;
                    recv_channal->close();
                });

                Message receive_message;
                // 等待 response，Channel内部会挂起协程，如果有消息到达或者被关闭则会被唤醒
                (*recv_channal) >> receive_message;

                if(timer){
                    timer->cancel();
                }

                // 删除序列号与 Channel 的映射
                node.second->earseResponseChannal(heart_message.seq);

                if (timeout) {
                    logger->info(fmt::format("HEART_BEAT to remote node{} falied",node.first));  
                }
                else{
                    std::shared_ptr<Serializer> s =receive_message.getSerializer();
                    AppendEntriesRets result;
                    while(!s->isReadEnd()){
                        (*s)>>result;
                    }
                }
            }
        },true);
        m_heartbeatTimer->stop();
    }

    void RaftNode::startElection(){
            m_raft_mutex.lock();
            RaftVoteParams request;
            request.term = m_currentTerm;
            request.candidateId = m_id;
            request.lastLogIndex = m_raft_logger.getLastIndex();
            request.lastLogTerm = m_raft_logger.getLastTerm();
            logger->info(fmt::format("startElection :term{}, candidateId{}, lastLogIndex{}, lastLogTerm{}",
                            request.term,request.candidateId,request.lastLogIndex,request.lastLogTerm)); 

            m_votedFor = m_id;
            m_electionTimer->resetTime(genRandom());
            m_raft_mutex.unlock();
            // 统计投票结果，自己开始有一票
            std::shared_ptr<int64_t> grantedVotes = std::make_shared<int64_t>(1);

            for (auto node: m_remote_nodes) {
                // 使用协程发起异步投票，不阻塞选举定时器，才能在选举超时后发起新的选举
                Scheduler::GetInstance()->addTask([grantedVotes, request, node, this] {
                    Message message(C_RPC::Message::RPC_REQUEST);
                    Serializer s;
                    //先写文函数名称
                    std::string f_name = "raftVote";
                    s<<f_name;
                    RaftVoteParams params = request;
                    s<<params;
                    message.encode(s);

                    if(!node.second->testAndreConnect(Address(m_nodes[node.first].m_ip,m_nodes[node.first].m_port))){
                        logger->info(fmt::format("raftVote Failed to connect node{}",node.first));  
                        return;
                    }
                    std::shared_ptr<CoroutineChannal<Message>> recv_channal = node.second->send(message);

                    bool timeout = false;
                    std::shared_ptr<C_RPC::TimeEvent> timer = C_RPC::Scheduler::GetTimer()->addTimeEvent(100,[&timer,&recv_channal,&timeout](){
                        timeout = true;
                        recv_channal->close();
                    });

                    Message receive_message;
                    // 等待 response，Channel内部会挂起协程，如果有消息到达或者被关闭则会被唤醒
                    (*recv_channal) >> receive_message;

                    if(timer){
                        timer->cancel();
                    }

                    // 删除序列号与 Channel 的映射
                    node.second->earseResponseChannal(message.seq);
                    RaftVoteRets result;
                    if (timeout) {
                        logger->info(fmt::format("Failed to raftVote node{} :time_out",node.first));  
                        return;
                    }
                    else{
                        //解析函数名称加地址
                        std::shared_ptr<Serializer> s =receive_message.getSerializer();
                        
                        while(!s->isReadEnd()){
                            (*s)>>result;
                        }
                    }

                    logger->info(fmt::format("ratfVote from node{} :term{}, voteGranted{}",
                            node.first,result.term,result.voteGranted)); 

                    m_raft_mutex.lock();
                    // 检查自己状态是否改变
                    if (m_currentTerm == result.term && m_state == RaftState::Candidate) {
                        if (result.voteGranted) {
                            ++(*grantedVotes);
                            // 赢得选举
                            if (*grantedVotes >= ((uint64_t)m_nodes.size() + 1) / 2) {
                                 // 成为leader停止选举定时器
                                if (m_electionTimer) {
                                    m_electionTimer->stop();
                                }
                                m_state = RaftState::Leader;
                                m_leaderId = m_id;
                                // 成为领导者后，领导者并不知道其它节点的日志情况，因此与其它节点需要同步那么日志，领导者并不知道集群其他节点状态，
                                // 因此他选择了不断尝试。nextIndex、matchIndex 分别用来保存其他节点的下一个待同步日志index、已匹配的日志index。
                                // nextIndex初始化值为lastIndex+1，即领导者最后一个日志序号+1，因此其实这个日志序号是不存在的，显然领导者也不
                                // 指望一次能够同步成功，而是拿出一个值来试探。matchIndex初始化值为0，这个很好理解，因为他还未与任何节点同步成功过，
                                // 所以直接为0。
                                for (auto node: m_remote_nodes) {
                                    //写入no-op日志后，才可以写入新的有效日志。
                                    //发送1个日志（op = "no-op")给协程
                                    //协程同步日志时，收到大多数node的ack，然后把日志应用到本地并且通知所有node可以应用（通过下一次日志或心跳的leadercommit）
                                    m_nextIndexs[node.first] = m_raft_logger.getLastIndex() + 1;
                                    m_matchIndexs[node.first] = 0;
                                }
                                Entry entry;
                                entry.op = "no-op";
                                entry.term = m_currentTerm;
                                m_raft_logger.appendEntry(entry);
                                
                                logger->info(fmt::format("raftnode {} become leader",m_id)); 
                                std::cout<<"raftnode "<<m_id<<" become leader"<<std::endl;
                                // 成为领导者，发起一轮心跳
                                //broadcastHeartbeat();
                                // 开启心跳定时器
                                m_heartbeatTimer->wake();
                                //
                                startReplicateLog();
                            }
                        }
                    }
                    else if(m_currentTerm < result.term){
                        // 成为follower停止心跳定时器
                        if (m_heartbeatTimer && m_state == RaftState::Leader) {
                            m_heartbeatTimer->stop();
                            m_electionTimer->wake();
                        }
                        m_state = RaftState::Follower;
                        m_currentTerm = result.term;
                        m_votedFor = -1;
                        m_leaderId = -1;
                    }
                    m_raft_mutex.unlock();
                });
            }
    }

    void RaftNode::startReplicateLog(){
        //创建n个同步log的协程
        //每个协程负责同步1个节点，replicateLog(i);
        for(uint64_t nodeId = 0;nodeId<m_nodes.size();nodeId++){
            if(nodeId == m_id)
                continue;
            Scheduler::GetInstance()->addTask([this,nodeId](){
                replicateLog(nodeId);
            });
        }
    }

    void RaftNode::replicateLog(uint64_t nodeId) {
        // 发送 rpc的时候一定不要持锁，否则很可能产生死锁。
        while(true){
            Coroutine::GetCurrentCoroutine()->yield();
            //std::cout<<"try get replicateLog"<<std::endl;
            m_raft_mutex.lock();
            //std::cout<<"try get replicateLog end"<<std::endl;
            if (m_state != RaftState::Leader) {
                m_raft_mutex.unlock();
                return;
            }

            //获得要发送的日志，如果日志为空则continue；
            Entry send_entry = m_raft_logger.getEntry(m_nextIndexs[nodeId]);
            if(send_entry.op=="null"){
                m_raft_mutex.unlock();
                continue;
            }

            AppendEntriesParams request{};
            request.term = m_currentTerm;
            request.leaderId = m_id;
            request.leaderCommit = m_raft_logger.getCommitIndex();
            request.prevLogIndex = m_nextIndexs[nodeId] - 1;
            request.prevLogTerm = m_raft_logger.getEntry(request.prevLogIndex).term;
            request.entry = send_entry;
            m_raft_mutex.unlock();

            Message message(C_RPC::Message::RPC_REQUEST);
            Serializer s;
            //先写文函数名称
            std::string f_name = "appendEntries";
            s<<f_name;
            s<<request;
            message.encode(s);

            if(!m_remote_nodes[nodeId]->testAndreConnect(Address(m_nodes[nodeId].m_ip,m_nodes[nodeId].m_port))){
                logger->info(fmt::format("appendEntries Failed to connect node{}",nodeId));  
                continue;
            }

            std::shared_ptr<CoroutineChannal<Message>> recv_channal = m_remote_nodes[nodeId]->send(message);

            bool timeout = false;
            std::shared_ptr<C_RPC::TimeEvent> timer = C_RPC::Scheduler::GetTimer()->addTimeEvent(100,[&timer,&recv_channal,&timeout](){
                timeout = true;
                recv_channal->close();
            });

            Message receive_message;
            // 等待 response，Channel内部会挂起协程，如果有消息到达或者被关闭则会被唤醒
            (*recv_channal) >> receive_message;

            if(timer){
                timer->cancel();
            }

            // 删除序列号与 Channel 的映射
            m_remote_nodes[nodeId]->earseResponseChannal(message.seq);
            AppendEntriesRets result;
            if (timeout) {
                logger->info(fmt::format("Failed to appendEntries node{} :time_out",nodeId));  
                continue;
            }
            else{
                //解析函数名称加地址
                std::shared_ptr<Serializer> s =receive_message.getSerializer();
                
                while(!s->isReadEnd()){
                    (*s)>>result;
                }
            }


            logger->info(fmt::format("appendEntries from node{} :success{}, term{}",
                            nodeId,result.success,result.term)); 

            m_raft_mutex.lock();
            if (m_state != RaftState::Leader) {
                m_raft_mutex.unlock();
                return;
            }

            // 如果因为网络原因集群选举出新的leader则自己变成follower
            if (result.term > m_currentTerm) {
                // 成为follower停止心跳定时器
                if (m_heartbeatTimer && m_state == RaftState::Leader) {
                    m_electionTimer->wake();
                    m_heartbeatTimer->stop();
                }
                m_state = RaftState::Follower;
                m_currentTerm = result.term;
                m_votedFor = -1;
                m_leaderId = -1;
                m_raft_mutex.unlock();
                return;
            }
            // 过期的响应
            if (result.term < m_currentTerm) {
                m_raft_mutex.unlock();
                continue;
            }
            // 日志追加失败，根据 nextIndex 更新 m_nextIndex 和 m_matchIndex
            if (!result.success) {
                if(m_nextIndexs[nodeId]>1)
                    m_nextIndexs[nodeId]--;
                m_raft_mutex.unlock();
                return;
            }

            // 日志追加成功，则更新 m_nextIndex 和 m_matchIndex
            m_nextIndexs[nodeId]++;
            m_matchIndexs[nodeId] = m_nextIndexs[nodeId] - 1;

            uint64_t last_index = m_matchIndexs[nodeId];

            // 计算副本数目大于节点数量的一半才提交一个当前任期内的日志
            uint64_t vote = 0;
            for (size_t i = 0; i<m_nodes.size();i++) {
                std::cout<<"check commit with node"<<i<<std::endl;
                if(i == m_id){
                    ++vote;
                }
                else if (m_matchIndexs[i] >= last_index) {
                    ++vote;
                }
                // 假设存在 N 满足N > commitIndex，使得大多数的 matchIndex[i] ≥ N以及log[N].term == currentTerm 成立，
                // 则令 commitIndex = N（5.3 和 5.4 节）
                if (vote >= ((uint64_t)m_nodes.size() + 1) / 2) {
                    // 只有领导人当前任期里的日志条目可以被提交
                    //如果这条日志的大多数节点已经同步，那么尝试更新logger的commitindex（如果其原来的小的话），使得它可以被应用
                    m_raft_logger.setCommitIndex(std::max(last_index,m_raft_logger.getCommitIndex()));
                    logger->info(fmt::format("appendEntries from node{} :success{}, term{} can be commited",
                            nodeId,result.success,result.term)); 
                    break;
                    //被应用了之后，就可以反馈给客户端（epool查看applyindex）
                }
            }
            m_raft_mutex.unlock();
        }
    }

    
    RaftVoteRets RaftNode::raftVote(RaftVoteParams params){
        //std::cout<<"try get raftVote end"<<std::endl;
        m_raft_mutex.lock();
        //std::cout<<"try get raftVote end"<<std::endl;
        logger->info(fmt::format("raftvote params:term{}, candidateId{}, lastLogIndex{}, lastLogTerm{}",params.term,params.candidateId,params.lastLogIndex,params.lastLogTerm)); 
        RaftVoteRets rets;
        // 拒绝给任期小于自己的候选人投票
        if (params.term <= m_currentTerm ||(params.term == m_currentTerm && m_votedFor != params.candidateId )) {
            rets.term = m_currentTerm;
            rets.voteGranted = false;
            m_raft_mutex.unlock();
            return rets;
        }

        // 任期比自己大，变成追随者
        if (params.term > m_currentTerm) {
            // 成为follower停止心跳定时器
            if (m_heartbeatTimer && m_state == RaftState::Leader) {
                m_heartbeatTimer->stop();
                m_electionTimer->wake();
            }
            m_state = RaftState::Follower;
            m_currentTerm = params.term;
            m_votedFor = -1;
            m_leaderId = params.candidateId;
        }

        // 拒绝掉那些日志没有自己新的投票请求
        if (!m_raft_logger.is_newer(params.lastLogTerm, params.lastLogIndex)) {
            rets.term = m_currentTerm;
            rets.voteGranted = false;
            m_raft_mutex.unlock();
            return rets;
        }

        // 投票给候选人
        m_votedFor = params.candidateId;
        // 重置选举超时计时器
        m_electionTimer->resetTime(genRandom());

        // 返回给候选人的结果
        rets.term = m_currentTerm;
        rets.voteGranted = true;
        m_raft_mutex.unlock();
        return rets;
    }
    
    AppendEntriesRets RaftNode::appendEntries(AppendEntriesParams params){
        m_raft_mutex.lock();
        logger->info(fmt::format("appendEntries params:term{}, leaderId{}, prevLogIndex{}, prevLogTerm{}, op{}, leaderCommit{}",
                 params.term,params.leaderId,params.prevLogIndex,params.prevLogTerm,params.entry.op,params.leaderCommit)); 
        AppendEntriesRets rets;
        
        // 拒绝任期小于自己的 leader 的日志复制请求
        if (params.term < m_currentTerm) {
            rets.term = m_currentTerm;
            rets.success = false;
            m_raft_mutex.unlock();
            return rets;
        }

        // 对方任期大于自己或者自己为同一任期内败选的候选人则转变为 follower
        // 已经是该任期内的follower就不用变
        if (params.term > m_currentTerm ||
                    (params.term == m_currentTerm && m_state == RaftState::Candidate)) {
            // 成为follower停止心跳定时器
            if (m_heartbeatTimer && m_state == RaftState::Leader) {
                m_electionTimer->wake();
                m_heartbeatTimer->stop();
            }

            m_state = RaftState::Follower;
            m_currentTerm = params.term;
            m_votedFor = -1;
            m_leaderId = params.leaderId;
        }

        //在不知道当前领导的情况下（领导人收到更高term的ack或者候选人收到更高term的拒绝ack，变成追随者），得知了当前领导
        if (m_leaderId < 0) {
            m_leaderId = params.leaderId;
        }

        // 自己为同一任期内的follower，更新选举定时器就行
        m_electionTimer->resetTime(genRandom());

        //更新commiindex
        m_raft_logger.setCommitIndex(std::min(params.leaderCommit,m_raft_logger.getLastIndex()));

        //心跳，直接返回
        if(params.entry.op=="heartbeat"){
            rets.term = m_currentTerm;
            rets.success = true;
            m_raft_mutex.unlock();
            return rets;
        }
        // 拒绝错误的日志追加请求
        if (!m_raft_logger.appendEntry(params.entry,params.prevLogIndex,params.prevLogTerm)) {
            rets.term = m_currentTerm;
            rets.success = false;
            return rets;
        }

        rets.term = m_currentTerm;
        rets.success = true;
        m_raft_mutex.unlock();
        return rets;
    }

    void RaftNode::run(){
        registerFunc("raftVote",std::function<RaftVoteRets(RaftVoteParams)>(std::bind(&RaftNode::raftVote, this,std::placeholders::_1)));
        registerFunc("appendEntries",std::function<AppendEntriesRets(AppendEntriesParams)>(std::bind(&RaftNode::appendEntries, this,std::placeholders::_1)));

        const int MAX_EVENTS = 1024; // 最大事件数
        epoll_event events[MAX_EVENTS]; // 用于存储事件的数组

        while (true) { // 主循环
            int eventCount = epoll_wait(m_epollFD, events, MAX_EVENTS, -1); // 等待事件发生
            if (eventCount == -1) { // 检查 epoll_wait 是否成功
                std::cerr << "epoll_wait error: " << strerror(errno) << "\n"; // 打印错误信息
                break; // 退出循环
            }

            for (int i = 0; i < eventCount; ++i) { // 遍历发生的事件
                if (events[i].data.fd == m_listenSocket.getSocketFD()) { // 如果是监听套接字的事件
                    std::shared_ptr<Socket> newSocket(new Socket);
                    //Socket newSocket; // 创建新套接字
                    if (m_listenSocket.accept(*(newSocket.get()))) { // 接受新连接
                        //logger->info(fmt::format("new connection accepted"));  
                        newSocket->setNonBlocking(true); // 设置新套接字为非阻塞模式
                        epoll_event event; // 新的 epoll 事件
                        event.events = EPOLLIN; // 设置事件类型为可读
                        event.data.fd = newSocket->getSocketFD(); // 设置事件关联的文件描述符

                        if (epoll_ctl(m_epollFD, EPOLL_CTL_ADD, newSocket->getSocketFD(), &event) == -1) { // 将新套接字添加到 epoll 实例
                            std::cerr << "Failed to add new socket to epoll: " << strerror(errno) << "\n"; // 打印错误信息
                            newSocket->close(); // 关闭新套接字
                        } else {
                            std::cout << "New connection accepted: " << newSocket->getSocketFD() << "\n"; // 打印新连接信息
                            m_clientSockets[newSocket->getSocketFD()]=newSocket; // 将新套接字添加到客户端套接字列表中
                        }
                    }
                } else { // 如果是客户端套接字的事件
                    
                    handleMsg(events[i].data.fd);               
                    
                    
                    
                }
            }
        }
    }

    
    // 添加远程函数
    bool RaftNode::addFunc(const std::string& funcName, const std::vector<std::string>& params){
        m_funcs[funcName] = params;
        return true;
    }
    // 删除远程函数
    bool RaftNode::eraseFunc(const std::string& funcName){
        auto it = m_funcs.find(funcName);
        if (it != m_funcs.end()) {
            m_funcs.erase(it);
            return true;
        }
        return false;
    }
    // 添加远程函数地址
    bool RaftNode::addAddress(const std::string& funcName, ip_port address){
        m_addresses.emplace(funcName, address);
        return true;
    }

    // 删除远程函数地址
    bool RaftNode::eraseAddress(const std::string& funcName, ip_port address){
        auto range = m_addresses.equal_range(funcName);
        for (auto it = range.first; it != range.second; ++it) {
            if ((it->second) == address) {
            m_addresses.erase(it);
            return true;
        }
        }
        return false;
    }

    void RaftNode::handleMsg(int32_t sockfd){
        logger->info(fmt::format("new data from fd{}",int(sockfd)));  
        auto it = m_clientSockets.find(sockfd);
        if (it != m_clientSockets.end()) { // 如果找到客户端套接字
            Message receive_message;
            ssize_t bytesReceived = it->second->receive(receive_message); // 接收数据
            if (bytesReceived <= 0) { // 如果接收失败或连接关闭
                if (bytesReceived < 0) { // 如果接收失败
                    std::cerr << "Receive error: " << strerror(errno) << "\n"; // 打印错误信息
                } else { // 如果连接关闭
                    std::cout << "Connection closed: " << it->second->getSocketFD() << "\n"; // 打印连接关闭信息
                    epoll_ctl(m_epollFD, EPOLL_CTL_DEL, it->second->getSocketFD(), nullptr); // 从 epoll 实例中移除套接字
                    it->second->close(); // 关闭套接字
                    m_clientSockets.erase(it); // 从客户端列表中删除套接字
                }
            } else { // 如果接收成功
                receive_message.decode();
                std::cout << "new data accessed:" << receive_message.toString() << "\n"; 
                logger->info(fmt::format("new data accessed:{}",receive_message.toString()));
                //当前节点非leader节点，客户端重定向
                if(m_id!=m_leaderId){
                    switch(receive_message.type){
                        case Message::Type::RPC_SERVER_INIT:
                        case Message::Type::RPC_SERVICE_REGISTER:
                        case Message::Type::RPC_SERVICE_DELETE:
                            handleServerRedirect(receive_message,it->second);
                            return;
                        default:
                            break;
                    } 
                }
                switch(receive_message.type){
                    case Message::Type::RPC_CLIENT_INIT:
                        handleCLientInit(receive_message,it->second);
                        break;
                    case Message::Type::RPC_SERVER_INIT:
                        handleServerInit(receive_message,it->second);
                        break;
                    case Message::Type::RPC_SERVICE_REGISTER:
                        handleServiceRegister(receive_message,it->second);
                        break;
                    case Message::Type::HEART_BEAT:
                        handleHeartBeat(receive_message,it->second);
                        break;
                    case Message::Type::RPC_REQUEST:
                        handleCLientRequest(receive_message,it->second);
                        break;
                    default:break;
                }
                
                
            }
        }
    }

    void RaftNode::handleServerRedirect(Message &receive_message,std::shared_ptr<Socket> socket){
        logger->info(fmt::format("msg type:RAFT_REDIRECT"));
        Message message(C_RPC::Message::RAFT_REDIRECT);
        message.seq = receive_message.seq;
        Serializer s;
        //再写函数地址
        std::string ip = m_nodes[m_leaderId].m_ip;
        uint16_t port =  m_nodes[m_leaderId].m_port;
        s<<ip<<port;
        message.encode(s);
        socket->send(message);
    }

    void RaftNode::handleCLientInit(Message &receive_message,std::shared_ptr<Socket> socket){
        logger->info(fmt::format("msg type:RPC_CLIENT_INIT"));
        Message message(C_RPC::Message::RPC_SERVICE_DISCOVER_RESPONSE);
        message.seq = receive_message.seq;
        Serializer s;
        for(auto &i:m_addresses){
            //先写文函数名称
            std::string f_name = i.first;
            s<<f_name;
            //再写函数地址
            std::string f_ip = i.second.m_ip;
            s<<f_ip<<i.second.m_port;
        }
        message.encode(s);
        socket->send(message);
    }

    void RaftNode::handleServiceDiscover(Message &receive_message,std::shared_ptr<Socket> socket){
        logger->info(fmt::format("msg type:RPC_SERVICE_DISCOVER"));
        std::shared_ptr<Serializer> s =receive_message.getSerializer();
        std::string f_name;
        while(!s->isReadEnd()){
            (*s)>>f_name;
            std::cout<<"fname:"<<f_name<<std::endl;
            logger->info(fmt::format("RPC_SERVICE_DISCOVER:{}",f_name));
        }
        Message message(C_RPC::Message::RPC_SERVICE_DISCOVER_RESPONSE);
        message.seq = receive_message.seq;
        Serializer res;
        auto range = m_addresses.equal_range(f_name);
        for (auto it = range.first; it != range.second; ++it) {
            std::string f_name = it->first;
            res<<f_name;
            //再写函数地址
            std::string f_ip = it->second.m_ip;
            res<<f_ip<<it->second.m_port;
        }
        
        message.encode(res);
        socket->send(message);
    }

    void RaftNode::handleServerInit(Message &receive_message,std::shared_ptr<Socket> socket){
        //解析返回值
        logger->info(fmt::format("msg type:RPC_SERVER_INIT"));  
        std::shared_ptr<Serializer> s =receive_message.getSerializer();
        //保证leader先提交一条空日志
        m_raft_mutex.lock();
        while(m_raft_logger.getEntry(m_raft_logger.size()-1).op=="no-op"&&m_raft_logger.getCommitIndex()<(m_raft_logger.size()-1)&&m_state==RaftState::Leader){
            m_raft_mutex.unlock();
            m_raft_mutex.lock();
        }
        m_raft_mutex.unlock();
        m_raft_mutex.lock();
        uint64_t index;
        while(!s->isReadEnd()){
            std::string f_name,f_ip;
            uint16_t f_port;
            (*s)>>f_name>>f_ip>>f_port;
            std::cout<<"fname:"<<f_name<<" f_ip:"<<f_ip<<" f_port:"<<f_port<<std::endl;
            logger->info(fmt::format("RPC_SERVER_INIT:{},{},{}",f_name,f_ip,f_port));
            Entry entry;
            entry.term = m_currentTerm;
            entry.op = "register";
            entry.f_name = f_name;
            entry.f_ip = f_ip;
            entry.f_port = f_port;
            index = m_raft_logger.appendEntry(entry);
        }
        m_raft_mutex.unlock();
        //应用了后才能返回客户端
        m_raft_mutex.lock();
        while(index<m_raft_logger.getLastApplied()){
            m_raft_mutex.unlock();
            m_raft_mutex.lock();
        }
        m_raft_mutex.unlock();
        auto timer = Scheduler::GetTimer()->addTimeEvent(1500,[=](){
            m_heartbeat_timers.erase(socket->getSocketFD());
            std::cout << "Connection closed by heartbeat: " << socket->getSocketFD() << "\n"; // 打印连接关闭信息
            socket->close();
        },false);
        m_heartbeat_timers.emplace(socket->getSocketFD(),timer);
        Message message(C_RPC::Message::RPC_SERVICE_REGISTER_RESPONSE);
        message.seq = receive_message.seq;
        Serializer res;
        message.encode(res);
        socket->send(message);
    }

    void RaftNode::handleServiceRegister(Message &receive_message,std::shared_ptr<Socket> socket){
        //解析返回值
        logger->info(fmt::format("msg type:RPC_SERVICE_REGISTER"));  
        std::shared_ptr<Serializer> s =receive_message.getSerializer();
        //保证leader先提交一条空日志
        m_raft_mutex.lock();
        while(m_raft_logger.getEntry(m_raft_logger.size()-1).op=="no-op"&&m_raft_logger.getCommitIndex()<(m_raft_logger.size()-1)&&m_state==RaftState::Leader){
            m_raft_mutex.lock();
            m_raft_mutex.unlock();
        }
        m_raft_mutex.unlock();
        m_raft_mutex.lock();
        uint64_t index;
        while(!s->isReadEnd()){
            std::string f_name,f_ip;
            uint16_t f_port;
            (*s)>>f_name>>f_ip>>f_port;
            std::cout<<"fname:"<<f_name<<" f_ip:"<<f_ip<<" f_port:"<<f_port<<std::endl;
            logger->info(fmt::format("RPC_SERVICE_REGISTER:{},{},{}",f_name,f_ip,f_port));
            Entry entry;
            entry.term = m_currentTerm;
            entry.op = "register";
            entry.f_name = f_name;
            entry.f_ip = f_ip;
            entry.f_port = f_port;
            index = m_raft_logger.appendEntry(entry);
        }
        m_raft_mutex.unlock();
        //应用了后才能返回客户端
        m_raft_mutex.lock();
        while(index<m_raft_logger.getLastApplied()){
            m_raft_mutex.unlock();
            m_raft_mutex.lock();
        }
        m_raft_mutex.unlock();
        Message message(C_RPC::Message::RPC_SERVICE_REGISTER_RESPONSE);
        message.seq = receive_message.seq;
        Serializer res;
        message.encode(res);
        socket->send(message);
    }

    void RaftNode::handleServiceDelete(Message &receive_message,std::shared_ptr<Socket> socket){
        //解析返回值
        logger->info(fmt::format("msg type:RPC_SERVICE_DELETE"));  
        std::shared_ptr<Serializer> s =receive_message.getSerializer();
        
        while(!s->isReadEnd()){
            std::string f_name,f_ip;
            uint16_t f_port;
            (*s)>>f_name>>f_ip>>f_port;
            std::cout<<"fname:"<<f_name<<" f_ip:"<<f_ip<<" f_port:"<<f_port<<std::endl;\
            logger->info(fmt::format("RPC_SERVICE_DELETE:{},{},{}",f_name,f_ip,f_port));
            ip_port ip_port_(f_ip,f_port);
            auto range = m_addresses.equal_range(f_name);
            for (auto it = range.first; it != range.second; ++it) {
                if (it->second == ip_port_) {
                    m_addresses.erase(it);
                }
            }
        }
        Message message(C_RPC::Message::RPC_SERVICE_REGISTER_RESPONSE);
        message.seq = receive_message.seq;
        Serializer res;
        message.encode(res);
        socket->send(message);
    }

    void RaftNode::handleSubscribe(Message &receive_message,std::shared_ptr<Socket> socket){
        //解析返回值
        logger->info(fmt::format("msg type:RPC_SUBSCRIBE"));  
        std::shared_ptr<Serializer> s =receive_message.getSerializer();
        
        while(!s->isReadEnd()){
            std::string f_name;
            (*s)>>f_name;
            std::cout<<"subscribe fname:"<<f_name<<std::endl;
            logger->info(fmt::format("RPC_SUBSCRIBE:{}",f_name));
            
            m_subscribes.emplace(f_name,socket);
        }
        Message message(C_RPC::Message::RPC_SUBSCRIBE_RESPONSE);
        message.seq = receive_message.seq;
        Serializer res;
        message.encode(res);
        socket->send(message);
    }

    void RaftNode::publish(std::string f_name,std::string f_ip,uint16_t f_port ,bool is_add ){
        Message message(C_RPC::Message::RPC_SERVICE_DISCOVER_RESPONSE);
        Serializer res;
        res<<is_add<<f_name<<f_ip<<f_port; 
        message.encode(res);
        auto range = m_subscribes.equal_range(f_name);
        for (auto it = range.first; it != range.second; ++it) {
            //to do: socket.send函数也要加协程锁，不然会冲突
            it->second->send(message);
        }
    }

    void RaftNode::handleHeartBeat(Message &receive_message,std::shared_ptr<Socket> socket){
        //解析返回值
        logger->info(fmt::format("msg type:HEART_BEAT"));  
        std::shared_ptr<Serializer> s =receive_message.getSerializer();
        
        while(!s->isReadEnd()){
        }
        //reset定时器
        auto timer = m_heartbeat_timers[socket->getSocketFD()];
        if(timer!=nullptr){
            logger->info(fmt::format("HEART_BEAT"));
            timer->resetTime();
            return;
        }
        m_heartbeat_timers.erase(socket->getSocketFD());
    }

    
    void RaftNode::handleCLientRequest(Message &receive_message,std::shared_ptr<Socket> socket){
        //logger->info(fmt::format("msg type:RPC_REQUEST"));  
        std::shared_ptr<Serializer> s =receive_message.getSerializer();
            
        std::string f_name;
        (*s)>>f_name;
        //logger->info(fmt::format("call fname:{}",f_name));  
        Serializer returns;
        call(f_name,*s,returns);
        //logger->info(fmt::format("call fname:{} end",f_name)); 
        Message message(C_RPC::Message::RPC_RESPONSE);
        message.seq = receive_message.seq;
        message.encode(returns);
        socket->send(message);
    }

}

}