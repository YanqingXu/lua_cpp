#include "LuaCore/Compiler.h"
#include "LuaCore/types.h"
#include <iostream>
#include <cassert>

namespace LuaCore {

void Compiler::compileStatement(Ptr<Statement> stmt) {
    if (auto assignStmt = std::dynamic_pointer_cast<AssignmentStmt>(stmt)) {
        compileAssignmentStmt(assignStmt);
    } else if (auto localVarStmt = std::dynamic_pointer_cast<LocalVarDeclStmt>(stmt)) {
        compileLocalVarDeclStmt(localVarStmt);
    } else if (auto funcCallStmt = std::dynamic_pointer_cast<FunctionCallStmt>(stmt)) {
        compileFunctionCallStmt(funcCallStmt);
    } else if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        compileIfStmt(ifStmt);
    } else if (auto whileStmt = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        compileWhileStmt(whileStmt);
    } else if (auto doStmt = std::dynamic_pointer_cast<DoStmt>(stmt)) {
        compileDoStmt(doStmt);
    } else if (auto numericForStmt = std::dynamic_pointer_cast<NumericForStmt>(stmt)) {
        compileForStmt(numericForStmt);
    } else if (auto genericForStmt = std::dynamic_pointer_cast<GenericForStmt>(stmt)) {
        compileGenericForStmt(genericForStmt);
    } else if (auto repeatStmt = std::dynamic_pointer_cast<RepeatStmt>(stmt)) {
        compileRepeatStmt(repeatStmt);
    } else if (auto funcDeclStmt = std::dynamic_pointer_cast<FunctionDeclStmt>(stmt)) {
        compileFunctionDeclStmt(funcDeclStmt);
    } else if (auto returnStmt = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        compileReturnStmt(returnStmt);
    } else if (auto breakStmt = std::dynamic_pointer_cast<BreakStmt>(stmt)) {
        compileBreakStmt(breakStmt);
    } else {
        // 未知语句类型
        std::cerr << "Unknown statement type!" << std::endl;
        assert(false && "Unknown statement type");
    }
}

void Compiler::compileBlock(Ptr<Block> block) {
    beginScope();
    
    for (const auto& stmt : block->getStatements()) {
        compileStatement(stmt);
    }
    
    endScope();
}

void Compiler::compileAssignmentStmt(Ptr<AssignmentStmt> stmt) {
    const auto& vars = stmt->getVariables();
    const auto& values = stmt->getValues();
    
    // 编译要赋值的表达式
    for (usize i = 0; i < values.size(); ++i) {
        compileExpression(values[i], m_current->localCount + i);
    }
    
    // 处理变量赋值
    for (usize i = 0; i < vars.size(); ++i) {
        i32 valueReg = m_current->localCount + (i < values.size() ? i : values.size() - 1);
        
        if (auto varExpr = std::dynamic_pointer_cast<VariableExpr>(vars[i])) {
            // 局部变量或全局变量
            const Str& name = varExpr->getName();
            
            // 查找局部变量
            i32 local = resolveLocal(m_current, name);
            if (local != -1) {
                // 如果是局部变量，直接赋值
                emitABC(OpCode::Move, local, valueReg, 0, 0);
                continue;
            }
            
            // 查找upvalue
            i32 upvalue = resolveUpvalue(m_current, name);
            if (upvalue != -1) {
                // 如果是upvalue，设置其值
                emitABC(OpCode::SetUpvalue, valueReg, upvalue, 0, 0);
                continue;
            }
            
            // 作为全局变量处理
            i32 nameIndex = addStringConstant(name);
            emitABC(OpCode::SetField, 0, nameIndex, valueReg, 0); // 假设寄存器0存放全局环境
        } else if (auto tableAccessExpr = std::dynamic_pointer_cast<TableAccessExpr>(vars[i])) {
            // 表访问赋值
            // 获取表
            compileExpression(tableAccessExpr->getTable(), m_current->localCount + values.size());
            // 获取键
            compileExpression(tableAccessExpr->getKey(), m_current->localCount + values.size() + 1);
            // 设置表项
            emitABC(OpCode::SetTable, m_current->localCount + values.size(), 
                   m_current->localCount + values.size() + 1, valueReg, 0);
        } else if (auto fieldAccessExpr = std::dynamic_pointer_cast<FieldAccessExpr>(vars[i])) {
            // 字段访问赋值
            // 获取表
            compileExpression(fieldAccessExpr->getTable(), m_current->localCount + values.size());
            // 添加字段名到常量表
            i32 fieldIndex = addStringConstant(fieldAccessExpr->getField());
            // 设置表字段
            emitABC(OpCode::SetField, m_current->localCount + values.size(), fieldIndex, valueReg, 0);
        } else {
            // 未知类型的变量表达式
            std::cerr << "Unknown variable expression type in assignment!" << std::endl;
            assert(false && "Unknown variable expression type in assignment");
        }
    }
}

void Compiler::compileLocalVarDeclStmt(Ptr<LocalVarDeclStmt> stmt) {
    const auto& names = stmt->getNames();
    const auto& initializers = stmt->getInitializers();
    
    // 编译初始化表达式
    for (usize i = 0; i < initializers.size(); ++i) {
        compileExpression(initializers[i], m_current->localCount + i);
    }
    
    // 声明局部变量
    for (usize i = 0; i < names.size(); ++i) {
        // 添加局部变量
        i32 localIndex = addLocal(names[i]);
        
        // 如果有初始化表达式，赋值给局部变量
        if (i < initializers.size()) {
            // 如果初始化表达式已经编译到正确位置，无需额外赋值
            if (m_current->localCount - 1 != m_current->localCount + i) {
                emitABC(OpCode::Move, localIndex, m_current->localCount + i, 0, 0);
            }
        } else {
            // 没有初始化表达式，赋nil值
            emitABC(OpCode::LoadNil, localIndex, 0, 0, 0);
        }
    }
}

void Compiler::compileFunctionCallStmt(Ptr<FunctionCallStmt> stmt) {
    // 编译函数调用表达式
    compileExpression(stmt->getCall(), m_current->localCount);
    // 丢弃返回值（作为语句，我们不需要保留返回值）
}

void Compiler::compileIfStmt(Ptr<IfStmt> stmt) {
    Vec<usize> endJumps;
    
    // 编译条件和块
    for (usize i = 0; i < stmt->getConditions().size(); ++i) {
        // 如果这是一个else-if分支（不是第一个分支），需要在这里放置一个标签，
        // 前一个分支判断失败时跳转到这里
        if (i > 0) {
            patchJump(endJumps.back(), m_current->proto->getCode().size());
            endJumps.pop_back();
        }
        
        // 编译条件表达式
        compileExpression(stmt->getConditions()[i], m_current->localCount);
        
        // 测试条件是否为假
        emitABC(OpCode::Test, m_current->localCount, 0, 0, 0);
        
        // 如果条件为假，跳过当前块
        usize skipJump = emitJump(OpCode::JumpIfFalse, 0);
        
        // 编译条件为真时执行的块
        compileBlock(stmt->getBlocks()[i]);
        
        // 块执行完后跳到if语句结束
        endJumps.push_back(emitJump(OpCode::Jump, 0));
        
        // 设置跳过当前块的跳转目标
        patchJump(skipJump, m_current->proto->getCode().size());
    }
    
    // 如果有else块，编译它
    if (stmt->getElseBlock() != nullptr) {
        compileBlock(stmt->getElseBlock());
    }
    
    // 设置所有结束跳转的目标
    for (auto jump : endJumps) {
        patchJump(jump, m_current->proto->getCode().size());
    }
}

void Compiler::compileWhileStmt(Ptr<WhileStmt> stmt) {
    // 保存循环开始位置
    usize loopStart = m_current->proto->getCode().size();
    
    // 编译条件表达式
    compileExpression(stmt->getCondition(), m_current->localCount);
    
    // 测试条件是否为假
    emitABC(OpCode::Test, m_current->localCount, 0, 0, 0);
    
    // 如果条件为假，跳出循环
    usize exitJump = emitJump(OpCode::JumpIfFalse, 0);
    
    // 编译循环体
    compileBlock(stmt->getBody());
    
    // 跳回循环开始
    emitASBx(OpCode::Jump, 0, loopStart - m_current->proto->getCode().size() - 1, 0);
    
    // 设置退出循环的跳转目标
    patchJump(exitJump, m_current->proto->getCode().size());
}

void Compiler::compileDoStmt(Ptr<DoStmt> stmt) {
    // 简单地编译块
    compileBlock(stmt->getBody());
}

void Compiler::compileForStmt(Ptr<NumericForStmt> stmt) {
    beginScope();
    
    // 编译初始值
    compileExpression(stmt->getStart(), m_current->localCount);
    
    // 编译终止值
    compileExpression(stmt->getEnd(), m_current->localCount + 1);
    
    // 编译步长（若有）
    if (stmt->getStep() != nullptr) {
        compileExpression(stmt->getStep(), m_current->localCount + 2);
    } else {
        // 默认步长为1
        i32 constIndex = addConstant(Value(1.0));
        emitABx(OpCode::LoadK, m_current->localCount + 2, constIndex, 0);
    }
    
    // 添加循环变量
    i32 varIndex = addLocal(stmt->getVarName());
    
    // 预处理循环（初始化）
    emitABC(OpCode::ForPrep, varIndex, 0, 0, 0);
    
    // 保存forprep指令的位置，稍后会修正跳转目标
    usize forPrepPos = m_current->proto->getCode().size() - 1;
    
    // 编译循环体
    compileBlock(stmt->getBody());
    
    // 循环递增和条件检查
    emitABC(OpCode::ForLoop, varIndex, 0, 0, 0);
    
    // 设置forprep的跳转目标（跳过循环体）
    patchJump(forPrepPos, m_current->proto->getCode().size());
    
    endScope();
}

void Compiler::compileGenericForStmt(Ptr<GenericForStmt> stmt) {
    beginScope();
    
    // 编译迭代器函数、状态和初始值
    compileExpression(stmt->getIteratorFunc(), m_current->localCount);     // 迭代器函数
    compileExpression(stmt->getState(), m_current->localCount + 1);        // 状态
    compileExpression(stmt->getControlVar(), m_current->localCount + 2);   // 控制变量（初始值）
    
    // 添加循环变量
    for (const auto& var : stmt->getVarNames()) {
        addLocal(var);
    }
    
    // 循环开始位置
    usize loopStart = m_current->proto->getCode().size();
    
    // 调用迭代器函数
    emitABC(OpCode::Call, m_current->localCount, 3, 1 + stmt->getVarNames().size(), 0);
    
    // 检查第一个返回值是否为nil（循环终止条件）
    emitABC(OpCode::Test, m_current->localCount + 3, 0, 0, 0);
    
    // 如果为nil，跳出循环
    usize exitJump = emitJump(OpCode::JumpIfTrue, 0);
    
    // 编译循环体
    compileBlock(stmt->getBody());
    
    // 跳回循环开始
    emitASBx(OpCode::Jump, 0, loopStart - m_current->proto->getCode().size() - 1, 0);
    
    // 设置退出循环的跳转目标
    patchJump(exitJump, m_current->proto->getCode().size());
    
    endScope();
}

void Compiler::compileRepeatStmt(Ptr<RepeatStmt> stmt) {
    // 保存循环开始位置
    usize loopStart = m_current->proto->getCode().size();
    
    beginScope();
    
    // 编译循环体
    compileBlock(stmt->getBody());
    
    // 编译条件表达式
    compileExpression(stmt->getCondition(), m_current->localCount);
    
    // 测试条件是否为真（如果为真，退出循环）
    emitABC(OpCode::Test, m_current->localCount, 0, 1, 0);
    
    // 如果条件为假，继续循环
    emitASBx(OpCode::JumpIfFalse, 0, loopStart - m_current->proto->getCode().size() - 1, 0);
    
    endScope();
}

void Compiler::compileFunctionDeclStmt(Ptr<FunctionDeclStmt> stmt) {
    // 创建函数原型
    auto proto = std::make_shared<FunctionProto>(stmt->getName(), stmt->getParams().size(), stmt->isVararg());
    
    // 保存当前编译状态
    CompileState* enclosingState = m_current;
    
    // 创建新的编译状态
    CompileState newState;
    newState.proto = proto;
    newState.enclosing = enclosingState;
    newState.scopeDepth = 0;
    newState.localCount = 0;
    newState.stackSize = 0;
    
    m_current = &newState;
    
    // 进入新作用域
    beginScope();
    
    // 添加参数作为局部变量
    for (const auto& param : stmt->getParams()) {
        addLocal(param);
    }
    
    // 编译函数体
    compileBlock(stmt->getBody());
    
    // 添加默认返回指令
    emitABC(OpCode::Return, 0, 1, 0, 0);
    
    // 结束作用域
    endScope();
    
    // 恢复之前的编译状态
    m_current = enclosingState;
    
    // 添加子函数原型
    enclosingState->proto->addProto(proto);
    
    // 获取函数名
    bool isMethod = stmt->isMethod();
    
    if (isMethod) {
        // 方法声明 (例如 table:method())
        if (auto fieldAccess = std::dynamic_pointer_cast<FieldAccessExpr>(stmt->getFuncName())) {
            // 编译表达式
            compileExpression(fieldAccess->getTable(), m_current->localCount);
            
            // 创建闭包
            emitABx(OpCode::Closure, m_current->localCount + 1, enclosingState->proto->getProtos().size() - 1, 0);
            
            // 处理upvalue
            for (const auto& upvalue : proto->getUpvalues()) {
                emitABC(upvalue.isLocal ? OpCode::Move : OpCode::GetUpvalue, 0, upvalue.index, 0, 0);
            }
            
            // 添加字段名到常量表
            i32 fieldIndex = addStringConstant(fieldAccess->getField());
            
            // 设置方法
            emitABC(OpCode::SetField, m_current->localCount, fieldIndex, m_current->localCount + 1, 0);
        } else {
            // 不支持其他类型的方法声明
            std::cerr << "Unsupported method declaration!" << std::endl;
            assert(false && "Unsupported method declaration");
        }
    } else {
        // 普通函数声明
        if (auto varExpr = std::dynamic_pointer_cast<VariableExpr>(stmt->getFuncName())) {
            const Str& name = varExpr->getName();
            
            // 创建闭包
            emitABx(OpCode::Closure, m_current->localCount, enclosingState->proto->getProtos().size() - 1, 0);
            
            // 处理upvalue
            for (const auto& upvalue : proto->getUpvalues()) {
                emitABC(upvalue.isLocal ? OpCode::Move : OpCode::GetUpvalue, 0, upvalue.index, 0, 0);
            }
            
            // 查找局部变量
            i32 local = resolveLocal(m_current, name);
            if (local != -1) {
                // 如果是局部变量，直接赋值
                emitABC(OpCode::Move, local, m_current->localCount, 0, 0);
            } else {
                // 查找upvalue
                i32 upvalue = resolveUpvalue(m_current, name);
                if (upvalue != -1) {
                    // 如果是upvalue，设置其值
                    emitABC(OpCode::SetUpvalue, m_current->localCount, upvalue, 0, 0);
                } else {
                    // 作为全局变量处理
                    i32 nameIndex = addStringConstant(name);
                    emitABC(OpCode::SetField, 0, nameIndex, m_current->localCount, 0);
                }
            }
        } else if (auto fieldAccess = std::dynamic_pointer_cast<FieldAccessExpr>(stmt->getFuncName())) {
            // 表字段赋值（例如 table.field = function）
            // 编译表达式
            compileExpression(fieldAccess->getTable(), m_current->localCount + 1);
            
            // 创建闭包
            emitABx(OpCode::Closure, m_current->localCount, enclosingState->proto->getProtos().size() - 1, 0);
            
            // 处理upvalue
            for (const auto& upvalue : proto->getUpvalues()) {
                emitABC(upvalue.isLocal ? OpCode::Move : OpCode::GetUpvalue, 0, upvalue.index, 0, 0);
            }
            
            // 添加字段名到常量表
            i32 fieldIndex = addStringConstant(fieldAccess->getField());
            
            // 设置表字段
            emitABC(OpCode::SetField, m_current->localCount + 1, fieldIndex, m_current->localCount, 0);
        } else {
            // 不支持其他类型的函数声明
            std::cerr << "Unsupported function declaration!" << std::endl;
            assert(false && "Unsupported function declaration");
        }
    }
}

void Compiler::compileReturnStmt(Ptr<ReturnStmt> stmt) {
    const auto& values = stmt->getValues();
    
    // 编译返回值
    for (usize i = 0; i < values.size(); ++i) {
        compileExpression(values[i], i);
    }
    
    // 生成返回指令
    emitABC(OpCode::Return, 0, values.empty() ? 1 : values.size() + 1, 0, 0);
}

void Compiler::compileBreakStmt(Ptr<BreakStmt> stmt) {
    // 生成跳转指令，暂时不设置跳转目标
    // 在完整实现中，我们需要维护一个跳转列表，并在循环结束时回填这些跳转
    emitASBx(OpCode::Jump, 0, 0, 0);
}

} // namespace LuaCore
