#ifndef _SINGLETON_H_
#define _SINGLETON_H_

#include <memory>
namespace C_RPC{

template<typename T>
class Singleton{
public:
    static T* GetInstance(){
        static T instance;
        return &instance;
    }

};

template<typename T>
class SingletonPtr{
public:
    static std::shared_ptr<T> GetInstance(){
        static std::shared_ptr<T> instance(std::make_shared<T>());
        return instance;
    }
};

}
#endif
