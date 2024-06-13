#include "util/serializer.h"
#include <iostream>
void test(){
    C_RPC::Serializer s;
    //test bool
    bool flag = true;
    uint8_t u8= 111;
    int8_t i8= -1;
    uint16_t u16= 333;
    int16_t i16= -444;
    uint32_t u32= 555;
    int32_t i32= -666;
    uint64_t u64= 777;
    int64_t i64= -888;
    float f = 5.67;
    double d = -1222.34;
    std::string str("string");
    std::set<int> set{123,234,345};
    std::vector<int> vec{123,234,345};
    std::map<int,int> map;map[321] = 654; map[999]=2333;
    s<<flag<<u8<<i8<<u16<<i16<<u32<<i32<<u64<<i64<<f<<d<<str<<set<<vec<<map<<"end";
    s.reset();
    bool tflag ;
    uint8_t tu8;
    int8_t ti8;
    uint16_t tu16;
    int16_t ti16;
    uint32_t tu32;
    int32_t ti32;
    uint64_t tu64;
    int64_t ti64;
    float tf ;
    double td;
    std::string tstr1,tstr2;
    std::set<int> tset;
    std::vector<int> tvec;
    std::map<int,int> tmap;
    s>>tflag>>tu8>>ti8>>tu16>>ti16>>tu32>>ti32>>tu64>>ti64>>tf>>td>>tstr1>>tset>>tvec>>tmap>>tstr2;
    std::cout<<s.toString()<<std::endl;
    std::cout<<tflag<<std::endl;
    std::cout<<int(tu8)<<std::endl;
    std::cout<<int(ti8)<<std::endl;
    std::cout<<tu16<<std::endl;
    std::cout<<ti16<<std::endl;
    std::cout<<tu32<<std::endl;
    std::cout<<ti32<<std::endl;
    std::cout<<tu64<<std::endl;
    std::cout<<ti64<<std::endl;
    std::cout<<tf<<std::endl;
    std::cout<<td<<std::endl;
    std::cout<<tstr1<<std::endl;
    for(auto x : tset)
        std::cout<<x<<std::endl;
    for(auto x : tvec)
        std::cout<<x<<std::endl;
    for(auto x : tmap)
        std::cout<<x.first<<"->"<<x.second<<std::endl;
    std::cout<<tstr2<<std::endl;
}
        

int main(){
    test();
    return 0;
}