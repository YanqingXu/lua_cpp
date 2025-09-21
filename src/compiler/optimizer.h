/**
 * @file optimizer.h
 * @brief 字节码优化器声明
 * @description 定义字节码优化器类，实现各种编译优化技术
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#pragma once

#include "bytecode.h"
#include "../types/value.h"
#include <vector>
#include <cmath>

namespace lua_cpp {

/* ========================================================================== */
/* 优化配置 */
/* ========================================================================== */

enum class OptimizationType {
    ConstantFolding,        // 常量折叠
    DeadCodeElimination,    // 死代码消除
    JumpOptimization,       // 跳转优化
    LocalVariableReuse,     // 局部变量重用
    PeepholeOptimization,   // 窥孔优化
    LoopOptimization        // 循环优化
};

/**
 * @brief 优化配置类
 */
class OptimizationConfig {
public:
    OptimizationConfig() = default;
    
    /**
     * @brief 启用指定优化
     */
    void Enable(OptimizationType type) {
        enabled_optimizations_.insert(type);
    }
    
    /**
     * @brief 禁用指定优化
     */
    void Disable(OptimizationType type) {
        enabled_optimizations_.erase(type);
    }
    
    /**
     * @brief 检查优化是否启用
     */
    bool IsEnabled(OptimizationType type) const {
        return enabled_optimizations_.count(type) > 0;
    }
    
    /**
     * @brief 启用所有优化
     */
    void EnableAll() {
        Enable(OptimizationType::ConstantFolding);
        Enable(OptimizationType::DeadCodeElimination);
        Enable(OptimizationType::JumpOptimization);
        Enable(OptimizationType::LocalVariableReuse);
        Enable(OptimizationType::PeepholeOptimization);
        Enable(OptimizationType::LoopOptimization);
    }
    
    /**
     * @brief 禁用所有优化
     */
    void DisableAll() {
        enabled_optimizations_.clear();
    }

private:
    std::set<OptimizationType> enabled_optimizations_;
};

/* ========================================================================== */
/* 指令格式枚举 */
/* ========================================================================== */

enum class InstructionMode {
    iABC,    // 3个操作数格式
    iABx,    // A和Bx格式
    iAsBx    // A和有符号Bx格式
};

/* ========================================================================== */
/* 字节码优化器 */
/* ========================================================================== */

/**
 * @brief 字节码优化器类
 * @description 实现各种字节码优化技术，提升执行效率
 */
class BytecodeOptimizer {
public:
    /**
     * @brief 构造函数
     * @param config 优化配置
     */
    explicit BytecodeOptimizer(const OptimizationConfig& config);
    
    /**
     * @brief 执行优化
     * @param instructions 指令序列
     * @param constants 常量表
     * @param line_info 行号信息
     */
    void Optimize(std::vector<Instruction>& instructions,
                  std::vector<LuaValue>& constants,
                  std::vector<int>& line_info);

private:
    /* 优化配置 */
    OptimizationConfig config_;
    
    /* ===== 优化实现方法 ===== */
    
    /**
     * @brief 常量折叠优化
     * @description 在编译时计算常量表达式
     */
    void PerformConstantFolding(std::vector<Instruction>& instructions,
                                std::vector<LuaValue>& constants);
    
    /**
     * @brief 死代码消除优化
     * @description 移除永远不会执行的代码
     */
    void PerformDeadCodeElimination(std::vector<Instruction>& instructions,
                                    std::vector<int>& line_info);
    
    /**
     * @brief 跳转优化
     * @description 优化跳转指令，消除不必要的跳转
     */
    void PerformJumpOptimization(std::vector<Instruction>& instructions);
    
    /**
     * @brief 局部变量重用优化
     * @description 重用不再使用的局部变量寄存器
     */
    void PerformLocalVariableReuse(std::vector<Instruction>& instructions);
    
    /* ===== 辅助方法 ===== */
    
    /**
     * @brief 检查是否是可折叠的二元运算
     */
    bool IsFoldableBinaryOp(OpCode op) const;
    
    /**
     * @brief 检查是否是可折叠的一元运算
     */
    bool IsFoldableUnaryOp(OpCode op) const;
    
    /**
     * @brief 检查寄存器是否加载常量
     */
    bool IsConstantLoad(const std::vector<Instruction>& instructions,
                        Size current_pc, int reg) const;
    
    /**
     * @brief 获取寄存器的常量值
     */
    LuaValue GetConstantValue(const std::vector<Instruction>& instructions,
                              const std::vector<LuaValue>& constants,
                              Size current_pc, int reg) const;
    
    /**
     * @brief 尝试折叠常量运算
     */
    bool TryFoldConstants(OpCode op, const LuaValue& left, 
                          const LuaValue& right, LuaValue& result) const;
    
    /**
     * @brief 尝试折叠一元常量运算
     */
    bool TryFoldUnaryConstant(OpCode op, const LuaValue& operand, 
                              LuaValue& result) const;
    
    /**
     * @brief 添加常量到常量表
     */
    int AddConstant(std::vector<LuaValue>& constants, const LuaValue& value);
    
    /**
     * @brief 标记指令为删除
     */
    void MarkForDeletion(std::vector<Instruction>& instructions,
                         Size current_pc, int reg);
    
    /**
     * @brief 移除标记删除的指令
     */
    void RemoveMarkedInstructions(std::vector<Instruction>& instructions);
    
    /**
     * @brief 获取指令格式
     */
    InstructionMode GetInstructionMode(OpCode op) const;
    
    /* ===== 指令解码方法 ===== */
    
    OpCode DecodeOpCode(Instruction inst) const;
    RegisterIndex DecodeA(Instruction inst) const;
    int DecodeB(Instruction inst) const;
    int DecodeC(Instruction inst) const;
    int DecodeBx(Instruction inst) const;
    int DecodeSBx(Instruction inst) const;
    
    /* ===== 指令编码方法 ===== */
    
    Instruction EncodeABC(OpCode op, RegisterIndex a, int b, int c) const;
    Instruction EncodeABx(OpCode op, RegisterIndex a, int bx) const;
    Instruction EncodeAsBx(OpCode op, RegisterIndex a, int sbx) const;
};

} // namespace lua_cpp