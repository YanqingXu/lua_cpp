#pragma once

#include "types.hpp"

namespace Lua {

/**
 * @brief Lua字节码操作码
 */
enum class OpCode : u8 {
    // 加载常量和基本值
    LoadNil,        // 加载nil值到寄存器
    LoadTrue,       // 加载true值到寄存器
    LoadFalse,      // 加载false值到寄存器
    LoadK,          // 从常量表加载常量到寄存器
    
    // 表操作
    NewTable,       // 创建新表
    GetTable,       // 从表中获取值 (R(A) := R(B)[R(C)])
    SetTable,       // 设置表的值 (R(A)[R(B)] := R(C))
    GetField,       // 从表中获取字段 (R(A) := R(B)[K(C)])
    SetField,       // 设置表字段  (R(A)[K(B)] := R(C))
    
    // 算术和比较操作
    Add,            // R(A) := R(B) + R(C)
    Sub,            // R(A) := R(B) - R(C)
    Mul,            // R(A) := R(B) * R(C)
    Div,            // R(A) := R(B) / R(C)
    Mod,            // R(A) := R(B) % R(C)
    Pow,            // R(A) := R(B) ^ R(C)
    Concat,         // R(A) := R(B)..R(C)
    
    Eq,             // if ((R(B) == R(C)) ~= A) pc++
    Lt,             // if ((R(B) <  R(C)) ~= A) pc++
    Le,             // if ((R(B) <= R(C)) ~= A) pc++
    
    // 逻辑操作
    Not,            // R(A) := not R(B)
    Test,           // if (R(A) <=> C) pc++
    TestSet,        // if (R(B) <=> C) R(A) := R(B) else pc++
    
    // 控制流
    Jump,           // pc += sBx
    JumpIfFalse,    // 如果条件为假则跳转 R(A) ? nothing : pc += sBx
    JumpIfTrue,     // 如果条件为真则跳转 R(A) ? pc += sBx : nothing
    Call,           // 调用函数 R(A)(R(A+1), ..., R(A+B-1))
    TailCall,       // 尾调用优化 R(A)(R(A+1), ..., R(A+B-1))
    Return,         // 从函数返回 (A-1)个值
    
    // 局部变量和Upvalue
    Move,           // R(A) := R(B)
    GetUpval,       // R(A) := UpValue[B]
    SetUpval,       // UpValue[B] := R(A)
    GetGlobal,      // R(A) := _ENV[K(Bx)]
    SetGlobal,      // _ENV[K(Bx)] := R(A)
    
    // 闭包操作
    Closure,        // R(A) := 创建闭包(函数原型Bx)
    Close,          // 关闭所有到寄存器A为止的upvalue
    
    // 杂项
    Len,            // R(A) := length of R(B)
    Self,           // R(A+1) := R(B); R(A) := R(B)[RK(C)]
    Pop,            // 弹出A个值
    
    // 迭代器
    ForPrep,        // R(A) -= R(A+2); pc += sBx
    ForLoop,        // R(A) += R(A+2); if R(A) <?= R(A+1) { pc += sBx; R(A+3) = R(A) }
    
    // 泛型for迭代器
    TForCall,       // 准备迭代器函数调用
    TForLoop,       // 执行泛型for循环
};

/**
 * @brief 字节码指令结构
 */
struct Instruction {
    u32 code;
    // 32位指令编码
    // 指令编码格式:
    // +---+---+---+---+
    // |  B  |  C  |  A  |
    // +---+---+---+---+
    // |  Bx   |  A  |
    // +---+---+---+---+
    // |   sBx   |  A  |
    // +---+---+---+---+
    // |      Ax       |
    // +---+---+---+---+

    // 获取不同字段的方法
    u8 getA() const { return (code >> 6) & 0xFF; }
    u8 getB() const { return (code >> 23) & 0x1FF; }
    u8 getC() const { return (code >> 14) & 0x1FF; }
    u16 getBx() const { return (code >> 14) & 0x3FFFF; }
    i16 getSBx() const { return static_cast<i16>(getBx()) - 131071; } // 有符号版本, max_unsigned/2
    u32 getAx() const { return code >> 6; }

    // 创建不同格式指令的静态方法
    static Instruction create(OpCode op, u8 a, u8 b, u8 c) {
        return { static_cast<u32>(static_cast<u8>(op) | (a << 6) | (b << 23) | (c << 14)) };
    }
    
    static Instruction createABC(OpCode op, u8 a, u8 b, u8 c) {
        return create(op, a, b, c);
    }
    
    static Instruction createABx(OpCode op, u8 a, u16 bx) {
        return { static_cast<u32>(static_cast<u8>(op) | (a << 6) | (bx << 14)) };
    }
    
    static Instruction createASBx(OpCode op, u8 a, i16 sbx) {
        return createABx(op, a, static_cast<u16>(sbx + 131071)); // 转换为无符号值
    }
    
    static Instruction createAx(OpCode op, u32 ax) {
        return { static_cast<u32>(static_cast<u8>(op) | (ax << 6)) };
    }
    
    OpCode getOpCode() const {
        return static_cast<OpCode>(code & 0x3F); // 低6位是操作码
    }
};

} // namespace Lua
