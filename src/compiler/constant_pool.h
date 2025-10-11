/**
 * @file constant_pool.h
 * @brief 常量池管理器头文件
 * @description 管理编译时常量的存储、查找和优化
 * @author Lua C++ Project
 * @date 2025-09-25
 */

#pragma once

#include "bytecode.h"
#include "../types/value.h"
#include "../core/lua_common.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cmath>

namespace lua_cpp {

/* ========================================================================== */
/* LuaValue 哈希函数 */
/* ========================================================================== */

/**
 * @brief LuaValue 哈希函数对象
 */
struct LuaValueHash {
    std::size_t operator()(const LuaValue& value) const {
        switch (value.GetType()) {
            case LuaType::Nil:
                return 0;
            case LuaType::Bool:
                return std::hash<bool>{}(value.AsBool());
            case LuaType::Number:
                return std::hash<double>{}(value.AsNumber());
            case LuaType::String:
                return std::hash<std::string>{}(value.AsString());
            default:
                return 0;
        }
    }
};

/* ========================================================================== */
/* 常量池类 */
/* ========================================================================== */

/**
 * @brief 常量池管理器
 * @description 管理编译过程中的常量，提供去重和快速查找功能
 */
class ConstantPool {
public:
    /**
     * @brief 构造函数
     */
    ConstantPool();
    
    /**
     * @brief 析构函数
     */
    ~ConstantPool();
    
    // 禁用拷贝，允许移动
    ConstantPool(const ConstantPool&) = delete;
    ConstantPool& operator=(const ConstantPool&) = delete;
    ConstantPool(ConstantPool&&) = default;
    ConstantPool& operator=(ConstantPool&&) = default;
    
    /* ====================================================================== */
    /* 常量管理 */
    /* ====================================================================== */
    
    /**
     * @brief 添加常量
     * @param value 常量值
     * @return 常量在池中的索引
     * @throws CompilerError 如果常量池溢出
     */
    int AddConstant(const LuaValue& value);
    
    /**
     * @brief 查找常量
     * @param value 要查找的常量值
     * @return 常量索引，如果不存在返回-1
     */
    int FindConstant(const LuaValue& value) const;
    
    /**
     * @brief 获取常量
     * @param index 常量索引
     * @return 常量值的引用
     * @throws CompilerError 如果索引无效
     */
    const LuaValue& GetConstant(int index) const;
    
    /**
     * @brief 获取常量池大小
     * @return 常量数量
     */
    Size GetSize() const;
    
    /**
     * @brief 获取所有常量
     * @return 常量向量的引用
     */
    const std::vector<LuaValue>& GetConstants() const;
    
    /**
     * @brief 检查常量池是否为空
     * @return 是否为空
     */
    bool IsEmpty() const;
    
    /* ====================================================================== */
    /* 工具方法 */
    /* ====================================================================== */
    
    /**
     * @brief 清空常量池
     */
    void Clear();
    
    /**
     * @brief 预留空间
     * @param capacity 容量
     */
    void Reserve(Size capacity);
    
    /**
     * @brief 添加数值常量
     * @param number 数值
     * @return 常量索引
     */
    int AddNumber(double number);
    
    /**
     * @brief 添加字符串常量
     * @param str 字符串
     * @return 常量索引
     */
    int AddString(const std::string& str);
    
    /**
     * @brief 添加布尔常量
     * @param value 布尔值
     * @return 常量索引
     */
    int AddBoolean(bool value);
    
    /**
     * @brief 添加nil常量
     * @return 常量索引
     */
    int AddNil();
    
    /**
     * @brief 按类型查找常量
     * @param type 常量类型
     * @return 该类型的所有常量索引
     */
    std::vector<int> FindConstantsByType(LuaType type) const;
    
    /**
     * @brief 检查是否包含指定常量
     * @param value 常量值
     * @return 是否包含
     */
    bool HasConstant(const LuaValue& value) const;
    
    /**
     * @brief 优化存储
     * @description 重新组织常量池以提高性能
     */
    void OptimizeStorage();
    
    /**
     * @brief 转换为字符串表示
     * @return 字符串表示
     */
    std::string ToString() const;

private:
    std::vector<LuaValue> constants_;                           // 常量存储
    std::unordered_map<LuaValue, int, LuaValueHash> constant_map_; // 常量到索引的映射
};

/* ========================================================================== */
/* 常量池构建器 */
/* ========================================================================== */

/**
 * @brief 常量池构建器
 * @description 提供便利的常量池构建接口
 */
class ConstantPoolBuilder {
public:
    /**
     * @brief 构造函数
     */
    ConstantPoolBuilder();
    
    /**
     * @brief 析构函数
     */
    ~ConstantPoolBuilder();
    
    // 禁用拷贝，允许移动
    ConstantPoolBuilder(const ConstantPoolBuilder&) = delete;
    ConstantPoolBuilder& operator=(const ConstantPoolBuilder&) = delete;
    ConstantPoolBuilder(ConstantPoolBuilder&&) = default;
    ConstantPoolBuilder& operator=(ConstantPoolBuilder&&) = default;
    
    /* ====================================================================== */
    /* 构建接口 */
    /* ====================================================================== */
    
    /**
     * @brief 添加常量
     * @param value 常量值
     * @return 常量索引
     */
    int AddConstant(const LuaValue& value);
    
    /**
     * @brief 添加数值常量
     * @param number 数值
     * @return 常量索引
     */
    int AddNumber(double number);
    
    /**
     * @brief 添加字符串常量
     * @param str 字符串
     * @return 常量索引
     */
    int AddString(const std::string& str);
    
    /**
     * @brief 添加布尔常量
     * @param value 布尔值
     * @return 常量索引
     */
    int AddBoolean(bool value);
    
    /**
     * @brief 添加nil常量
     * @return 常量索引
     */
    int AddNil();
    
    /**
     * @brief 查找或添加常量
     * @param value 常量值
     * @return 常量索引
     */
    int FindOrAddConstant(const LuaValue& value);
    
    /**
     * @brief 查找或添加数值常量
     * @param number 数值
     * @return 常量索引
     */
    int FindOrAddNumber(double number);
    
    /**
     * @brief 查找或添加字符串常量
     * @param str 字符串
     * @return 常量索引
     */
    int FindOrAddString(const std::string& str);
    
    /**
     * @brief 查找或添加布尔常量
     * @param value 布尔值
     * @return 常量索引
     */
    int FindOrAddBoolean(bool value);
    
    /**
     * @brief 构建常量池
     * @return 构建好的常量池
     */
    ConstantPool Build() &&;
    
    /**
     * @brief 获取当前常量池（只读）
     * @return 常量池引用
     */
    const ConstantPool& GetPool() const;
    
    /**
     * @brief 获取当前大小
     * @return 常量数量
     */
    Size GetSize() const;
    
    /**
     * @brief 清空构建器
     */
    void Clear();
    
    /**
     * @brief 预留空间
     * @param capacity 容量
     */
    void Reserve(Size capacity);

private:
    ConstantPool pool_;
};

/* ========================================================================== */
/* 常量优化函数 */
/* ========================================================================== */

/**
 * @brief 检查常量是否可以内联
 * @param value 常量值
 * @return 是否可以内联
 */
bool CanBeInlined(const LuaValue& value);

/**
 * @brief 常量折叠：计算两个常量的二元运算结果
 * @param left 左操作数
 * @param right 右操作数
 * @param op 操作码
 * @return 计算结果，如果无法计算返回nil
 */
LuaValue FoldConstants(const LuaValue& left, const LuaValue& right, OpCode op);

/**
 * @brief 常量折叠：计算一元运算结果
 * @param operand 操作数
 * @param op 操作码
 * @return 计算结果，如果无法计算返回nil
 */
LuaValue FoldUnaryConstant(const LuaValue& operand, OpCode op);

/**
 * @brief 检查值是否为编译时常量
 * @param value 值
 * @return 是否为编译时常量
 */
bool IsConstantExpression(const LuaValue& value);

/**
 * @brief 估算常量池大小
 * @param values 常量值列表
 * @return 去重后的估算大小
 */
Size EstimateConstantPoolSize(const std::vector<LuaValue>& values);

/**
 * @brief 优化常量池
 * @param pool 要优化的常量池
 */
void OptimizeConstantPool(ConstantPool& pool);

} // namespace lua_cpp