#pragma once

#include "types.hpp"
#include "gc/gc_object.hpp"
#include <string>

namespace Lua {

/**
 * @brief 表示 Lua 中的不可变字符串对象
 * 
 * String 类是一个可垃圾回收的对象，表示 Lua 字符串。
 * 它提供高效的存储和字符串操作，同时参与垃圾回收系统。
 */
class String : public GCObject {
public:
    // 从 C++ 字符串创建
    explicit String(const Str& value);
    
    // 析构函数
    ~String();
    
    // 获取字符串值
    const Str& getValue() const;
    
    // 获取字符串长度
    usize getLength() const;
    
    // 获取哈希值
    u32 getHash() const;
    
    // 字符串比较
    bool equals(const String* other) const;
    
    // 计算哈希值
    void computeHash();
    
    // GCObject 接口实现
    void mark(GarbageCollector* gc) override;
    GCObject::Type type() const override { return GCObject::Type::String; }
    
private:
    Str m_value;  // 实际的字符串内容
    u32 m_hash;   // 字符串的哈希值
};
} // namespace Lua
