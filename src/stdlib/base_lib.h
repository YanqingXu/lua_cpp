/**
 * @file base_lib.h
 * @brief T027 Lua基础库头文件
 * 
 * 实现Lua 5.1.5基础库函数：
 * - 类型检查: type, getmetatable, setmetatable
 * - 类型转换: tostring, tonumber
 * - 表操作: rawget, rawset, rawequal, rawlen
 * - 迭代器: next, pairs, ipairs
 * - 全局环境: getfenv, setfenv
 * - 模块系统: require, module, package
 * - 错误处理: error, assert, pcall, xpcall
 * - 输出函数: print
 * 
 * @author Lua C++ Project Team
 * @date 2025-09-26
 * @version 1.0
 */

#pragma once

#include "stdlib_common.h"

namespace lua_cpp {
namespace stdlib {

/**
 * @brief Lua基础库模块
 * 
 * 实现Lua 5.1.5标准的基础库函数
 */
class BaseLibrary : public LibraryModule {
public:
    BaseLibrary();
    ~BaseLibrary() override = default;
    
    // LibraryModule接口实现
    std::string GetModuleName() const override { return "base"; }
    std::string GetModuleVersion() const override { return "1.0.0"; }
    std::vector<LibFunction> GetFunctions() const override;
    void RegisterModule(EnhancedVirtualMachine* vm) override;
    void Initialize(EnhancedVirtualMachine* vm) override;
    void Cleanup(EnhancedVirtualMachine* vm) override;

private:
    // ====================================================================
    // 基础库函数声明
    // ====================================================================
    
    // 类型检查和操作
    static LUA_STDLIB_FUNCTION(lua_type);
    static LUA_STDLIB_FUNCTION(lua_getmetatable);
    static LUA_STDLIB_FUNCTION(lua_setmetatable);
    
    // 类型转换
    static LUA_STDLIB_FUNCTION(lua_tostring);
    static LUA_STDLIB_FUNCTION(lua_tonumber);
    
    // 表操作
    static LUA_STDLIB_FUNCTION(lua_rawget);
    static LUA_STDLIB_FUNCTION(lua_rawset);
    static LUA_STDLIB_FUNCTION(lua_rawequal);
    static LUA_STDLIB_FUNCTION(lua_rawlen);
    
    // 迭代器
    static LUA_STDLIB_FUNCTION(lua_next);
    static LUA_STDLIB_FUNCTION(lua_pairs);
    static LUA_STDLIB_FUNCTION(lua_ipairs);
    
    // 全局环境
    static LUA_STDLIB_FUNCTION(lua_getfenv);
    static LUA_STDLIB_FUNCTION(lua_setfenv);
    
    // 模块系统
    static LUA_STDLIB_FUNCTION(lua_require);
    static LUA_STDLIB_FUNCTION(lua_module);
    
    // 错误处理
    static LUA_STDLIB_FUNCTION(lua_error);
    static LUA_STDLIB_FUNCTION(lua_assert);
    static LUA_STDLIB_FUNCTION(lua_pcall);
    static LUA_STDLIB_FUNCTION(lua_xpcall);
    
    // 输出函数
    static LUA_STDLIB_FUNCTION(lua_print);
    
    // 其他工具函数
    static LUA_STDLIB_FUNCTION(lua_select);
    static LUA_STDLIB_FUNCTION(lua_unpack);
    static LUA_STDLIB_FUNCTION(lua_loadstring);
    static LUA_STDLIB_FUNCTION(lua_loadfile);
    static LUA_STDLIB_FUNCTION(lua_dofile);
    static LUA_STDLIB_FUNCTION(lua_collectgarbage);
    
    // ====================================================================
    // 内部辅助函数
    // ====================================================================
    
    /**
     * @brief 获取值的类型名字符串
     */
    static std::string GetTypeName(const LuaValue& value);
    
    /**
     * @brief 将值转换为字符串表示
     */
    static std::string ValueToString(const LuaValue& value);
    
    /**
     * @brief 尝试将字符串转换为数字
     */
    static bool StringToNumber(const std::string& str, double& result, int base = 10);
    
    /**
     * @brief 检查表的序列长度
     */
    static size_t GetSequenceLength(const LuaTable& table);
};

} // namespace stdlib
} // namespace lua_cpp