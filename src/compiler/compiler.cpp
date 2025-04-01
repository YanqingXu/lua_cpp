#include "compiler.hpp"
#include "types.hpp"
#include "compiler/ast.hpp"
#include "vm/function_proto.hpp"
#include "vm/instruction.hpp"

#include <iostream>
#include <cassert>

namespace Lua {

// Compiler方法实现
Ptr<FunctionProto> Compiler::compile(Ptr<Block> ast, const Str& source) {
    // 创建编译状态
    CompileState state;
    state.proto = make_ptr<FunctionProto>();
    state.scopeDepth = 0;
    state.localCount = 1; // 第一个局部变量总是环境
    state.stackSize = 1;
    state.locals["_ENV"] = 0; // 第一个局部变量总是环境
    
    // 保存源代码
    m_source = source;
    
    // 保存当前编译状态
    m_current = &state;
    
    // 开始编译
    beginScope();
    
    // 编译代码块
    compileBlock(ast);
    
    // 确保有返回指令
    if (!m_lastInstructionWasReturn) {
        emitABC(OpCode::Return, 0, 1, 0, 0); // 返回0个结果
    }
    
    endScope();
    
    // 清理编译状态
    m_current = nullptr;
    
    return state.proto;
}

void Compiler::beginScope() {
    m_current->scopeDepth++;
}

void Compiler::endScope() {
    m_current->scopeDepth--;
    
    // 关闭当前作用域中的局部变量
    // 这里应该遍历所有局部变量，关闭超出作用域的变量
    // 简化实现，实际应该维护一个有序的局部变量列表
    auto it = m_current->locals.begin();
    while (it != m_current->locals.end()) {
        if (it->second > m_current->scopeDepth) {
            // 变量超出作用域，从locals中移除
            it = m_current->locals.erase(it);
        } else {
            ++it;
        }
    }
    
    // 更新局部变量计数和栈大小
    m_current->localCount = static_cast<i32>(m_current->locals.size());
    m_current->stackSize = m_current->localCount;
}

i32 Compiler::addLocal(const Str& name) {
    if (m_current->localCount >= 255) {
        // Lua限制局部变量数量为255
        throw std::runtime_error("局部变量数量超过限制");
    }
    
    // 添加局部变量
    i32 index = m_current->localCount++;
    m_current->locals[name] = index;
    m_current->stackSize = std::max(m_current->stackSize, m_current->localCount);
    
    return index;
}

i32 Compiler::resolveLocal(CompileState* state, const Str& name) {
    // 查找局部变量
    auto it = state->locals.find(name);
    if (it != state->locals.end()) {
        return it->second;
    }
    
    // 未找到局部变量
    return -1;
}

i32 Compiler::resolveUpvalue(CompileState* state, const Str& name) {
    if (state->enclosing == nullptr) {
        // 没有外部函数
        return -1;
    }
    
    // 尝试在外部函数的局部变量中查找
    i32 local = resolveLocal(state->enclosing, name);
    if (local != -1) {
        // 标记局部变量被捕获（这里简化处理，实际应该更新局部变量表）
        
        // 添加upvalue
        Upvalue upvalue;
        upvalue.index = local;
        upvalue.isLocal = true;
        
        // 查找是否已存在相同的upvalue
        for (usize i = 0; i < state->upvalues.size(); ++i) {
            const Upvalue& existing = state->upvalues[i];
            if (existing.index == upvalue.index && existing.isLocal == upvalue.isLocal) {
                return static_cast<i32>(i);
            }
        }
        
        // 添加新upvalue
        state->upvalues.push_back(upvalue);
        return static_cast<i32>(state->upvalues.size() - 1);
    }
    
    // 尝试在外部函数的upvalue中查找
    i32 upvalue = resolveUpvalue(state->enclosing, name);
    if (upvalue != -1) {
        // 添加upvalue
        Upvalue newUpvalue;
        newUpvalue.index = upvalue;
        newUpvalue.isLocal = false;
        
        // 查找是否已存在相同的upvalue
        for (usize i = 0; i < state->upvalues.size(); ++i) {
            const Upvalue& existing = state->upvalues[i];
            if (existing.index == newUpvalue.index && existing.isLocal == newUpvalue.isLocal) {
                return static_cast<i32>(i);
            }
        }
        
        // 添加新upvalue
        state->upvalues.push_back(newUpvalue);
        return static_cast<i32>(state->upvalues.size() - 1);
    }
    
    // 未找到upvalue
    return -1;
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
    return emitASBx(op, 0, 0x7FFF, line); // 0x7FFF是一个无效的偏移量，稍后会被修补
}

void Compiler::patchJump(usize jumpInstr, usize target) {
    // 计算跳转偏移量
    i32 offset = static_cast<i32>(target) - static_cast<i32>(jumpInstr + 1);
    if (offset > 0x7FFF || offset < -0x7FFF) {
        throw ::std::runtime_error("跳转偏移量过大");
    }
    
    // 修补跳转指令
    Instruction& instr = m_current->proto->getCode()[jumpInstr];
    i16 sbx = static_cast<i16>(offset);
    instr.setASBx(instr.getOpCode(), instr.getA(), sbx);
}

i32 Compiler::addConstant(const Object::Value& value) {
    return static_cast<i32>(m_current->proto->addConstant(value));
}

i32 Compiler::addStringConstant(const Str& str) {
    return addConstant(Object::Value::string(str));
}

void Compiler::compileBlock(Ptr<Block> block) {
    // 编译代码块中的每条语句
    for (const auto& stmt : block->getStatements()) {
        compileStatement(stmt);
    }
}

// 表达式和语句的编译方法实现在对应的cpp文件中

} // namespace Lua
