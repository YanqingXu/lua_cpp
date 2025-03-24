#include "include/Compiler.h"
#include "types.h"

#include <iostream>
#include <cassert>

namespace LuaCore {

// FunctionProto方法实现
usize FunctionProto::addConstant(const Value& value) {
    // 检查常量是否已存在
    for (usize i = 0; i < m_constants.size(); ++i) {
        if (m_constants[i] == value) {
            return i;
        }
    }
    
    // 添加新常量
    m_constants.push_back(value);
    return m_constants.size() - 1;
}

void FunctionProto::addLocalVar(const Str& name, i32 scopeDepth) {
    m_localVars.push_back({name, scopeDepth, false, static_cast<i32>(m_localVars.size())});
}

i32 FunctionProto::addUpvalue(u8 index, bool isLocal) {
    // 检查upvalue是否已存在
    for (usize i = 0; i < m_upvalues.size(); ++i) {
        const Upvalue& upvalue = m_upvalues[i];
        if (upvalue.index == index && upvalue.isLocal == isLocal) {
            return static_cast<i32>(i);
        }
    }
    
    // 添加新upvalue
    m_upvalues.push_back({index, isLocal});
    return m_upvalues.size() - 1;
}

usize FunctionProto::addInstruction(const Instruction& instruction) {
    m_code.push_back(instruction);
    return m_code.size() - 1;
}

void FunctionProto::addProto(Ptr<FunctionProto> proto) {
    m_protos.push_back(std::move(proto));
}

void FunctionProto::setLineInfo(usize instructionIndex, i32 line) {
    if (m_lineInfo.size() <= instructionIndex) {
        m_lineInfo.resize(instructionIndex + 1, 0);
    }
    m_lineInfo[instructionIndex] = line;
}

i32 FunctionProto::getLine(usize instructionIndex) const {
    if (instructionIndex < m_lineInfo.size()) {
        return m_lineInfo[instructionIndex];
    }
    return 0;
}

// Compiler方法实现
Ptr<FunctionProto> Compiler::compile(Ptr<Block> ast, const Str& source) {
    m_source = source;
    
    // 创建主函数原型
    auto mainProto = std::make_shared<FunctionProto>("main", 0, false);
    
    // 设置编译状态
    CompileState mainState;
    mainState.proto = mainProto;
    mainState.enclosing = nullptr;
    mainState.scopeDepth = 0;
    mainState.localCount = 0;
    mainState.stackSize = 0;
    
    m_current = &mainState;
    
    // 编译代码块
    compileBlock(ast);
    
    // 添加默认返回指令
    emitABC(OpCode::Return, 0, 1, 0, 0);
    
    return mainProto;
}

void Compiler::beginScope() {
    m_current->scopeDepth++;
}

void Compiler::endScope() {
    m_current->scopeDepth--;
    
    // 移除当前作用域的局部变量
    while (m_current->localCount > 0 &&
           m_current->proto->getLocalVars()[m_current->localCount - 1].scopeDepth > m_current->scopeDepth) {
        if (m_current->proto->getLocalVars()[m_current->localCount - 1].isCaptured) {
            // 如果变量被捕获，需要关闭upvalue
            emitABC(OpCode::Close, m_current->localCount - 1, 0, 0, 0);
        }
        m_current->localCount--;
    }
}

i32 Compiler::addLocal(const Str& name) {
    // 添加局部变量到当前函数原型
    m_current->proto->addLocalVar(name, m_current->scopeDepth);
    m_current->locals[name] = m_current->localCount;
    return m_current->localCount++;
}

i32 Compiler::resolveLocal(CompileState* state, const Str& name) {
    // 查找局部变量
    auto it = state->locals.find(name);
    if (it != state->locals.end()) {
        return it->second;
    }
    return -1; // 未找到
}

i32 Compiler::resolveUpvalue(CompileState* state, const Str& name) {
    if (state->enclosing == nullptr) {
        return -1; // 没有外层函数
    }
    
    // 尝试在外层函数中解析为局部变量
    i32 local = resolveLocal(state->enclosing, name);
    if (local != -1) {
        // 将外层函数的局部变量标记为被捕获
        state->enclosing->proto->getLocalVars()[local].isCaptured = true;
        return state->proto->addUpvalue(local, true);
    }
    
    // 尝试在外层函数的upvalue中解析
    i32 upvalue = resolveUpvalue(state->enclosing, name);
    if (upvalue != -1) {
        return state->proto->addUpvalue(upvalue, false);
    }
    
    return -1; // 未找到
}

usize Compiler::emitInstruction(const Instruction& instruction, i32 line) {
    usize index = m_current->proto->addInstruction(instruction);
    m_current->proto->setLineInfo(index, line);
    return index;
}

usize Compiler::emitABC(OpCode op, u8 a, u8 b, u8 c, i32 line) {
    return emitInstruction(Instruction::createABC(op, a, b, c), line);
}

usize Compiler::emitABx(OpCode op, u8 a, u16 bx, i32 line) {
    return emitInstruction(Instruction::createABx(op, a, bx), line);
}

usize Compiler::emitASBx(OpCode op, u8 a, i16 sbx, i32 line) {
    return emitInstruction(Instruction::createASBx(op, a, sbx), line);
}

usize Compiler::emitAx(OpCode op, u32 ax, i32 line) {
    return emitInstruction(Instruction::createAx(op, ax), line);
}

usize Compiler::emitJump(OpCode op, i32 line) {
    return emitASBx(op, 0, 0, line); // 占位，之后会修正跳转目标
}

void Compiler::patchJump(usize jumpInstr, usize target) {
    // 计算相对跳转偏移
    i32 offset = static_cast<i32>(target) - static_cast<i32>(jumpInstr) - 1;
    
    // 修改指令中的sBx字段
    Instruction& instr = m_current->proto->getCode()[jumpInstr];
    instr = Instruction::createASBx(
        instr.getOpCode(),
        instr.getA(),
        offset
    );
}

i32 Compiler::addConstant(const Value& value) {
    return m_current->proto->addConstant(value);
}

i32 Compiler::addStringConstant(const Str& str) {
    return addConstant(Value(str));
}

// 表达式编译方法实现会在Compiler_Expressions.cpp中完成
// 语句编译方法实现会在Compiler_Statements.cpp中完成

} // namespace LuaCore
