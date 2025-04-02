#include "string.hpp"
#include "gc/gc_object.hpp"
#include "gc/gc_manager.hpp"
#include <algorithm>
#include <cstring>

namespace Lua {

String::String(const Str& value) 
    : GCObject(), m_value(value), m_hash(0) {
    // 计算字符串的哈希值
    computeHash();
}

String::~String() {
    // 字符串对象的析构函数
    // 无需特殊处理，m_value在对象销毁时自动清理
}

const Str& String::getValue() const {
    return m_value;
}

usize String::getLength() const {
    return m_value.length();
}

u32 String::getHash() const {
    return m_hash;
}

bool String::equals(const String* other) const {
    // 首先比较哈希值，这是快速检查
    if (m_hash != other->m_hash) {
        return false;
    }
    
    // 然后比较长度
    if (m_value.length() != other->m_value.length()) {
        return false;
    }
    
    // 最后比较内容
    return m_value == other->m_value;
}

void String::computeHash() {
    // 使用FNV-1a哈希算法
    const unsigned char* data = reinterpret_cast<const unsigned char*>(m_value.c_str());
    usize length = m_value.length();
    
    // FNV-1a参数
    constexpr u32 FNV_PRIME = 16777619;
    constexpr u32 FNV_OFFSET_BASIS = 2166136261;
    
    // 计算哈希值
    u32 hash = FNV_OFFSET_BASIS;
    for (usize i = 0; i < length; ++i) {
        hash ^= data[i];
        hash *= FNV_PRIME;
    }
    
    m_hash = hash;
}

void String::mark(GarbageCollector* gc) {
    if (isMarked()) {
        return;
    }
    
    GCObject::mark(gc);
}
} // namespace Lua
