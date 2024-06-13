#ifndef _SERIALIZER_H_
#define _SERIALIZER_H_

#include <iostream>
#include <memory>
#include <list>
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <tuple>
#include <string>
#include <utility>
#include <vector>
#include <arpa/inet.h>
#include <cstring>
#include "raft/raft_config.h"
namespace C_RPC {


class Buffer {
public:
    Buffer() : m_position(0) {}

    void write(const char* data, size_t len) {
        m_data.insert(m_data.end(), data, data + len);
    }

    void writeInt8(int8_t value) {
        m_data.push_back(static_cast<char>(value));
    }

    void writeUint8(uint8_t value) {
        m_data.push_back(static_cast<char>(value));
    }

    void writeInt16(int16_t value) {
        value = htons(value);
        write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    void writeUint16(uint16_t value) {
        value = htons(value);
        write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    void writeInt32(int32_t value) {
        value = htonl(value);
        write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    void writeUint32(uint32_t value) {
        value = htonl(value);
        write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    void writeInt64(int64_t value) {
        value = htonll(value);
        write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    void writeUint64(uint64_t value) {
        value = htonll(value);
        write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    void writeFloat(float value) {
        write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    void writeDouble(double value) {
        write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    void read(char* data, size_t len) {
        checkRead(len);
        std::memcpy(data, m_data.data() + m_position, len);
        m_position += len;
    }

    int8_t readInt8() {
        checkRead(sizeof(int8_t));
        return static_cast<int8_t>(m_data[m_position++]);
    }

    uint8_t readUint8() {
        checkRead(sizeof(uint8_t));
        return static_cast<uint8_t>(m_data[m_position++]);
    }

    int16_t readInt16() {
        int16_t value;
        read(reinterpret_cast<char*>(&value), sizeof(value));
        return ntohs(value);
    }

    uint16_t readUint16() {
        uint16_t value;
        read(reinterpret_cast<char*>(&value), sizeof(value));
        return ntohs(value);
    }

    int32_t readInt32() {
        int32_t value;
        read(reinterpret_cast<char*>(&value), sizeof(value));
        return ntohl(value);
    }

    uint32_t readUint32() {
        uint32_t value;
        read(reinterpret_cast<char*>(&value), sizeof(value));
        return ntohl(value);
    }

    int64_t readInt64() {
        int64_t value;
        read(reinterpret_cast<char*>(&value), sizeof(value));
        return ntohll(value);
    }

    uint64_t readUint64() {
        uint64_t value;
        read(reinterpret_cast<char*>(&value), sizeof(value));
        return ntohll(value);
    }

    float readFloat() {
        float value;
        read(reinterpret_cast<char*>(&value), sizeof(value));
        return value;
    }

    double readDouble() {
        double value;
        read(reinterpret_cast<char*>(&value), sizeof(value));
        return value;
    }

    
    void setPosition(size_t position) {
        if (position > m_data.size()) {
            throw std::out_of_range("Position out of range");
        }
        m_position = position;
    }

    size_t getPosition() const {
        return m_position;
    }

    size_t getSize() const {
        return m_data.size();
    }

    void clear() {
        m_data.clear();
        m_position = 0;
    }

    std::string toString() const {
        return std::string(m_data.begin(), m_data.end());
    }

    bool isReadEnd(){
        return m_position>=getSize();
    }

private:
    void checkRead(size_t len) {
        if (m_position + len > m_data.size()) {
            throw std::out_of_range("Read out of range");
        }
    }

    uint64_t htonll(uint64_t value) {
        if (__BYTE_ORDER == __LITTLE_ENDIAN) {
            return ((uint64_t)htonl(value & 0xFFFFFFFF) << 32) | htonl(value >> 32);
        } else if (__BYTE_ORDER == __BIG_ENDIAN) {
            return value;
        }
    }

    uint64_t ntohll(uint64_t value) {
        if (__BYTE_ORDER == __LITTLE_ENDIAN) {
            return ((uint64_t)ntohl(value & 0xFFFFFFFF) << 32) | ntohl(value >> 32);
        } else if (__BYTE_ORDER == __BIG_ENDIAN) {
            return value;
        }
    }

    std::vector<char> m_data;
    size_t m_position;
};

// 前向声明友元函数
// template<typename... Types>
// class Serializer;

// template<typename... Types>
// Serializer& operator<<(Serializer& serializer, const std::tuple<Types...>& t);

// template<typename... Types>
// Serializer& operator>>(Serializer& serializer, std::tuple<Types...>& t);
template<std::size_t Index, typename... Types>
struct TupleSerializer;

class Serializer {
public:
    // template<typename... Types>
    // friend Serializer& operator<<(Serializer& serializer, const std::tuple<Types...>& t);

    // template<typename... Types>
    // friend Serializer& operator>>(Serializer& serializer, std::tuple<Types...>& t);

    void write(const char* data, size_t len) {
        m_buffer.write(data, len);
    }

    void read(char* data, size_t len) {
        m_buffer.read(data,len);
    }


    size_t size() const {
        return m_buffer.getSize();
    }

    void reset() {
        m_buffer.setPosition(0);
    }

    bool isReadEnd(){
        return m_buffer.isReadEnd();
    }

    void offset(size_t off) {
        size_t old = m_buffer.getPosition();
        m_buffer.setPosition(old + off);
    }

    std::string toString() const {
        return m_buffer.toString();
    }


    void clear() {
        m_buffer.clear();
    }

    template<typename T>
    void read(T& t) {
        if constexpr (std::is_same_v<T, bool>) {
            t = m_buffer.readInt8();
        } else if constexpr (std::is_same_v<T, float>) {
            t = m_buffer.readFloat();
        } else if constexpr (std::is_same_v<T, double>) {
            t = m_buffer.readDouble();
        } else if constexpr (std::is_same_v<T, int8_t>) {
            t = m_buffer.readInt8();
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            t = m_buffer.readUint8();
        } else if constexpr (std::is_same_v<T, int16_t>) {
            t = m_buffer.readInt16();
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            t = m_buffer.readUint16();
        } else if constexpr (std::is_same_v<T, int32_t>) {
            t = m_buffer.readInt32();
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            t = m_buffer.readUint32();
        } else if constexpr (std::is_same_v<T, int64_t>) {
            t = m_buffer.readInt64();
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            t = m_buffer.readUint64();
        }
    }

    template<typename T>
    void write(const T& t) {
        if constexpr (std::is_same_v<T, bool>) {
            m_buffer.writeInt8(t);
        } else if constexpr (std::is_same_v<T, float>) {
            m_buffer.writeFloat(t);
        } else if constexpr (std::is_same_v<T, double>) {
            m_buffer.writeDouble(t);
        } else if constexpr (std::is_same_v<T, int8_t>) {
            m_buffer.writeInt8(t);
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            m_buffer.writeUint8(t);
        } else if constexpr (std::is_same_v<T, int16_t>) {
            m_buffer.writeInt16(t);
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            m_buffer.writeUint16(t);
        } else if constexpr (std::is_same_v<T, int32_t>) {
            m_buffer.writeInt32(t);
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            m_buffer.writeUint32(t);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            m_buffer.writeInt64(t);
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            m_buffer.writeUint64(t);
        }
    }

    template<typename T>
    Serializer& operator>>(T& t) {
        read(t);
        return *this;
    }

    template<typename T>
    Serializer& operator<<(const T& t) {
        write(t);
        return *this;
    }

    Serializer& operator>>(std::string& v) {
        size_t size;
        read(size);
        v.resize(size);
        m_buffer.read(&v[0], size);
        return *this;
    }
    
    Serializer& operator<<(std::string& v) {
        size_t len =v.size();
        write(len);
        m_buffer.write(v.data(), len);
        return *this;
    }

    Serializer& operator<<(char *v) {
        std::string str(v);
        (*this)<<str;
        return *this;
    }

    Serializer& operator<<(const char *v) {
        std::string str(v);
        (*this)<<str;
        return *this;
    }


    template<typename T>
    Serializer& operator>>(std::list<T>& v) {
        size_t size;
        read(size);
        for (size_t i = 0; i < size; ++i) {
            T t;
            read(t);
            v.emplace_back(t);
        }
        return *this;
    }

    template<typename T>
    Serializer& operator<<(const std::list<T>& v) {
        size_t size = v.size();
        write(size);
        for (const auto& t : v) {
            (*this) << t;
        }
        return *this;
    }

    template<typename T>
    Serializer& operator>>(std::vector<T>& v) {
        size_t size;
        read(size);
        for (size_t i = 0; i < size; ++i) {
            T t;
            read(t);
            v.emplace_back(t);
        }
        return *this;
    }

    template<typename T>
    Serializer& operator<<(const std::vector<T>& v) {
        size_t size = v.size();
        write(size);
        for (const auto& t : v) {
            (*this) << t;
        }
        return *this;
    }

    template<typename T>
    Serializer& operator>>(std::set<T>& v) {
        size_t size;
        read(size);
        for (size_t i = 0; i < size; ++i) {
            T t;
            read(t);
            v.emplace(t);
        }
        return *this;
    }

    template<typename T>
    Serializer& operator<<(const std::set<T>& v) {
        size_t size = v.size();
        write(size);
        for (const auto& t : v) {
            (*this) << t;
        }
        return *this;
    }

    template<typename T>
    Serializer& operator>>(std::multiset<T>& v) {
        size_t size;
        read(size);
        for (size_t i = 0; i < size; ++i) {
            T t;
            read(t);
            v.emplace(t);
        }
        return *this;
    }

    template<typename T>
    Serializer& operator<<(const std::multiset<T>& v) {
        size_t size = v.size();
        write(size);
        for (const auto& t : v) {
            (*this) << t;
        }
        return *this;
    }

    template<typename T>
    Serializer& operator>>(std::unordered_set<T>& v) {
        size_t size;
        read(size);
        for (size_t i = 0; i < size; ++i) {
            T t;
            read(t);
            v.emplace(t);
        }
        return *this;
    }

    template<typename T>
    Serializer& operator<<(const std::unordered_set<T>& v) {
        size_t size = v.size();
        write(size);
        for (const auto& t : v) {
            (*this) << t;
        }
        return *this;
    }

    template<typename T>
    Serializer& operator>>(std::unordered_multiset<T>& v) {
        size_t size;
        read(size);
        for (size_t i = 0; i < size; ++i) {
            T t;
            read(t);
            v.emplace(t);
        }
        return *this;
    }

    template<typename T>
    Serializer& operator<<(const std::unordered_multiset<T>& v) {
        size_t size = v.size();
        write(size);
        for (const auto& t : v) {
            (*this) << t;
        }
        return *this;
    }

    template<typename K, typename V>
    Serializer& operator>>(std::pair<K, V>& p) {
        (*this) >> p.first >> p.second;
        return *this;
    }

    template<typename K, typename V>
    Serializer& operator<<(const std::pair<K, V>& p) {
        (*this) << p.first << p.second;
        return *this;
    }

    template<typename K, typename V>
    Serializer& operator>>(std::map<K, V>& m) {
        size_t size;
        read(size);
        for (size_t i = 0; i < size; ++i) {
            std::pair<K, V> p;
            (*this) >> p;
            m.emplace(p);
        }
        return *this;
    }

    template<typename K, typename V>
    Serializer& operator<<(const std::map<K, V>& m) {
        size_t size = m.size();
        write(size);
        for (const auto& t : m) {
            (*this) << t;
        }
        return *this;
    }

    template<typename K, typename V>
    Serializer& operator>>(std::unordered_map<K, V>& m) {
        size_t size;
        read(size);
        for (size_t i = 0; i < size; ++i) {
            std::pair<K, V> p;
            (*this) >> p;
            m.emplace(p);
        }
        return *this;
    }

    template<typename K, typename V>
    Serializer& operator<<(const std::unordered_map<K, V>& m) {
        size_t size = m.size();
        write(size);
        for (const auto& t : m) {
            (*this) << t;
        }
        return *this;
    }

    template<typename K, typename V>
    Serializer& operator>>(std::multimap<K, V>& m) {
        size_t size;
        read(size);
        for (size_t i = 0; i < size; ++i) {
            std::pair<K, V> p;
            (*this) >> p;
            m.emplace(p);
        }
        return *this;
    }

    template<typename K, typename V>
    Serializer& operator<<(const std::multimap<K, V>& m) {
        size_t size = m.size();
        write(size);
        for (const auto& t : m) {
            (*this) << t;
        }
        return *this;
    }

    template<typename K, typename V>
    Serializer& operator>>(std::unordered_multimap<K, V>& m) {
        size_t size;
        read(size);
        for (size_t i = 0; i < size; ++i) {
            std::pair<K, V> p;
            (*this) >> p;
            m.emplace(p);
        }
        return *this;
    }

    template<typename K, typename V>
    Serializer& operator<<(const std::unordered_multimap<K, V>& m) {
        size_t size = m.size();
        write(size);
        for (const auto& t : m) {
            (*this) << t;
        }
        return *this;
    }

    
    template<typename... Types>
    Serializer& operator>>( std::tuple<Types...>& t) {
        TupleSerializer<0, Types...>::deserialize(*this, t);
        return *this;
    }

    template<typename... Types>
    Serializer& operator<<(const std::tuple<Types...>& t) {
        TupleSerializer<0, Types...>::serialize(*this, t);
        return *this;
    }
    
    Serializer& operator>>( Raft::RaftVoteParams& t) {
        (*this)>>t.term>>t.candidateId>>t.lastLogIndex>>t.lastLogTerm;
        return *this;
    }

    Serializer& operator<<( Raft::RaftVoteParams& t) {
        (*this)<<t.term<<t.candidateId<<t.lastLogIndex<<t.lastLogTerm;
        return *this;
    }

    Serializer& operator>>( Raft::RaftVoteRets& t) {
        (*this)>>t.term>>t.voteGranted;
        return *this;
    }

    Serializer& operator<<( Raft::RaftVoteRets& t) {
        (*this)<<t.term<<t.voteGranted;
        return *this;
    }

    Serializer& operator>>( Raft::Entry& t) {
        (*this)>>t.index>>t.term>>t.op>>t.f_name>>t.f_ip>>t.f_port;
        return *this;
    }

    Serializer& operator<<( Raft::Entry& t) {
        (*this)<<t.index<<t.term<<t.op<<t.f_name<<t.f_ip<<t.f_port;
        return *this;
    }

    Serializer& operator>>( Raft::AppendEntriesParams& t) {
        (*this)>>t.term>>t.leaderId>>t.prevLogIndex>>t.prevLogTerm>>t.entry>>t.leaderCommit;
        return *this;
    }

    Serializer& operator<<( Raft::AppendEntriesParams& t) {
        (*this)<<t.term<<t.leaderId<<t.prevLogIndex<<t.prevLogTerm<<t.entry<<t.leaderCommit;
        return *this;
    }

    Serializer& operator>>( Raft::AppendEntriesRets& t) {
        (*this)>>t.success>>t.term;
        return *this;
    }

    Serializer& operator<<( Raft::AppendEntriesRets& t) {
        (*this)<<t.success<<t.term;
        return *this;
    }

private:
    Buffer m_buffer;
};

// TupleSerializer 辅助模板结构
template<std::size_t Index, typename... Types>
struct TupleSerializer {
    static void serialize(Serializer& serializer, const std::tuple<Types...>& t) {
        if constexpr (Index < sizeof...(Types)) {
            serializer << std::get<Index>(t);
            TupleSerializer<Index + 1, Types...>::serialize(serializer, t);
        }
    }

    static void deserialize(Serializer& serializer, std::tuple<Types...>& t) {
        if constexpr (Index < sizeof...(Types)) {
            serializer >> std::get<Index>(t);
            TupleSerializer<Index + 1, Types...>::deserialize(serializer, t);
        }
    }
};


}
#endif 
