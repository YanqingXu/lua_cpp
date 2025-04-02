#pragma once

#include "types.hpp"
#include "gc/gc_object.hpp"
#include "value.hpp"

namespace Lua {
/**
 * @brief Implements the Lua table data structure
 * 
 * The Table class represents Lua tables, which are the primary data structure in Lua.
 * It supports both array and hash parts for efficient storage of sequential and
 * associative data. Tables are garbage-collectible objects.
 */
class Table : public GCObject {
public:
    // 表项结构，用于存储键值对
    struct Entry {
        Value key;
        Value value;
    };
    
    // 创建表对象，可指定初始数组部分和哈希部分的大小
    Table(i32 narray = 0, i32 nrec = 0);
    
    // 析构函数
    ~Table();
    
    // 通过键获取值
    Value get(const Value& key) const;
    
    // 设置键值对
    void set(const Value& key, const Value& value);
    
    // 获取数组部分长度（最后一个非nil元素的索引）
    i32 getLength() const;
    
    // 检查表是否包含指定键
    bool contains(const Value& key) const;
    
    // 直接访问数组部分（用于优化）
    const Vec<Value>& getArrayPart() const { return m_array; }
    
    // 获取表中所有键值对
    Vec<Entry> getEntries() const;
    
    // 获取和设置元表
    Ptr<Table> getMetatable() const { return m_metatable; }
    void setMetatable(Ptr<Table> metatable) { m_metatable = metatable; }
    
    // 直接通过整数索引访问（用于优化）
    Value rawGetI(i32 index) const;
    void rawSetI(i32 index, const Value& value);
    
    // 调整表大小
    void resize(i32 narray);
    
    // 迭代支持
    bool next(Value& key, Value& value);
    
    // GCObject接口实现
    Type type() const override { return Type::Table; }
    void mark(GarbageCollector* gc) override;
    
private:
    // 数组部分，存储连续整数索引的值（Lua中从1开始，实现中从0开始）
    Vec<Value> m_array;
    
    // 哈希部分，存储非连续整数索引和其他类型的键
    HashMap<Value, Value> m_hash;
    
    // 元表（可选）
    Ptr<Table> m_metatable;
    
    // 辅助方法
    bool isArrayIndex(const Value& key) const;
    i32 getArrayIndex(const Value& key) const;
};
} // namespace Lua
