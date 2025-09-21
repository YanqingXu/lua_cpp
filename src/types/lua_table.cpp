/**
 * @file lua_table.cpp
 * @brief Lua表类型实现
 * @description 简单的Lua表类型实现，用于GC测试
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#include "lua_table.h"

namespace lua_cpp {

void LuaTable::Set(const LuaValue& key, const LuaValue& value) {
    // 简化实现：将key转为字符串
    std::string key_str = "key"; // 简化
    data_[key_str] = value;
}

LuaValue LuaTable::Get(const LuaValue& key) const {
    // 简化实现
    std::string key_str = "key"; // 简化
    auto it = data_.find(key_str);
    if (it != data_.end()) {
        return it->second;
    }
    return LuaValue(); // 返回nil
}

} // namespace lua_cpp