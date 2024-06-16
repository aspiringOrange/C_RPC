#ifndef _SINGLETON_H_
#define _SINGLETON_H_

#include <memory>
namespace C_RPC{
/**
  * @brief 单例模式
  */
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

}// namespace C_RPC
#endif //_SINGLETON_H_
