/**
 * @file bytecode_generator.h
 * @brief 字节码生成器头文件
 * @description 提供字节码指令生成和管理功能
 * @author Lua C++ Project
 * @date 2025-09-25
 */

#pragma once

#include "bytecode.h"
#include "../core/lua_common.h"
#include <vector>
#include <string>

namespace lua_cpp {

/* ========================================================================== */
/* 字节码生成器类 */
/* ========================================================================== */

/**
 * @brief 字节码生成器
 * @description 负责生成和管理字节码指令序列
 */
class BytecodeGenerator {
public:
    /**
     * @brief 构造函数
     */
    BytecodeGenerator();
    
    /**
     * @brief 析构函数
     */
    ~BytecodeGenerator();
    
    // 禁用拷贝，允许移动
    BytecodeGenerator(const BytecodeGenerator&) = delete;
    BytecodeGenerator& operator=(const BytecodeGenerator&) = delete;
    BytecodeGenerator(BytecodeGenerator&&) = default;
    BytecodeGenerator& operator=(BytecodeGenerator&&) = default;
    
    /* ====================================================================== */
    /* 指令生成接口 */
    /* ====================================================================== */
    
    /**
     * @brief 发射指令
     * @param instruction 指令
     * @param line 源代码行号
     * @return 指令在代码中的位置
     */
    Size EmitInstruction(Instruction instruction, int line = 0);
    
    /**
     * @brief 发射ABC格式指令
     * @param op 操作码
     * @param a A字段
     * @param b B字段
     * @param c C字段
     * @param line 源代码行号
     * @return 指令位置
     */
    Size EmitABC(OpCode op, int a, int b, int c, int line = 0);
    
    /**
     * @brief 发射ABx格式指令
     * @param op 操作码
     * @param a A字段
     * @param bx Bx字段
     * @param line 源代码行号
     * @return 指令位置
     */
    Size EmitABx(OpCode op, int a, int bx, int line = 0);
    
    /**
     * @brief 发射AsBx格式指令
     * @param op 操作码
     * @param a A字段
     * @param sbx sBx字段
     * @param line 源代码行号
     * @return 指令位置
     */
    Size EmitAsBx(OpCode op, int a, int sbx, int line = 0);
    
    /* ====================================================================== */
    /* 跳转管理 */
    /* ====================================================================== */
    
    /**
     * @brief 发射跳转指令占位符
     * @param op 跳转指令操作码
     * @param a A字段
     * @return 跳转指令位置
     */
    Size EmitJump(OpCode op = OpCode::JMP, int a = 0);
    
    /**
     * @brief 修复跳转指令
     * @param pc 跳转指令位置
     * @param target 跳转目标位置
     */
    void PatchJump(Size pc, Size target);
    
    /**
     * @brief 修复跳转指令到当前位置
     * @param pc 跳转指令位置
     */
    void PatchJumpToHere(Size pc);
    
    /**
     * @brief 检查是否为有效跳转目标
     * @param pc 目标位置
     * @return 是否有效
     */
    bool IsValidJumpTarget(Size pc) const;
    
    /* ====================================================================== */
    /* 代码信息 */
    /* ====================================================================== */
    
    /**
     * @brief 获取当前程序计数器
     * @return 当前PC值
     */
    Size GetCurrentPC() const;
    
    /**
     * @brief 设置当前行号
     * @param line 行号
     */
    void SetCurrentLine(int line);
    
    /**
     * @brief 获取当前行号
     * @return 当前行号
     */
    int GetCurrentLine() const;
    
    /**
     * @brief 获取指令序列
     * @return 指令序列的引用
     */
    const std::vector<Instruction>& GetInstructions() const;
    
    /**
     * @brief 获取行号信息
     * @return 行号信息的引用
     */
    const std::vector<int>& GetLineInfo() const;
    
    /* ====================================================================== */
    /* 指令操作 */
    /* ====================================================================== */
    
    /**
     * @brief 预分配指令空间
     * @param count 指令数量
     */
    void ReserveInstructions(Size count);
    
    /**
     * @brief 获取指定位置的指令
     * @param pc 程序计数器
     * @return 指令
     */
    Instruction GetInstruction(Size pc) const;
    
    /**
     * @brief 设置指定位置的指令
     * @param pc 程序计数器
     * @param instruction 新指令
     */
    void SetInstruction(Size pc, Instruction instruction);
    
    /**
     * @brief 将指令转换为字符串表示
     * @param inst 指令
     * @return 字符串表示
     */
    std::string InstructionToString(Instruction inst) const;
    
    /**
     * @brief 清空生成器状态
     */
    void Clear();

private:
    std::vector<Instruction> instructions_;     // 指令序列
    std::vector<int> line_info_;               // 行号信息
    int current_line_;                         // 当前行号
};

/* ========================================================================== */
/* 指令发射器类 */
/* ========================================================================== */

/**
 * @brief 指令发射器
 * @description 提供语义化的指令发射接口
 */
class InstructionEmitter {
public:
    /**
     * @brief 构造函数
     * @param generator 字节码生成器引用
     */
    explicit InstructionEmitter(BytecodeGenerator& generator);
    
    /* ====================================================================== */
    /* 数据移动指令 */
    /* ====================================================================== */
    
    /**
     * @brief 发射MOVE指令
     * @param dst 目标寄存器
     * @param src 源寄存器
     * @return 指令位置
     */
    Size EmitMove(RegisterIndex dst, RegisterIndex src);
    
    /**
     * @brief 发射LOADK指令
     * @param dst 目标寄存器
     * @param constant_index 常量索引
     * @return 指令位置
     */
    Size EmitLoadK(RegisterIndex dst, int constant_index);
    
    /**
     * @brief 发射LOADBOOL指令
     * @param dst 目标寄存器
     * @param value 布尔值
     * @param skip 是否跳过下一条指令
     * @return 指令位置
     */
    Size EmitLoadBool(RegisterIndex dst, bool value, bool skip = false);
    
    /**
     * @brief 发射LOADNIL指令
     * @param start 起始寄存器
     * @param end 结束寄存器
     * @return 指令位置
     */
    Size EmitLoadNil(RegisterIndex start, RegisterIndex end = 0);
    
    /* ====================================================================== */
    /* 全局变量指令 */
    /* ====================================================================== */
    
    /**
     * @brief 发射GETGLOBAL指令
     * @param dst 目标寄存器
     * @param name_index 变量名常量索引
     * @return 指令位置
     */
    Size EmitGetGlobal(RegisterIndex dst, int name_index);
    
    /**
     * @brief 发射SETGLOBAL指令
     * @param src 源寄存器
     * @param name_index 变量名常量索引
     * @return 指令位置
     */
    Size EmitSetGlobal(RegisterIndex src, int name_index);
    
    /* ====================================================================== */
    /* 表操作指令 */
    /* ====================================================================== */
    
    /**
     * @brief 发射GETTABLE指令
     * @param dst 目标寄存器
     * @param table 表寄存器
     * @param key_rk 键的RK值
     * @return 指令位置
     */
    Size EmitGetTable(RegisterIndex dst, RegisterIndex table, int key_rk);
    
    /**
     * @brief 发射SETTABLE指令
     * @param table 表寄存器
     * @param key_rk 键的RK值
     * @param value_rk 值的RK值
     * @return 指令位置
     */
    Size EmitSetTable(RegisterIndex table, int key_rk, int value_rk);
    
    /**
     * @brief 发射NEWTABLE指令
     * @param dst 目标寄存器
     * @param array_size 数组部分大小
     * @param hash_size 哈希部分大小
     * @return 指令位置
     */
    Size EmitNewTable(RegisterIndex dst, int array_size = 0, int hash_size = 0);
    
    /* ====================================================================== */
    /* 算术运算指令 */
    /* ====================================================================== */
    
    /**
     * @brief 发射ADD指令
     * @param dst 目标寄存器
     * @param left_rk 左操作数RK值
     * @param right_rk 右操作数RK值
     * @return 指令位置
     */
    Size EmitAdd(RegisterIndex dst, int left_rk, int right_rk);
    
    /**
     * @brief 发射SUB指令
     */
    Size EmitSub(RegisterIndex dst, int left_rk, int right_rk);
    
    /**
     * @brief 发射MUL指令
     */
    Size EmitMul(RegisterIndex dst, int left_rk, int right_rk);
    
    /**
     * @brief 发射DIV指令
     */
    Size EmitDiv(RegisterIndex dst, int left_rk, int right_rk);
    
    /**
     * @brief 发射MOD指令
     */
    Size EmitMod(RegisterIndex dst, int left_rk, int right_rk);
    
    /**
     * @brief 发射POW指令
     */
    Size EmitPow(RegisterIndex dst, int left_rk, int right_rk);
    
    /* ====================================================================== */
    /* 一元运算指令 */
    /* ====================================================================== */
    
    /**
     * @brief 发射UNM指令（一元减号）
     * @param dst 目标寄存器
     * @param src 源寄存器
     * @return 指令位置
     */
    Size EmitUnm(RegisterIndex dst, RegisterIndex src);
    
    /**
     * @brief 发射NOT指令
     * @param dst 目标寄存器
     * @param src 源寄存器
     * @return 指令位置
     */
    Size EmitNot(RegisterIndex dst, RegisterIndex src);
    
    /**
     * @brief 发射LEN指令（长度运算）
     * @param dst 目标寄存器
     * @param src 源寄存器
     * @return 指令位置
     */
    Size EmitLen(RegisterIndex dst, RegisterIndex src);
    
    /* ====================================================================== */
    /* 字符串连接指令 */
    /* ====================================================================== */
    
    /**
     * @brief 发射CONCAT指令
     * @param dst 目标寄存器
     * @param start 起始寄存器
     * @param end 结束寄存器
     * @return 指令位置
     */
    Size EmitConcat(RegisterIndex dst, RegisterIndex start, RegisterIndex end);
    
    /* ====================================================================== */
    /* 跳转指令 */
    /* ====================================================================== */
    
    /**
     * @brief 发射JMP指令
     * @param offset 跳转偏移
     * @return 指令位置
     */
    Size EmitJump(int offset = 0);
    
    /**
     * @brief 发射跳转占位符
     * @return 指令位置
     */
    Size EmitJumpPlaceholder();
    
    /* ====================================================================== */
    /* 比较指令 */
    /* ====================================================================== */
    
    /**
     * @brief 发射EQ指令
     * @param invert 是否反转结果
     * @param left_rk 左操作数RK值
     * @param right_rk 右操作数RK值
     * @return 指令位置
     */
    Size EmitEq(bool invert, int left_rk, int right_rk);
    
    /**
     * @brief 发射LT指令
     */
    Size EmitLt(bool invert, int left_rk, int right_rk);
    
    /**
     * @brief 发射LE指令
     */
    Size EmitLe(bool invert, int left_rk, int right_rk);
    
    /* ====================================================================== */
    /* 测试指令 */
    /* ====================================================================== */
    
    /**
     * @brief 发射TEST指令
     * @param condition 条件寄存器
     * @param invert 是否反转条件
     * @return 指令位置
     */
    Size EmitTest(RegisterIndex condition, bool invert = false);
    
    /**
     * @brief 发射TESTSET指令
     * @param dst 目标寄存器
     * @param condition 条件寄存器
     * @param invert 是否反转条件
     * @return 指令位置
     */
    Size EmitTestSet(RegisterIndex dst, RegisterIndex condition, bool invert = false);
    
    /* ====================================================================== */
    /* 函数调用指令 */
    /* ====================================================================== */
    
    /**
     * @brief 发射CALL指令
     * @param func 函数寄存器
     * @param num_args 参数数量（包含函数本身）
     * @param num_results 返回值数量
     * @return 指令位置
     */
    Size EmitCall(RegisterIndex func, int num_args, int num_results);
    
    /**
     * @brief 发射TAILCALL指令
     * @param func 函数寄存器
     * @param num_args 参数数量（包含函数本身）
     * @return 指令位置
     */
    Size EmitTailCall(RegisterIndex func, int num_args);
    
    /**
     * @brief 发射RETURN指令
     * @param start 起始寄存器
     * @param count 返回值数量
     * @return 指令位置
     */
    Size EmitReturn(RegisterIndex start = 0, int count = 0);
    
    /* ====================================================================== */
    /* 循环指令 */
    /* ====================================================================== */
    
    /**
     * @brief 发射FORPREP指令
     * @param base 基础寄存器
     * @param jump_offset 跳转偏移
     * @return 指令位置
     */
    Size EmitForPrep(RegisterIndex base, int jump_offset);
    
    /**
     * @brief 发射FORLOOP指令
     * @param base 基础寄存器
     * @param jump_offset 跳转偏移
     * @return 指令位置
     */
    Size EmitForLoop(RegisterIndex base, int jump_offset);
    
    /**
     * @brief 发射TFORLOOP指令（泛型for循环）
     * @param base 基础寄存器
     * @param jump_offset 跳转偏移
     * @return 指令位置
     */
    Size EmitTForLoop(RegisterIndex base, int jump_offset);
    
    /* ====================================================================== */
    /* 表设置指令 */
    /* ====================================================================== */
    
    /**
     * @brief 发射SETLIST指令
     * @param table 表寄存器
     * @param batch 批次
     * @param count 元素数量
     * @return 指令位置
     */
    Size EmitSetList(RegisterIndex table, int batch, int count);
    
    /* ====================================================================== */
    /* 闭包指令 */
    /* ====================================================================== */
    
    /**
     * @brief 发射CLOSE指令
     * @param start 起始寄存器
     * @return 指令位置
     */
    Size EmitClose(RegisterIndex start);
    
    /**
     * @brief 发射CLOSURE指令
     * @param dst 目标寄存器
     * @param proto_index 函数原型索引
     * @return 指令位置
     */
    Size EmitClosure(RegisterIndex dst, int proto_index);
    
    /* ====================================================================== */
    /* 可变参数指令 */
    /* ====================================================================== */
    
    /**
     * @brief 发射VARARG指令
     * @param dst 目标寄存器
     * @param count 参数数量
     * @return 指令位置
     */
    Size EmitVararg(RegisterIndex dst, int count);
    
    /* ====================================================================== */
    /* Upvalue指令 */
    /* ====================================================================== */
    
    /**
     * @brief 发射GETUPVAL指令
     * @param dst 目标寄存器
     * @param upval_index Upvalue索引
     * @return 指令位置
     */
    Size EmitGetUpval(RegisterIndex dst, int upval_index);
    
    /**
     * @brief 发射SETUPVAL指令
     * @param src 源寄存器
     * @param upval_index Upvalue索引
     * @return 指令位置
     */
    Size EmitSetUpval(RegisterIndex src, int upval_index);

private:
    BytecodeGenerator& generator_;
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
 * @brief 检查常量索引是否有效
 * @param index 常量索引
 * @return 是否有效
 */
bool IsValidConstantIndex(int index);

/**
 * @brief 检查RK值是否有效
 * @param rk RK值
 * @return 是否有效
 */
bool IsValidRK(int rk);

/**
 * @brief 将寄存器编码为RK值
 * @param reg 寄存器索引
 * @return RK值
 */
int EncodeRK(RegisterIndex reg);

/**
 * @brief 将常量索引编码为RK值
 * @param constant_index 常量索引
 * @return RK值
 */
int EncodeRK(int constant_index);

/**
 * @brief 解码指令为字符串表示
 * @param inst 指令
 * @return 字符串表示
 */
std::string DecodeInstruction(Instruction inst);

} // namespace lua_cpp