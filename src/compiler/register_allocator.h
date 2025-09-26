/**
 * @file register_allocator.h
 * @brief 寄存器分配器头文件
 * @description 管理Lua虚拟机寄存器的分配、释放和作用域管理
 * @author Lua C++ Project
 * @date 2025-09-25
 */

#pragma once

#include "../core/lua_common.h"
#include <vector>
#include <string>
#include <stack>
#include <sstream>

namespace lua_cpp {

/* ========================================================================== */
/* 寄存器相关常量和类型 */
/* ========================================================================== */

/**
 * @brief 无效寄存器索引
 */
constexpr RegisterIndex INVALID_REGISTER = static_cast<RegisterIndex>(-1);

/**
 * @brief 最大寄存器索引
 */
constexpr RegisterIndex MAX_REGISTER_INDEX = 255; // Lua 5.1.5 限制

/**
 * @brief 默认最大寄存器数量
 */
constexpr Size DEFAULT_MAX_REGISTERS = 256;

/**
 * @brief 寄存器类型枚举
 */
enum class RegisterType {
    Local,      // 局部变量寄存器
    Temporary,  // 临时寄存器
    Parameter,  // 参数寄存器
    Reserved    // 保留寄存器
};

/**
 * @brief 寄存器信息结构
 */
struct RegisterInfo {
    RegisterType type = RegisterType::Local;    // 寄存器类型
    std::string name;                          // 寄存器名称（用于调试）
    RegisterIndex index = INVALID_REGISTER;    // 寄存器索引
    bool is_temp = false;                      // 是否为临时寄存器
    
    RegisterInfo() = default;
    
    RegisterInfo(RegisterType t, const std::string& n, RegisterIndex idx, bool temp = false)
        : type(t), name(n), index(idx), is_temp(temp) {}
};

/* ========================================================================== */
/* 局部变量信息 */
/* ========================================================================== */

/**
 * @brief 局部变量信息结构
 */
struct LocalVariable {
    std::string name;           // 变量名
    RegisterIndex register_idx; // 寄存器索引
    int scope_level;           // 作用域层级
    bool is_captured;          // 是否被闭包捕获
    
    LocalVariable() 
        : register_idx(INVALID_REGISTER), scope_level(0), is_captured(false) {}
    
    LocalVariable(const std::string& n, RegisterIndex reg, int level = 0)
        : name(n), register_idx(reg), scope_level(level), is_captured(false) {}
};

/* ========================================================================== */
/* 寄存器分配器类 */
/* ========================================================================== */

/**
 * @brief 寄存器分配器
 * @description 管理Lua虚拟机寄存器的分配和释放
 */
class RegisterAllocator {
public:
    /**
     * @brief 构造函数
     * @param max_registers 最大寄存器数量
     */
    explicit RegisterAllocator(Size max_registers = DEFAULT_MAX_REGISTERS);
    
    /**
     * @brief 析构函数
     */
    ~RegisterAllocator();
    
    // 禁用拷贝，允许移动
    RegisterAllocator(const RegisterAllocator&) = delete;
    RegisterAllocator& operator=(const RegisterAllocator&) = delete;
    RegisterAllocator(RegisterAllocator&&) = default;
    RegisterAllocator& operator=(RegisterAllocator&&) = default;
    
    /* ====================================================================== */
    /* 基础分配接口 */
    /* ====================================================================== */
    
    /**
     * @brief 分配一个寄存器
     * @return 分配的寄存器索引
     * @throws CompilerError 如果无法分配
     */
    RegisterIndex Allocate();
    
    /**
     * @brief 分配一个带名称的寄存器
     * @param name 寄存器名称
     * @return 分配的寄存器索引
     * @throws CompilerError 如果无法分配
     */
    RegisterIndex AllocateNamed(const std::string& name);
    
    /**
     * @brief 分配一个临时寄存器
     * @return 分配的寄存器索引
     * @throws CompilerError 如果无法分配
     */
    RegisterIndex AllocateTemporary();
    
    /**
     * @brief 分配连续的寄存器范围
     * @param count 需要分配的寄存器数量
     * @return 起始寄存器索引
     * @throws CompilerError 如果无法分配
     */
    RegisterIndex AllocateRange(Size count);
    
    /* ====================================================================== */
    /* 释放接口 */
    /* ====================================================================== */
    
    /**
     * @brief 释放寄存器
     * @param reg 要释放的寄存器索引
     */
    void Free(RegisterIndex reg);
    
    /**
     * @brief 释放寄存器范围
     * @param start 起始寄存器索引
     * @param count 要释放的寄存器数量
     */
    void FreeRange(RegisterIndex start, Size count);
    
    /**
     * @brief 释放临时寄存器
     * @param saved_top 保存的寄存器栈顶位置
     */
    void FreeTemporaries(Size saved_top);
    
    /**
     * @brief 释放所有临时寄存器
     */
    void FreeAllTemporaries();
    
    /* ====================================================================== */
    /* 栈管理 */
    /* ====================================================================== */
    
    /**
     * @brief 获取寄存器栈顶
     * @return 栈顶位置
     */
    Size GetTop() const;
    
    /**
     * @brief 设置寄存器栈顶
     * @param top 新的栈顶位置
     */
    void SetTop(Size top);
    
    /**
     * @brief 获取临时寄存器栈顶
     * @return 临时栈顶位置
     */
    Size GetTempTop() const;
    
    /**
     * @brief 保存临时寄存器栈顶
     * @return 当前临时栈顶位置
     */
    Size SaveTempTop();
    
    /**
     * @brief 恢复临时寄存器栈顶
     */
    void RestoreTempTop();
    
    /* ====================================================================== */
    /* 状态查询 */
    /* ====================================================================== */
    
    /**
     * @brief 获取空闲寄存器数量
     * @return 空闲寄存器数量
     */
    Size GetFreeCount() const;
    
    /**
     * @brief 获取已使用寄存器数量
     * @return 已使用寄存器数量
     */
    Size GetUsedCount() const;
    
    /**
     * @brief 检查寄存器是否已分配
     * @param reg 寄存器索引
     * @return 是否已分配
     */
    bool IsAllocated(RegisterIndex reg) const;
    
    /**
     * @brief 检查寄存器是否空闲
     * @param reg 寄存器索引
     * @return 是否空闲
     */
    bool IsFree(RegisterIndex reg) const;
    
    /**
     * @brief 检查寄存器是否为临时寄存器
     * @param reg 寄存器索引
     * @return 是否为临时寄存器
     */
    bool IsTemporary(RegisterIndex reg) const;
    
    /* ====================================================================== */
    /* 寄存器信息 */
    /* ====================================================================== */
    
    /**
     * @brief 获取寄存器名称
     * @param reg 寄存器索引
     * @return 寄存器名称
     */
    const std::string& GetRegisterName(RegisterIndex reg) const;
    
    /**
     * @brief 设置寄存器名称
     * @param reg 寄存器索引
     * @param name 寄存器名称
     */
    void SetRegisterName(RegisterIndex reg, const std::string& name);
    
    /**
     * @brief 获取寄存器类型
     * @param reg 寄存器索引
     * @return 寄存器类型
     */
    RegisterType GetRegisterType(RegisterIndex reg) const;
    
    /* ====================================================================== */
    /* 工具方法 */
    /* ====================================================================== */
    
    /**
     * @brief 重置分配器状态
     */
    void Reset();
    
    /**
     * @brief 预留寄存器
     * @param count 要预留的寄存器数量
     */
    void Reserve(Size count);
    
    /**
     * @brief 获取所有已分配的寄存器
     * @return 已分配寄存器索引列表
     */
    std::vector<RegisterIndex> GetAllocatedRegisters() const;
    
    /**
     * @brief 获取所有临时寄存器
     * @return 临时寄存器索引列表
     */
    std::vector<RegisterIndex> GetTemporaryRegisters() const;
    
    /**
     * @brief 转换为字符串表示
     * @return 字符串表示
     */
    std::string ToString() const;

private:
    Size max_registers_;                        // 最大寄存器数量
    RegisterIndex next_register_;               // 下一个分配的寄存器
    Size register_top_;                         // 寄存器栈顶
    Size temp_top_;                            // 临时寄存器栈顶
    
    std::vector<bool> free_registers_;          // 寄存器空闲状态
    std::vector<RegisterInfo> register_info_;   // 寄存器信息
    std::stack<Size> temp_markers_;            // 临时寄存器标记栈
    
    /**
     * @brief 重新计算栈顶位置
     */
    void RecalculateTop();
};

/* ========================================================================== */
/* 作用域管理器类 */
/* ========================================================================== */

/**
 * @brief 作用域管理器
 * @description 管理局部变量的作用域和生命周期
 */
class ScopeManager {
public:
    /**
     * @brief 构造函数
     */
    ScopeManager();
    
    /**
     * @brief 析构函数
     */
    ~ScopeManager();
    
    // 禁用拷贝，允许移动
    ScopeManager(const ScopeManager&) = delete;
    ScopeManager& operator=(const ScopeManager&) = delete;
    ScopeManager(ScopeManager&&) = default;
    ScopeManager& operator=(ScopeManager&&) = default;
    
    /* ====================================================================== */
    /* 作用域管理 */
    /* ====================================================================== */
    
    /**
     * @brief 进入新作用域
     */
    void EnterScope();
    
    /**
     * @brief 退出当前作用域
     * @return 退出作用域时释放的变量数量
     */
    int ExitScope();
    
    /**
     * @brief 获取当前作用域层级
     * @return 作用域层级
     */
    int GetCurrentLevel() const;
    
    /* ====================================================================== */
    /* 局部变量管理 */
    /* ====================================================================== */
    
    /**
     * @brief 声明局部变量
     * @param name 变量名
     * @param allocator 寄存器分配器
     * @return 分配的寄存器索引
     */
    RegisterIndex DeclareLocal(const std::string& name, RegisterAllocator& allocator);
    
    /**
     * @brief 查找局部变量
     * @param name 变量名
     * @return 局部变量信息，如果不存在返回nullptr
     */
    const LocalVariable* FindLocal(const std::string& name) const;
    
    /**
     * @brief 获取局部变量的寄存器
     * @param name 变量名
     * @return 寄存器索引，如果不存在返回INVALID_REGISTER
     */
    RegisterIndex GetLocalRegister(const std::string& name) const;
    
    /**
     * @brief 获取所有局部变量
     * @return 局部变量列表的引用
     */
    const std::vector<LocalVariable>& GetLocals() const;
    
    /**
     * @brief 获取指定作用域层级的局部变量
     * @param level 作用域层级
     * @return 该层级的局部变量列表
     */
    std::vector<LocalVariable> GetLocalsInScope(int level) const;
    
    /**
     * @brief 检查局部变量是否已声明
     * @param name 变量名
     * @return 是否已声明
     */
    bool IsLocalDeclared(const std::string& name) const;
    
    /* ====================================================================== */
    /* 闭包相关 */
    /* ====================================================================== */
    
    /**
     * @brief 标记变量为被捕获
     * @param name 变量名
     */
    void MarkCaptured(const std::string& name);
    
    /**
     * @brief 检查变量是否被捕获
     * @param name 变量名
     * @return 是否被捕获
     */
    bool IsCaptured(const std::string& name) const;
    
    /* ====================================================================== */
    /* 状态查询 */
    /* ====================================================================== */
    
    /**
     * @brief 获取局部变量数量
     * @return 变量数量
     */
    Size GetLocalCount() const;
    
    /**
     * @brief 清空管理器状态
     */
    void Clear();
    
    /**
     * @brief 转换为字符串表示
     * @return 字符串表示
     */
    std::string ToString() const;

private:
    std::vector<LocalVariable> locals_;     // 局部变量列表
    std::vector<Size> scope_markers_;       // 作用域标记
    int current_level_;                     // 当前作用域层级
};

/* ========================================================================== */
/* 辅助函数 */
/* ========================================================================== */

/**
 * @brief 检查寄存器索引是否有效
 * @param reg 寄存器索引
 * @return 是否有效
 */
bool IsValidRegister(RegisterIndex reg);

/**
 * @brief 获取下一个寄存器索引
 * @param reg 当前寄存器索引
 * @return 下一个寄存器索引
 */
RegisterIndex NextRegister(RegisterIndex reg);

/**
 * @brief 获取上一个寄存器索引
 * @param reg 当前寄存器索引
 * @return 上一个寄存器索引，如果没有则返回INVALID_REGISTER
 */
RegisterIndex PrevRegister(RegisterIndex reg);

/**
 * @brief 计算寄存器范围大小
 * @param start 起始寄存器
 * @param end 结束寄存器
 * @return 范围大小
 */
Size CalculateRegisterRange(RegisterIndex start, RegisterIndex end);

/**
 * @brief 将寄存器索引转换为字符串
 * @param reg 寄存器索引
 * @return 字符串表示
 */
std::string RegisterToString(RegisterIndex reg);

} // namespace lua_cpp