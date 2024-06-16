#ifndef _RPC_PROTOCOL_H_
#define _RPC_PROTOCOL_H_

#include <atomic>
#include <memory>
#include  "util/serializer.h"
namespace C_RPC{
//msg序列号
static std::atomic<uint64_t> SEQ{0};
/**
  * @brief msg协议
  */
class Message{
public:
/**
  * @brief 消息类型
  */
enum Type :uint8_t{
    //for client
    RPC_REQUEST,//RPC请求
    RPC_CLIENT_INIT,//用户初始化
    RPC_SERVICE_DISCOVER,//服务发现
    RPC_SUBSCRIBE,//服务订阅
    //for server
    RPC_RESPONSE,//RPC响应
    RPC_SERVER_INIT,//服务器初始化
    RPC_SERVICE_DELETE,//服务删除
    RPC_SERVICE_REGISTER,//服务注册
    //for register
    RPC_SERVICE_REGISTER_RESPONSE,//服务注册响应
    RPC_SERVICE_DISCOVER_RESPONSE,//服务注册响应
    RPC_SUBSCRIBE_RESPONSE,//服务订阅响应
    RPC_PUBLISH,//服务发布


    HEART_BEAT,//心跳
    RAFT_REDIRECT,//注册中心重定向
    RPC_DEFAULT,
};

/**
  * @brief msg协议
  */
enum ErrCode :uint8_t{
    SUCCESS,
    ERROR_CALLNAME,
    ERROR_CALLPARAMS,
    ERRCODE_DEFAULT,
};

Message(uint8_t type = Type::RPC_REQUEST,uint8_t errCode = ErrCode::SUCCESS,uint8_t version = 0x01)
    :serializerPtr(new Serializer),version(version),type(type),errCode(errCode),seq(++SEQ){}

void encode(Serializer &s){
    (*serializerPtr)<<magic<<version<<type<<errCode<<seq;
    contentLenth=s.size();
    std::string temp = s.toString();
    (*serializerPtr)<<temp;
}

void decode(){
    (*serializerPtr)>>magic>>version>>type>>errCode>>seq>>contentLenth;
}

std::string toString() const{
    return serializerPtr->toString();
}

std::shared_ptr<Serializer> getSerializer(){
    return serializerPtr;
}
public:
    uint8_t magic{0x66};
    uint8_t version;
    uint8_t type;
    uint8_t errCode;
    uint64_t seq;
    uint64_t contentLenth;
private:
    std::shared_ptr<Serializer> serializerPtr;
};

}


#endif