/**
 * @file stdlib_common.h
 * @brief T027 标准库通用头文件和接口定义
 * 
 * 本文件定义了Lua 5.1.5标准库的通用接口、类型和工具：
 * - 标准库模块基础接口
 * - C函数注册机制
 * - 错误处理工具
 * - VM集成接口
 * 
 * 设计原则：
 * 🔍 lua_c_analysis: 严格遵循Lua 5.1.5标准库行为
 * 🏗️ lua_with_cpp: 现代C++实现，类型安全，RAII
 * 🎯 T026集成: 与EnhancedVirtualMachine无缝集成
 * 
 * @author Lua C++ Project Team
 * @date 2025-09-26
 * @version 1.0
 */

#pragma once

#include "core/lua_common.h"
#include "types/value.h"
#include "vm/enhanced_virtual_machine.h"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>

namespace lua_cpp {
namespace stdlib {

// ============================================================================
// 核心类型定义
// ============================================================================

/**
 * @brief C函数类型定义
 * 
 * 符合Lua 5.1.5 lua_CFunction规范
 */
using LuaCFunction = std::function<int(EnhancedVirtualMachine* vm)>;

/**
 * @brief 库函数注册结构
 */
struct LibFunction {
    std::string name;           ///< 函数名
    LuaCFunction func;          ///< 函数指针
    std::string doc;            ///< 文档字符串
};

/**
 * @brief 标准库模块接口
 * 
 * 所有标准库模块都必须实现此接口
 */
class LibraryModule {
public:
    virtual ~LibraryModule() = default;
    
    /**
     * @brief 获取模块名称
     */
    virtual std::string GetModuleName() const = 0;
    
    /**
     * @brief 获取模块版本
     */
    virtual std::string GetModuleVersion() const = 0;
    
    /**
     * @brief 获取模块函数列表
     */
    virtual std::vector<LibFunction> GetFunctions() const = 0;
    
    /**
     * @brief 注册模块到VM
     * @param vm 虚拟机实例
     */
    virtual void RegisterModule(EnhancedVirtualMachine* vm) = 0;
    
    /**
     * @brief 模块初始化
     * @param vm 虚拟机实例
     */
    virtual void Initialize(EnhancedVirtualMachine* vm) {}
    
    /**
     * @brief 模块清理
     * @param vm 虚拟机实例
     */
    virtual void Cleanup(EnhancedVirtualMachine* vm) {}
};

// ============================================================================
// 标准库管理器
// ============================================================================

/**
 * @brief 标准库管理器
 * 
 * 管理所有标准库模块的注册、初始化和生命周期
 */
class StandardLibrary {
public:
    StandardLibrary();
    ~StandardLibrary();
    
    // 禁用拷贝，允许移动
    StandardLibrary(const StandardLibrary&) = delete;
    StandardLibrary& operator=(const StandardLibrary&) = delete;
    StandardLibrary(StandardLibrary&&) = default;
    StandardLibrary& operator=(StandardLibrary&&) = default;
    
    /**
     * @brief 注册标准库模块
     * @param module 模块实例
     */
    void RegisterModule(std::unique_ptr<LibraryModule> module);
    
    /**
     * @brief 初始化所有模块到VM
     * @param vm 虚拟机实例
     */
    void InitializeAll(EnhancedVirtualMachine* vm);
    
    /**
     * @brief 获取已注册的模块
     * @param name 模块名
     * @return 模块指针，如果不存在返回nullptr
     */
    LibraryModule* GetModule(const std::string& name) const;
    
    /**
     * @brief 获取所有模块名称
     */
    std::vector<std::string> GetModuleNames() const;
    
    /**
     * @brief 清理所有模块
     * @param vm 虚拟机实例
     */
    void CleanupAll(EnhancedVirtualMachine* vm);

private:
    std::unordered_map<std::string, std::unique_ptr<LibraryModule>> modules_;
    bool initialized_ = false;
};

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 栈操作工具类
 * 
 * 提供类型安全的栈操作封装
 */
class StackHelper {
public:
    explicit StackHelper(EnhancedVirtualMachine* vm) : vm_(vm) {}
    
    /**
     * @brief 检查参数数量
     * @param expected 期望的参数数量
     * @param func_name 函数名（用于错误消息）
     */
    void CheckArgCount(int expected, const std::string& func_name) const;
    
    /**
     * @brief 检查参数数量范围
     * @param min_args 最小参数数量
     * @param max_args 最大参数数量
     * @param func_name 函数名
     */
    void CheckArgRange(int min_args, int max_args, const std::string& func_name) const;
    
    /**
     * @brief 检查参数类型
     * @param index 栈索引
     * @param expected_type 期望类型
     * @param func_name 函数名
     */
    void CheckArgType(int index, LuaValueType expected_type, const std::string& func_name) const;
    
    /**
     * @brief 安全获取字符串参数
     * @param index 栈索引
     * @param default_value 默认值
     * @return 字符串值
     */
    std::string GetStringArg(int index, const std::string& default_value = "") const;
    
    /**
     * @brief 安全获取数字参数
     * @param index 栈索引
     * @param default_value 默认值
     * @return 数字值
     */
    double GetNumberArg(int index, double default_value = 0.0) const;
    
    /**
     * @brief 安全获取整数参数
     * @param index 栈索引
     * @param default_value 默认值
     * @return 整数值
     */
    int GetIntArg(int index, int default_value = 0) const;
    
    /**
     * @brief 安全获取布尔参数
     * @param index 栈索引
     * @param default_value 默认值
     * @return 布尔值
     */
    bool GetBoolArg(int index, bool default_value = false) const;

private:
    EnhancedVirtualMachine* vm_;
};

/**
 * @brief 错误处理工具
 */
class ErrorHelper {
public:
    /**
     * @brief 抛出参数错误
     * @param func_name 函数名
     * @param arg_index 参数索引
     * @param message 错误消息
     */
    [[noreturn]] static void ArgError(const std::string& func_name, 
                                     int arg_index, 
                                     const std::string& message);
    
    /**
     * @brief 抛出类型错误
     * @param func_name 函数名
     * @param arg_index 参数索引
     * @param expected_type 期望类型
     * @param actual_type 实际类型
     */
    [[noreturn]] static void TypeError(const std::string& func_name,
                                      int arg_index,
                                      const std::string& expected_type,
                                      const std::string& actual_type);
    
    /**
     * @brief 抛出运行时错误
     * @param func_name 函数名
     * @param message 错误消息
     */
    [[noreturn]] static void RuntimeError(const std::string& func_name,
                                         const std::string& message);
};

/**
 * @brief 字符串工具
 */
class StringHelper {
public:
    /**
     * @brief 格式化字符串（printf风格）
     * @param format 格式字符串
     * @param args 参数列表
     * @return 格式化后的字符串
     */
    static std::string Format(const char* format, const std::vector<LuaValue>& args);
    
    /**
     * @brief 检查是否为有效的Lua标识符
     * @param str 字符串
     * @return 是否为有效标识符
     */
    static bool IsValidIdentifier(const std::string& str);
    
    /**
     * @brief 转义字符串
     * @param str 原字符串
     * @return 转义后的字符串
     */
    static std::string Escape(const std::string& str);
    
    /**
     * @brief 反转义字符串
     * @param str 转义后的字符串
     * @return 原字符串
     */
    static std::string Unescape(const std::string& str);
};

// ============================================================================
// 宏定义简化函数注册
// ============================================================================

/**
 * @brief 定义标准库函数的便利宏
 */
#define LUA_STDLIB_FUNCTION(name) \
    int name(EnhancedVirtualMachine* vm)

/**
 * @brief 注册函数到模块的便利宏
 */
#define REGISTER_FUNCTION(functions, name, func, doc) \
    functions.emplace_back(LibFunction{name, func, doc})

/**
 * @brief 获取栈顶参数数量
 */
#define GET_ARG_COUNT(vm) ((vm)->GetStack().size())

/**
 * @brief 检查参数数量宏
 */
#define CHECK_ARG_COUNT(vm, expected, func_name) \
    do { \
        StackHelper helper(vm); \
        helper.CheckArgCount(expected, func_name); \
    } while(0)

/**
 * @brief 检查参数类型宏
 */
#define CHECK_ARG_TYPE(vm, index, type, func_name) \
    do { \
        StackHelper helper(vm); \
        helper.CheckArgType(index, type, func_name); \
    } while(0)

} // namespace stdlib
} // namespace lua_cpp