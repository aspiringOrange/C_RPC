// // rpc_server.h


// #include <unordered_map>
// #include <functional>
// #include <string>
// #include "util/serializer.h"

// class RpcRegister {
// public:
//     template<typename Ret, typename... Args>
//     void registerFunc(const std::string& name, Ret(*func)(Args...)) {
//         //std::cout<<std::index_sequence_for<Args...><<std::endl;//生成[0,1,2,3,...,n]
//         m_funcs[name] = [func, name](const int& params, int& returns) {
//             callFunc(func, params, returns, std::index_sequence_for<Args...>{});
//         };
//     }

//     void call(const std::string& name, const int& params, int& returns) {
//         if (m_funcs.find(name) != m_funcs.end()) {
//             m_funcs[name](params, returns);
//         } else {
//             throw std::runtime_error("Function not found");
//         }
//     }

// private:
//     std::unordered_map<std::string, std::function<void(const int&, int&)>> m_funcs;

//     template<typename Ret, typename Func, typename Tuple, std::size_t... I>
//     void callFuncImpl(Func func, const Tuple& t, Ret& result, std::index_sequence<I...>) {
//         if constexpr (std::is_same_v<Ret, void>) {
//             func(std::get<I>(t)...);
//         } else {
//             result = func(std::get<I>(t)...);
//         }
//     }

//     template<typename Ret, typename... Args, std::size_t... I>
//     void callFunc(Ret(*func)(Args...), const int& params, int& returns, std::index_sequence<I...>) {
//         std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...> args;
//         unpack(params, args, std::index_sequence_for<Args...>{});
//         callFuncImpl(func, args, result, std::index_sequence_for<Args...>{});
//         if constexpr (!std::is_same_v<Ret, void>) {
//             returns << result;
//         }
//     }

//     template<typename Tuple, std::size_t... I>
//     void unpack(const int& params, Tuple& t, std::index_sequence<I...>) {
//         (params >> ... >> std::get<I>(t));//t[I[0]]=params[0],t[I[1]]=params[1],...
//     }



// };
#include "rpc/rpc_server.h"
int main(){
    //RpcRegister a;
    return 0;
}