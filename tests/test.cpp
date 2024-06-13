#include <typeindex>
//#include <typeinfo>
#include <iostream>
#include <string>
#include <vector>
class A{
    int x;
};

struct S{
    char x;
};

int main(){
    //std::cout<<typeid(int)<<std::endl;
    std::cout<<typeid(123).name()<<std::endl;
    std::cout<<typeid(int32_t).name()<<std::endl;
    std::cout<<typeid(uint8_t).name()<<std::endl;
    std::cout<<typeid(int8_t).name()<<std::endl;
    std::cout<<typeid(true).name()<<std::endl;
    std::cout<<typeid(char).name()<<std::endl;
    std::cout<<typeid(std::string).name()<<std::endl;
    std::cout<<typeid(std::vector<int>).name()<<std::endl;
    std::vector<int> x{1,2,3};
    std::cout<<typeid(x).name()<<std::endl;
    std::cout<<typeid(std::vector<std::vector<int>>).name()<<std::endl;
    std::cout<<typeid(A).name()<<std::endl;
    std::cout<<typeid(S).name()<<std::endl;
    std::cout<<typeid(std::vector<A>).name()<<std::endl;
    std::cout<<typeid(std::vector<S>).name()<<std::endl;
    std::cout<<typeid(typeid(int)).name()<<std::endl;
    std::cout<<typeid(std::type_info).name()<<std::endl;

}