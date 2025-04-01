#include "table.hpp"
#include "value.hpp"
#include <algorithm>

namespace Lua {
namespace Object {

Table::Table(i32 narray, i32 nrec) {
    // 预分配数组部分的空间
    if (narray > 0) {
        m_array.resize(narray);
    }
    
    // 预留哈希表空间
    if (nrec > 0) {
        m_hash.reserve(nrec);
    }
}

Table::~Table() {
    // 清理table中的所有值
    m_array.clear();
    m_hash.clear();
}

Value Table::get(const Value& key) const {
    // nil键不能存在于表中
    if (key.isNil()) {
        return Value::nil();
    }
    
    // 对于整数键，检查数组部分
    if (key.isNumber()) {
        double numKey = key.asNumber();
        i32 intKey = static_cast<i32>(numKey);
        
        // 检查是否是整数且在数组范围内
        if (static_cast<double>(intKey) == numKey && intKey > 0 && intKey <= static_cast<i32>(m_array.size())) {
            return m_array[intKey - 1]; // Lua的数组索引从1开始
        }
    }
    
    // 查找哈希表部分
    auto it = m_hash.find(key);
    if (it != m_hash.end()) {
        return it->second;
    }
    
    // 键不存在，返回nil
    return Value::nil();
}

void Table::set(const Value& key, const Value& value) {
    // nil键不能存在于表中
    if (key.isNil()) {
        return;
    }
    
    // 对于整数键，可能存储在数组部分
    if (key.isNumber()) {
        double numKey = key.asNumber();
        i32 intKey = static_cast<i32>(numKey);
        
        // 检查是否是整数且适合存储在数组部分
        if (static_cast<double>(intKey) == numKey && intKey > 0) {
            // 扩展数组如果需要
            if (intKey > static_cast<i32>(m_array.size())) {
                // 如果值是nil，无需扩展数组
                if (value.isNil()) {
                    // 但要检查是否需要从哈希表中删除
                    m_hash.erase(key);
                    return;
                }
                
                // 仅当要设置的键相对当前数组长度不太大时，才扩展数组
                if (intKey <= static_cast<i32>(m_array.size()) * 2) {
                    m_array.resize(intKey);
                    for (usize i = m_array.size() - 1; i < static_cast<usize>(intKey) - 1; ++i) {
                        m_array[i] = Value::nil();
                    }
                } else {
                    // 否则存入哈希表部分
                    if (value.isNil()) {
                        m_hash.erase(key);
                    } else {
                        m_hash[key] = value;
                    }
                    return;
                }
            }
            
            // 设置数组元素
            if (value.isNil()) {
                // 如果值为nil且在数组范围内，则设置为nil
                if (intKey <= static_cast<i32>(m_array.size())) {
                    m_array[intKey - 1] = Value::nil();
                }
                // 确保从哈希表中删除
                m_hash.erase(key);
            } else {
                m_array[intKey - 1] = value;
            }
            return;
        }
    }
    
    // 对于非整数键或数组范围外的整数键，使用哈希表部分
    if (value.isNil()) {
        // 如果值为nil，则删除该键
        m_hash.erase(key);
    } else {
        // 否则设置或更新该键的值
        m_hash[key] = value;
    }
}

Value Table::rawGetI(i32 index) const {
    // 检查索引是否在数组范围内
    if (index > 0 && index <= static_cast<i32>(m_array.size())) {
        return m_array[index - 1];
    }
    
    // 否则尝试从哈希表获取
    return get(Value::number(static_cast<double>(index)));
}

void Table::rawSetI(i32 index, const Value& value) {
    set(Value::number(static_cast<double>(index)), value);
}

i32 Table::getLength() const {
    // 计算数组部分的"长度"
    // Lua表的长度是最后一个非nil元素的索引
    i32 len = 0;
    
    // 从数组的末尾向前搜索第一个非nil元素
    for (i32 i = static_cast<i32>(m_array.size()); i > 0; --i) {
        if (!m_array[i - 1].isNil()) {
            len = i;
            break;
        }
    }
    
    // 检查哈希表部分是否有整数键
    for (const auto& pair : m_hash) {
        if (pair.first.isNumber()) {
            double numKey = pair.first.asNumber();
            i32 intKey = static_cast<i32>(numKey);
            
            if (static_cast<double>(intKey) == numKey && intKey > 0 && !pair.second.isNil()) {
                len = std::max(len, intKey);
            }
        }
    }
    
    return len;
}

bool Table::contains(const Value& key) const {
    // 检查nil键
    if (key.isNil()) {
        return false;  // nil键不能存在于表中
    }
    
    // 检查是否是数组索引
    if (key.isNumber()) {
        double numKey = key.asNumber();
        i32 intKey = static_cast<i32>(numKey);
        
        if (static_cast<double>(intKey) == numKey && intKey > 0 && 
            intKey <= static_cast<i32>(m_array.size()) && !m_array[intKey - 1].isNil()) {
            return true;
        }
    }
    
    // 检查哈希表部分
    return m_hash.find(key) != m_hash.end();
}

void Table::resize(i32 narray) {
    if (narray <= 0) {
        m_array.clear();
        return;
    }
    
    // 调整数组部分大小
    i32 oldSize = static_cast<i32>(m_array.size());
    m_array.resize(narray);
    
    // 填充新元素为nil
    for (i32 i = oldSize; i < narray; ++i) {
        m_array[i] = Value::nil();
    }
}

bool Table::next(Value& key, Value& value) {
    // 首先检查数组部分
    if (key.isNil()) {
        // 从数组开始
        for (usize i = 0; i < m_array.size(); ++i) {
            if (!m_array[i].isNil()) {
                key = Value::number(static_cast<double>(i + 1));
                value = m_array[i];
                return true;
            }
        }
        
        // 数组部分为空，检查哈希表部分
        if (!m_hash.empty()) {
            auto it = m_hash.begin();
            key = it->first;
            value = it->second;
            return true;
        }
        
        // 表为空
        return false;
    }
    
    // 如果上一个键在数组部分
    if (key.isNumber()) {
        double numKey = key.asNumber();
        i32 intKey = static_cast<i32>(numKey);
        
        if (static_cast<double>(intKey) == numKey && intKey > 0 && 
            intKey <= static_cast<i32>(m_array.size())) {
            // 查找下一个非nil数组元素
            for (i32 i = intKey; i < static_cast<i32>(m_array.size()); ++i) {
                if (!m_array[i].isNil()) {
                    key = Value::number(static_cast<double>(i + 1));
                    value = m_array[i];
                    return true;
                }
            }
            
            // 数组剩余部分为空，转到哈希表
            if (!m_hash.empty()) {
                auto it = m_hash.begin();
                key = it->first;
                value = it->second;
                return true;
            }
            
            // 无更多元素
            return false;
        }
    }
    
    // 在哈希表中查找下一个元素
    auto it = m_hash.find(key);
    if (it != m_hash.end()) {
        ++it;
        if (it != m_hash.end()) {
            key = it->first;
            value = it->second;
            return true;
        }
    }
    
    // 无更多元素
    return false;
}

Vec<Table::Entry> Table::getEntries() const {
    Vec<Entry> entries;
    
    // 添加数组部分的非nil元素
    for (usize i = 0; i < m_array.size(); ++i) {
        if (!m_array[i].isNil()) {
            entries.push_back({Value::number(static_cast<double>(i + 1)), m_array[i]});
        }
    }
    
    // 添加哈希表部分的所有元素
    for (const auto& pair : m_hash) {
        entries.push_back({pair.first, pair.second});
    }
    
    return entries;
}

void Table::mark() {
    // 调用基类的mark方法
    GCObject::mark();
    
    // 标记元表
    if (m_metatable) {
        m_metatable->mark();
    }
    
    // 标记数组和哈希表中的所有GC对象
    for (const auto& value : m_array) {
        if (value.isGCObject() && value.asGCObject()) {
            value.asGCObject()->mark();
        }
    }
    
    for (const auto& pair : m_hash) {
        // 标记键（如果是GC对象）
        if (pair.first.isGCObject() && pair.first.asGCObject()) {
            pair.first.asGCObject()->mark();
        }
        
        // 标记值（如果是GC对象）
        if (pair.second.isGCObject() && pair.second.asGCObject()) {
            pair.second.asGCObject()->mark();
        }
    }
}

bool Table::isArrayIndex(const Value& key) const {
    if (!key.isNumber()) {
        return false;
    }
    
    double numKey = key.asNumber();
    i32 intKey = static_cast<i32>(numKey);
    
    return static_cast<double>(intKey) == numKey && intKey > 0;
}

i32 Table::getArrayIndex(const Value& key) const {
    if (!isArrayIndex(key)) {
        return -1;  // 非法索引
    }
    
    return static_cast<i32>(key.asNumber());
}

} // namespace Object
} // namespace Lua
