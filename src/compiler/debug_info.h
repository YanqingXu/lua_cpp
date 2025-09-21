/**
 * @file debug_info.h
 * @brief 调试信息生成器声明
 * @description 定义调试信息生成器，用于生成源码映射和变量信息
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#pragma once

#include "../core/lua_common.h"
#include "bytecode.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace lua_cpp {

/* ========================================================================== */
/* 调试信息结构 */
/* ========================================================================== */

/**
 * @brief 局部变量调试信息
 */
struct LocalDebugInfo {
    std::string name;        // 变量名
    int start_pc;           // 开始有效的PC
    int end_pc;             // 结束有效的PC
    RegisterIndex register_idx; // 寄存器索引
    
    LocalDebugInfo(const std::string& n, int start, int end, RegisterIndex reg)
        : name(n), start_pc(start), end_pc(end), register_idx(reg) {}
};

/**
 * @brief 上值调试信息
 */
struct UpvalueDebugInfo {
    std::string name;        // 上值名
    bool in_stack;          // 是否在栈中
    int index;              // 索引
    
    UpvalueDebugInfo(const std::string& n, bool stack, int idx)
        : name(n), in_stack(stack), index(idx) {}
};

/**
 * @brief 源码位置信息
 */
struct SourceLocation {
    int line;               // 行号
    int column;             // 列号
    
    SourceLocation(int l = 0, int c = 0) : line(l), column(c) {}
};

/**
 * @brief 函数调试信息
 */
struct FunctionDebugInfo {
    std::string name;                           // 函数名
    std::string source_name;                    // 源文件名
    int line_defined;                          // 定义行号
    int last_line_defined;                     // 结束行号
    std::vector<int> line_info;                // 每条指令对应的行号
    std::vector<LocalDebugInfo> locals;        // 局部变量信息
    std::vector<UpvalueDebugInfo> upvalues;    // 上值信息
    
    FunctionDebugInfo() = default;
    FunctionDebugInfo(const std::string& n, const std::string& src = "")
        : name(n), source_name(src), line_defined(0), last_line_defined(0) {}
};

/* ========================================================================== */
/* 调试信息生成器 */
/* ========================================================================== */

/**
 * @brief 调试信息生成器类
 * @description 负责生成和管理字节码的调试信息
 */
class DebugInfoGenerator {
public:
    /**
     * @brief 构造函数
     */
    DebugInfoGenerator() = default;
    
    /**
     * @brief 析构函数
     */
    ~DebugInfoGenerator() = default;
    
    // 禁用拷贝，允许移动
    DebugInfoGenerator(const DebugInfoGenerator&) = delete;
    DebugInfoGenerator& operator=(const DebugInfoGenerator&) = delete;
    DebugInfoGenerator(DebugInfoGenerator&&) = default;
    DebugInfoGenerator& operator=(DebugInfoGenerator&&) = default;
    
    /* ===== 函数调试信息管理 ===== */
    
    /**
     * @brief 开始新函数的调试信息收集
     * @param name 函数名
     * @param source_name 源文件名
     * @param line_defined 定义行号
     */
    void BeginFunction(const std::string& name, 
                      const std::string& source_name = "",
                      int line_defined = 0);
    
    /**
     * @brief 结束当前函数的调试信息收集
     * @param last_line_defined 结束行号
     * @return 函数调试信息
     */
    FunctionDebugInfo EndFunction(int last_line_defined = 0);
    
    /* ===== 行号信息管理 ===== */
    
    /**
     * @brief 为指令设置行号
     * @param pc 指令地址
     * @param line 行号
     */
    void SetLineInfo(int pc, int line);
    
    /**
     * @brief 批量设置行号信息
     * @param line_info 行号信息数组
     */
    void SetLineInfo(const std::vector<int>& line_info);
    
    /**
     * @brief 获取指令的行号
     * @param pc 指令地址
     * @return 行号，如果没有信息则返回0
     */
    int GetLineInfo(int pc) const;
    
    /* ===== 局部变量信息管理 ===== */
    
    /**
     * @brief 注册局部变量
     * @param name 变量名
     * @param start_pc 开始有效的PC
     * @param register_idx 寄存器索引
     */
    void RegisterLocal(const std::string& name, int start_pc, RegisterIndex register_idx);
    
    /**
     * @brief 结束局部变量的生命周期
     * @param name 变量名
     * @param end_pc 结束有效的PC
     */
    void EndLocal(const std::string& name, int end_pc);
    
    /**
     * @brief 批量结束局部变量
     * @param end_pc 结束PC
     * @param count 结束的变量数量（从最后开始）
     */
    void EndLocals(int end_pc, int count);
    
    /**
     * @brief 获取指定PC处的局部变量信息
     * @param pc 指令地址
     * @return 局部变量信息列表
     */
    std::vector<LocalDebugInfo> GetLocalsAtPC(int pc) const;
    
    /* ===== 上值信息管理 ===== */
    
    /**
     * @brief 注册上值
     * @param name 上值名
     * @param in_stack 是否在栈中
     * @param index 索引
     */
    void RegisterUpvalue(const std::string& name, bool in_stack, int index);
    
    /**
     * @brief 获取上值信息
     * @param index 上值索引
     * @return 上值调试信息
     */
    const UpvalueDebugInfo* GetUpvalueInfo(int index) const;
    
    /* ===== 源码位置映射 ===== */
    
    /**
     * @brief 设置源码位置映射
     * @param pc 指令地址
     * @param location 源码位置
     */
    void SetSourceLocation(int pc, const SourceLocation& location);
    
    /**
     * @brief 获取指令的源码位置
     * @param pc 指令地址
     * @return 源码位置
     */
    SourceLocation GetSourceLocation(int pc) const;
    
    /* ===== 调试信息查询 ===== */
    
    /**
     * @brief 获取当前函数调试信息
     */
    const FunctionDebugInfo& GetCurrentFunctionInfo() const;
    
    /**
     * @brief 获取指定名称的局部变量
     * @param name 变量名
     * @param pc 指令地址
     * @return 局部变量信息，如果没有找到返回nullptr
     */
    const LocalDebugInfo* FindLocal(const std::string& name, int pc) const;
    
    /**
     * @brief 清除所有调试信息
     */
    void Clear();

private:
    /* 当前函数调试信息 */
    FunctionDebugInfo current_function_;
    
    /* 源码位置映射 */
    std::unordered_map<int, SourceLocation> source_locations_;
    
    /* 临时局部变量存储（在函数结束前） */
    std::vector<LocalDebugInfo> temp_locals_;
    
    /* ===== 辅助方法 ===== */
    
    /**
     * @brief 查找局部变量索引
     * @param name 变量名
     * @return 变量索引，如果没有找到返回-1
     */
    int FindLocalIndex(const std::string& name) const;
    
    /**
     * @brief 扩展行号信息数组
     * @param size 新大小
     */
    void ResizeLineInfo(Size size);
};

/* ========================================================================== */
/* 调试信息工具函数 */
/* ========================================================================== */

/**
 * @brief 格式化源码位置
 * @param location 源码位置
 * @return 格式化字符串
 */
std::string FormatSourceLocation(const SourceLocation& location);

/**
 * @brief 格式化局部变量信息
 * @param local 局部变量信息
 * @return 格式化字符串
 */
std::string FormatLocalInfo(const LocalDebugInfo& local);

/**
 * @brief 格式化函数调试信息
 * @param info 函数调试信息
 * @return 格式化字符串
 */
std::string FormatFunctionInfo(const FunctionDebugInfo& info);

} // namespace lua_cpp