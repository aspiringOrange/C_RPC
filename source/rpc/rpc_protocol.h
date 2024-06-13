#ifndef _RPC_PROTOCOL_H_
#define _RPC_PROTOCOL_H_

#include <atomic>
#include <memory>
#include  "util/serializer.h"
namespace C_RPC{

static std::atomic<uint64_t> SEQ{0};
class Message{
public:
enum Type :uint8_t{
    //for client
    RPC_REQUEST,
    RPC_CLIENT_INIT,
    RPC_SERVICE_DISCOVER,
    RPC_SUBSCRIBE,
    //for server
    RPC_RESPONSE,
    RPC_SERVER_INIT,
    RPC_SERVICE_DELETE,
    RPC_SERVICE_REGISTER,
    //for register

    RPC_SERVICE_REGISTER_RESPONSE,
    RPC_SERVICE_DISCOVER_RESPONSE,
    RPC_SUBSCRIBE_RESPONSE,
    RPC_PUBLISH,


    HEART_BEAT,
    RAFT_REDIRECT,
    RPC_DEFAULT,
};

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