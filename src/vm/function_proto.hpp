#pragma once

#include "types.hpp"
#include "object/value.hpp"
#include "instruction.hpp"

namespace Lua {

/**
 * @brief 表示编译过程中的局部变量
 */
struct LocalVar {
    Str name;      // 变量名
    i32 scopeDepth; // 定义该变量的作用域深度
    bool isCaptured;       // 是否被内层函数捕获
    i32 slot;      // 变量在栈上的位置
};

/**
 * @brief 表示Upvalue（被内部函数捕获的外部变量）
 */
struct Upvalue {
    u8 index;     // 变量在外层函数局部变量表或upvalue表中的索引
    bool isLocal;         // 是否是直接外层函数的局部变量
};

/**
 * @brief 表示一个函数原型
 */
class FunctionProto {
public:
    FunctionProto(const Str& name, i32 numParams, bool isVararg)
        : m_name(name), m_numParams(numParams), m_isVararg(isVararg) {}

    // 添加常量到常量表
    usize addConstant(const Object::Value& value) {
        m_constants.push_back(value);
        return m_constants.size() - 1;
    }

    // 获取常量表
    const Vec<Object::Value>& getConstants() const { return m_constants; }

    // 添加局部变量
    void addLocalVar(const Str& name, i32 scopeDepth) {
        LocalVar var;
        var.name = name;
        var.scopeDepth = scopeDepth;
        var.isCaptured = false;
        var.slot = static_cast<i32>(m_localVars.size());
        m_localVars.push_back(var);
    }

    // 添加upvalue
    i32 addUpvalue(u8 index, bool isLocal) {
        // 检查是否已存在该upvalue
        for (usize i = 0; i < m_upvalues.size(); i++) {
            if (m_upvalues[i].index == index && m_upvalues[i].isLocal == isLocal) {
                return static_cast<i32>(i);
            }
        }

        Upvalue upvalue;
        upvalue.index = index;
        upvalue.isLocal = isLocal;
        m_upvalues.push_back(upvalue);
        return static_cast<i32>(m_upvalues.size() - 1);
    }

    // 添加指令
    usize addInstruction(const Instruction& instruction) {
        m_code.push_back(instruction);
        return m_code.size() - 1;
    }

    // 获取指令
    Vec<Instruction>& getCode() { return m_code; }
    const Vec<Instruction>& getCode() const { return m_code; }

    // 添加子函数原型
    void addProto(Ptr<FunctionProto> proto) {
        m_protos.push_back(move(proto));
    }

    // 获取子函数原型
    const Vec<Ptr<FunctionProto>>& getProtos() const { return m_protos; }

    // 设置行号信息
    void setLineInfo(usize instructionIndex, i32 line) {
        if (instructionIndex >= m_lineInfo.size()) {
            m_lineInfo.resize(instructionIndex + 1, 0);
        }
        m_lineInfo[instructionIndex] = line;
    }

    // 获取行号信息
    i32 getLine(usize instructionIndex) const {
        return instructionIndex < m_lineInfo.size() ? m_lineInfo[instructionIndex] : 0;
    }

    // 获取函数原型的基本信息
    const Str& getName() const { return m_name; }
    i32 getNumParams() const { return m_numParams; }
    bool isVararg() const { return m_isVararg; }

    // 获取局部变量表和upvalue表
    Vec<LocalVar>& getLocalVars() { return m_localVars; }
    const Vec<LocalVar>& getLocalVars() const { return m_localVars; }
    const Vec<Upvalue>& getUpvalues() const { return m_upvalues; }

    i32 getMaxStackSize() const { return m_maxStackSize; }
    void setMaxStackSize(i32 size) { m_maxStackSize = size; }

private:
    Str m_name;                        // 函数名
    i32 m_numParams;                   // 参数数量
    bool m_isVararg;                   // 是否支持变长参数
    Vec<Object::Value> m_constants;    // 常量表
    Vec<Instruction> m_code;           // 指令列表
    Vec<i32> m_lineInfo;               // 行号信息
    Vec<LocalVar> m_localVars;         // 局部变量表
    Vec<Upvalue> m_upvalues;           // upvalue表
    Vec<Ptr<FunctionProto>> m_protos;  // 子函数原型
    i32 m_maxStackSize = 0;            // 最大栈大小
};

} // namespace Lua
