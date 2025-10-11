/**
 * @file lua_table.h
 * @brief Lua表类型定义
 * @description 简单的Lua表类型定义，用于GC依赖
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#pragma once

#include "core/lua_common.h"
#include "value.h"
#include <unordered_map>
#include <vector>

namespace lua_cpp {

/**
 * @brief 简单的Lua表实现（用于GC测试）
 */
class LuaTable {
public:
    LuaTable() = default;
    ~LuaTable() = default;
    
    // 基本操作
    void Set(const LuaValue& key, const LuaValue& value);
    LuaValue Get(const LuaValue& key) const;
    
    // 大小信息
    Size GetSize() const { return data_.size(); }
    bool IsEmpty() const { return data_.empty(); }
    
    // 遍历支持
    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }
    
private:
    std::unordered_map<std::string, LuaValue> data_;
};

} // namespace lua_cpp