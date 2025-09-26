/**
 * @file table_lib.h
 * @brief T027 Lua表库头文件
 * 
 * 实现Lua 5.1.5表库函数：
 * - 数组操作: insert, remove, sort, concat
 * - 表遍历: foreach (已废弃但保留兼容)
 * - 表工具: maxn, getn, setn (部分在5.1中已移除)
 * 
 * @author Lua C++ Project Team
 * @date 2025-09-26
 * @version 1.0
 */

#pragma once

#include "stdlib_common.h"
#include <functional>

namespace lua_cpp {
namespace stdlib {

/**
 * @brief Lua表库模块
 * 
 * 实现Lua 5.1.5标准的表库函数
 */
class TableLibrary : public LibraryModule {
public:
    TableLibrary();
    ~TableLibrary() override = default;
    
    // LibraryModule接口实现
    std::string GetModuleName() const override { return "table"; }
    std::string GetModuleVersion() const override { return "1.0.0"; }
    std::vector<LibFunction> GetFunctions() const override;
    void RegisterModule(EnhancedVirtualMachine* vm) override;
    void Initialize(EnhancedVirtualMachine* vm) override;
    void Cleanup(EnhancedVirtualMachine* vm) override;

private:
    // ====================================================================
    // 表库函数声明
    // ====================================================================
    
    // 数组操作
    static LUA_STDLIB_FUNCTION(lua_table_insert);
    static LUA_STDLIB_FUNCTION(lua_table_remove);
    static LUA_STDLIB_FUNCTION(lua_table_sort);
    static LUA_STDLIB_FUNCTION(lua_table_concat);
    
    // 表工具
    static LUA_STDLIB_FUNCTION(lua_table_maxn);
    static LUA_STDLIB_FUNCTION(lua_table_foreach);
    static LUA_STDLIB_FUNCTION(lua_table_foreachi);
    
    // ====================================================================
    // 内部辅助函数
    // ====================================================================
    
    /**
     * @brief 获取表的数组长度
     * @param table 目标表
     * @return 数组长度
     */
    static size_t GetArrayLength(const LuaTable& table);
    
    /**
     * @brief 获取表的最大数字键
     * @param table 目标表
     * @return 最大数字键，如果没有则返回0
     */
    static double GetMaxNumericKey(const LuaTable& table);
    
    /**
     * @brief 表排序比较函数类型
     */
    using CompareFunction = std::function<bool(const LuaValue&, const LuaValue&)>;
    
    /**
     * @brief 默认排序比较函数
     * @param a 第一个值
     * @param b 第二个值
     * @return a < b
     */
    static bool DefaultCompare(const LuaValue& a, const LuaValue& b);
    
    /**
     * @brief 对表进行快速排序
     * @param table 目标表
     * @param compare 比较函数
     * @param start 起始索引（1基）
     * @param end 结束索引（1基）
     */
    static void QuickSort(LuaTable& table, CompareFunction compare, int start, int end);
    
    /**
     * @brief 快速排序分区函数
     * @param table 目标表
     * @param compare 比较函数
     * @param low 低位索引
     * @param high 高位索引
     * @return 分区点索引
     */
    static int Partition(LuaTable& table, CompareFunction compare, int low, int high);
    
    /**
     * @brief 交换表中两个位置的值
     * @param table 目标表
     * @param i 第一个位置（1基）
     * @param j 第二个位置（1基）
     */
    static void SwapTableElements(LuaTable& table, int i, int j);
    
    /**
     * @brief 验证表索引的有效性
     * @param index 索引值
     * @param table_len 表长度
     * @param func_name 函数名（用于错误报告）
     * @return 有效的索引值
     */
    static int ValidateIndex(int index, size_t table_len, const std::string& func_name);
};

} // namespace stdlib
} // namespace lua_cpp