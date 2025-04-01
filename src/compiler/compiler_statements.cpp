#include "compiler.hpp"
#include "types.hpp"
#include "vm/instruction.hpp"

#include <iostream>
#include <cassert>

namespace Lua {

void Compiler::compileStatement(Ptr<Statement> stmt) {
    if (auto assignStmt = ::std::dynamic_pointer_cast<AssignmentStmt>(stmt)) {
        compileAssignmentStmt(assignStmt);
    } else if (auto localVarStmt = ::std::dynamic_pointer_cast<LocalVarDeclStmt>(stmt)) {
        compileLocalVarDeclStmt(localVarStmt);
    } else if (auto funcCallStmt = ::std::dynamic_pointer_cast<FunctionCallStmt>(stmt)) {
        compileFunctionCallStmt(funcCallStmt);
    } else if (auto ifStmt = ::std::dynamic_pointer_cast<IfStmt>(stmt)) {
        compileIfStmt(ifStmt);
    } else if (auto whileStmt = ::std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        compileWhileStmt(whileStmt);
    } else if (auto doStmt = ::std::dynamic_pointer_cast<DoStmt>(stmt)) {
        compileDoStmt(doStmt);
    } else if (auto numericForStmt = ::std::dynamic_pointer_cast<NumericForStmt>(stmt)) {
        compileForStmt(numericForStmt);
    } else if (auto genericForStmt = ::std::dynamic_pointer_cast<GenericForStmt>(stmt)) {
        compileGenericForStmt(genericForStmt);
    } else if (auto repeatStmt = ::std::dynamic_pointer_cast<RepeatStmt>(stmt)) {
        compileRepeatStmt(repeatStmt);
    } else if (auto funcDeclStmt = ::std::dynamic_pointer_cast<FunctionDeclStmt>(stmt)) {
        compileFunctionDeclStmt(funcDeclStmt);
    } else if (auto returnStmt = ::std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        compileReturnStmt(returnStmt);
    } else if (auto breakStmt = ::std::dynamic_pointer_cast<BreakStmt>(stmt)) {
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
    const auto& exprs = stmt->getExpressions();
    
    // 声明临时变量
    i32 tempRegs = 0;
    i32 varCount = static_cast<i32>(vars.size());
    i32 exprCount = static_cast<i32>(exprs.size());
    
    // 计算表达式
    for (i32 i = 0; i < exprCount - 1; ++i) {
        compileExpression(exprs[i], tempRegs + i);
    }
    
    // 最后一个表达式可能产生多个返回值
    if (exprCount > 0) {
        if (exprCount < varCount && isFunctionCall(exprs.back())) {
            // 如果最后一个表达式是函数调用，并且变量比表达式多，
            // 那么我们需要捕获所有返回值
            i32 lastReg = tempRegs + exprCount - 1;
            compileFunctionCallForMultipleReturns(::std::dynamic_pointer_cast<FunctionCallExpr>(exprs.back()), lastReg, varCount - exprCount + 1);
        } else {
            // 否则正常编译最后一个表达式
            compileExpression(exprs.back(), tempRegs + exprCount - 1);
        }
    }
    
    // 赋值
    for (i32 i = 0; i < varCount; ++i) {
        i32 valueReg = (i < exprCount) ? tempRegs + i : -1; // -1 表示nil值
        
        if (auto varExpr = ::std::dynamic_pointer_cast<VariableExpr>(vars[i])) {
            // 变量赋值
            const Str& name = varExpr->getName();
            
            // 尝试解析为局部变量
            i32 local = resolveLocal(m_current, name);
            if (local != -1) {
                // 设置局部变量
                if (valueReg >= 0) {
                    emitABC(OpCode::Move, local, valueReg, 0, 0);
                } else {
                    emitABC(OpCode::LoadNil, local, 0, 0, 0);
                }
                continue;
            }
            
            // 尝试解析为upvalue
            i32 upvalue = resolveUpvalue(m_current, name);
            if (upvalue != -1) {
                // 设置upvalue
                if (valueReg >= 0) {
                    emitABC(OpCode::SetUpval, valueReg, upvalue, 0, 0);
                } else {
                    emitABC(OpCode::LoadNil, tempRegs, 0, 0, 0);
                    emitABC(OpCode::SetUpval, tempRegs, upvalue, 0, 0);
                }
                continue;
            }
            
            // 设置全局变量
            // 首先加载_ENV
            emitABC(OpCode::GetUpval, tempRegs + varCount, 0, 0, 0); // _ENV总是第一个upvalue
            
            // 添加变量名到常量表
            i32 constant = addStringConstant(name);
            
            // 先准备值
            if (valueReg < 0) {
                emitABC(OpCode::LoadNil, tempRegs, 0, 0, 0);
                valueReg = tempRegs;
            }
            
            // 设置全局变量
            emitABC(OpCode::SetTable, tempRegs + varCount, constant, valueReg, 0);
        } else if (auto tableAccessExpr = ::std::dynamic_pointer_cast<TableAccessExpr>(vars[i])) {
            // 表元素赋值
            i32 tableReg = tempRegs + varCount;
            i32 keyReg = tempRegs + varCount + 1;
            
            // 编译表表达式
            compileExpression(tableAccessExpr->getTable(), tableReg);
            
            // 编译键表达式
            compileExpression(tableAccessExpr->getKey(), keyReg);
            
            // 先准备值
            if (valueReg < 0) {
                emitABC(OpCode::LoadNil, tempRegs, 0, 0, 0);
                valueReg = tempRegs;
            }
            
            // 设置表元素
            emitABC(OpCode::SetTable, tableReg, keyReg, valueReg, 0);
        } else if (auto fieldAccessExpr = ::std::dynamic_pointer_cast<FieldAccessExpr>(vars[i])) {
            // 表字段赋值
            i32 tableReg = tempRegs + varCount;
            
            // 编译表表达式
            compileExpression(fieldAccessExpr->getTable(), tableReg);
            
            // 添加字段名到常量表
            i32 constant = addStringConstant(fieldAccessExpr->getField());
            
            // 先准备值
            if (valueReg < 0) {
                emitABC(OpCode::LoadNil, tempRegs, 0, 0, 0);
                valueReg = tempRegs;
            }
            
            // 设置表字段
            emitABC(OpCode::SetTable, tableReg, constant, valueReg, 0);
        }
    }
}

void Compiler::compileLocalVarDeclStmt(Ptr<LocalVarDeclStmt> stmt) {
    const auto& names = stmt->getNames();
    const auto& exprs = stmt->getExpressions();
    
    i32 varCount = static_cast<i32>(names.size());
    i32 exprCount = static_cast<i32>(exprs.size());
    
    // 声明临时变量的基址
    i32 baseReg = m_current->localCount;
    
    // 计算表达式
    for (i32 i = 0; i < exprCount - 1; ++i) {
        compileExpression(exprs[i], baseReg + i);
    }
    
    // 最后一个表达式可能产生多个返回值
    if (exprCount > 0) {
        if (exprCount < varCount && isFunctionCall(exprs.back())) {
            // 如果最后一个表达式是函数调用，并且变量比表达式多，
            // 那么我们需要捕获所有返回值
            i32 lastReg = baseReg + exprCount - 1;
            compileFunctionCallForMultipleReturns(::std::dynamic_pointer_cast<FunctionCallExpr>(exprs.back()), lastReg, varCount - exprCount + 1);
        } else {
            // 否则正常编译最后一个表达式
            compileExpression(exprs.back(), baseReg + exprCount - 1);
        }
    }
    
    // 添加局部变量
    for (i32 i = 0; i < varCount; ++i) {
        addLocal(names[i]);
        
        if (i >= exprCount) {
            // 如果变量比表达式多，那么未初始化的变量为nil
            emitABC(OpCode::LoadNil, baseReg + i, 0, 0, 0);
        }
    }
}

void Compiler::compileFunctionCallStmt(Ptr<FunctionCallStmt> stmt) {
    // 编译函数调用表达式，忽略返回值
    compileFunctionCallExpr(stmt->getCall(), m_current->localCount);
    // 弹出所有返回值
    emitABC(OpCode::Pop, 1, 0, 0, 0);
}

void Compiler::compileIfStmt(Ptr<IfStmt> stmt) {
    // 编译条件表达式
    compileExpression(stmt->getCondition(), m_current->localCount);
    
    // 生成条件跳转指令
    usize jumpToElse = emitJump(OpCode::JumpIfFalse, 0);
    
    // 弹出条件值
    emitABC(OpCode::Pop, 1, 0, 0, 0);
    
    // 编译then分支
    compileBlock(stmt->getThenBlock());
    
    // 生成跳过else分支的指令
    usize jumpToEnd = emitJump(OpCode::Jump, 0);
    
    // 修补跳转到else分支的指令
    patchJump(jumpToElse, m_current->function->getCode().size());
    
    // 弹出条件值
    emitABC(OpCode::Pop, 1, 0, 0, 0);
    
    // 编译elseif分支
    const auto& elseIfBranches = stmt->getElseIfBranches();
    std::vector<usize> jumpToEndFromElseIf;
    
    for (const auto& branch : elseIfBranches) {
        // 编译条件表达式
        compileExpression(branch.condition, m_current->localCount);
        
        // 生成条件跳转指令
        usize jumpToNextBranch = emitJump(OpCode::JumpIfFalse, 0);
        
        // 弹出条件值
        emitABC(OpCode::Pop, 1, 0, 0, 0);
        
        // 编译分支体
        compileBlock(branch.body);
        
        // 生成跳过后续分支的指令
        jumpToEndFromElseIf.push_back(emitJump(OpCode::Jump, 0));
        
        // 修补跳转到下一个分支的指令
        patchJump(jumpToNextBranch, m_current->function->getCode().size());
        
        // 弹出条件值
        emitABC(OpCode::Pop, 1, 0, 0, 0);
    }
    
    // 编译else分支
    if (stmt->getElseBlock()) {
        compileBlock(stmt->getElseBlock());
    }
    
    // 修补所有跳转到结尾的指令
    patchJump(jumpToEnd, m_current->function->getCode().size());
    for (usize jump : jumpToEndFromElseIf) {
        patchJump(jump, m_current->function->getCode().size());
    }
}

void Compiler::compileWhileStmt(Ptr<WhileStmt> stmt) {
    // 记住循环开始的位置
    usize loopStart = m_function->getCode().size();
    
    // 编译条件表达式
    compileExpression(stmt->getCondition(), m_localsCount);
    
    // 生成条件跳转指令
    usize exitJump = emitJump(OpCode::JumpIfFalse, 0);
    
    // 弹出条件值
    emitABC(OpCode::Pop, 1, 0, 0, 0);
    
    // 编译循环体
    compileBlock(stmt->getBody().get(), m_currentState);
    
    // 生成跳转到循环开始的指令
    emitASBx(OpCode::Jump, 0, loopStart - (m_function->getCode().size() + 1), 0);
    
    // 修补退出循环的指令
    patchJump(exitJump, m_function->getCode().size());
    
    // 弹出条件值
    emitABC(OpCode::Pop, 1, 0, 0, 0);
}

void Compiler::compileDoStmt(Ptr<DoStmt> stmt) {
    // 简单地编译代码块
    compileBlock(stmt->getBody().get(), m_currentState);
}

void Compiler::compileForStmt(Ptr<NumericForStmt> stmt) {
    // 进入新作用域
    beginScope();
    
    // 编译初始值表达式
    compileExpression(stmt->getInitial(), m_localsCount);
    
    // 编译上限表达式
    compileExpression(stmt->getLimit(), m_localsCount + 1);
    
    // 编译步长表达式，如果没有则默认为1
    if (stmt->getStep()) {
        compileExpression(stmt->getStep(), m_localsCount + 2);
    } else {
        // 加载默认步长1
        emitABC(OpCode::LoadK, m_localsCount + 2, addConstant(Object::Value::number(1)), 0, 0);
    }
    
    // 添加循环变量
    addLocal(stmt->getVariable());
    
    // 记住循环开始的位置
    usize loopStart = m_function->getCode().size();
    
    // 检查循环条件
    if (stmt->getStep()) {
        // 根据步长的符号决定比较操作
        // TODO: 实现步长检查和相应的比较操作
    }
    
    // 生成条件跳转指令
    usize exitJump = emitJump(OpCode::ForLoop, 0);
    
    // 编译循环体
    compileBlock(stmt->getBody().get(), m_currentState);
    
    // 生成循环指令
    emitASBx(OpCode::ForPrep, m_localsCount, loopStart - (m_function->getCode().size() + 1), 0);
    
    // 修补退出循环的指令
    patchJump(exitJump, m_function->getCode().size());
    
    // 离开作用域
    endScope();
}

void Compiler::compileGenericForStmt(Ptr<GenericForStmt> stmt) {
    // 进入新作用域
    beginScope();
    
    // 添加迭代器函数、状态和控制变量
    i32 iteratorFunc = m_localsCount;
    i32 iteratorState = m_localsCount + 1;
    i32 controlVar = m_localsCount + 2;
    
    // 编译迭代器表达式
    const auto& iterExpressions = stmt->getIterableExpressions();
    
    // 编译迭代器函数
    compileExpression(iterExpressions[0], iteratorFunc);
    
    // 编译状态
    if (iterExpressions.size() > 1) {
        compileExpression(iterExpressions[1], iteratorState);
    } else {
        emitABC(OpCode::LoadNil, iteratorState, 0, 0, 0);
    }
    
    // 编译控制变量
    if (iterExpressions.size() > 2) {
        compileExpression(iterExpressions[2], controlVar);
    } else {
        emitABC(OpCode::LoadNil, controlVar, 0, 0, 0);
    }
    
    // 添加循环变量
    const auto& varNames = stmt->getVariables();
    for (usize i = 0; i < varNames.size(); ++i) {
        addLocal(varNames[i]);
    }
    
    // 记住循环开始的位置
    usize loopStart = m_function->getCode().size();
    
    // 生成循环指令
    emitABC(OpCode::TForCall, iteratorFunc, varNames.size(), 0, 0);
    
    // 生成条件跳转指令
    usize exitJump = emitJump(OpCode::TForLoop, 0);
    
    // 编译循环体
    compileBlock(stmt->getBody().get(), m_currentState);
    
    // 生成跳转到循环开始的指令
    emitASBx(OpCode::Jump, 0, loopStart - (m_function->getCode().size() + 1), 0);
    
    // 修补退出循环的指令
    patchJump(exitJump, m_function->getCode().size());
    
    // 离开作用域
    endScope();
}

void Compiler::compileRepeatStmt(Ptr<RepeatStmt> stmt) {
    // 进入新作用域
    beginScope();
    
    // 记住循环开始的位置
    usize loopStart = m_function->getCode().size();
    
    // 编译循环体
    compileBlock(stmt->getBody().get(), m_currentState);
    
    // 编译条件表达式
    compileExpression(stmt->getCondition(), m_localsCount);
    
    // 生成条件跳转指令，条件为假时继续循环
    emitASBx(OpCode::JumpIfFalse, m_localsCount, loopStart - (m_function->getCode().size() + 1), 0);
    
    // 弹出条件值
    emitABC(OpCode::Pop, 1, 0, 0, 0);
    
    // 离开作用域
    endScope();
}

void Compiler::compileFunctionDeclStmt(Ptr<FunctionDeclStmt> stmt) {
    // 获取函数名称组件
    const auto& nameComponents = stmt->getNameComponents();
    Str funcName = nameComponents[0];
    
    // 检查是否是局部函数
    bool isLocal = stmt->isLocal();
    
    // 保存当前编译状态
    CompileState* previousState = m_currentState;
    
    // 创建新的编译状态
    CompileState newState;
    newState.function = std::make_shared<FunctionProto>();
    newState.function->setName(funcName);
    newState.enclosing = previousState;
    newState.scopeDepth = 0;
    
    // 设置当前编译状态
    m_currentState = &newState;
    
    // 进入新作用域
    beginScope();
    
    // 处理参数
    const auto& params = stmt->getParameters();
    for (const Str& param : params) {
        // 添加参数作为局部变量
        addLocal(param);
    }
    
    // 编译函数体
    compileBlock(stmt->getBody().get(), &newState);
    
    // 确保有返回指令
    if (m_lastInstructionWasReturn) {
        m_lastInstructionWasReturn = false;
    } else {
        emitReturn(0, 0, -1); // 返回0个结果
    }
    
    // 离开作用域
    endScope();
    
    // 恢复之前的编译状态
    m_currentState = previousState;
    
    // 添加子函数原型
    i32 protoIndex = m_function->addProto(newState.function);
    
    // 函数注册目标寄存器
    i32 funcReg = m_localsCount;
    
    // 生成创建闭包的指令
    emitABx(OpCode::Closure, funcReg, protoIndex, 0);
    
    // 为每个upvalue生成额外指令
    for (const auto& upvalue : newState.function->getUpvalues()) {
        emitABC(upvalue.isLocal ? OpCode::Move : OpCode::GetUpval, 0, upvalue.index, 0, 0);
    }
    
    // 注册函数
    if (isLocal) {
        // 局部函数，添加为局部变量
        addLocal(funcName);
    } else if (nameComponents.size() == 1) {
        // 全局函数
        // 加载_ENV
        emitABC(OpCode::GetUpval, funcReg + 1, 0, 0, 0); // _ENV总是第一个upvalue
        
        // 添加函数名到常量表
        i32 constant = addStringConstant(funcName);
        
        // 设置全局函数
        emitABC(OpCode::SetTable, funcReg + 1, constant, funcReg, 0);
    } else {
        // 成员函数，需要处理表和字段
        
        // 找到表
        if (isLocal) {
            // 尝试解析为局部变量
            i32 local = resolveLocal(m_currentState, nameComponents[0]);
            if (local != -1) {
                // 加载局部变量
                emitABC(OpCode::Move, funcReg + 1, local, 0, 0);
            } else {
                // 尝试解析为upvalue
                i32 upvalue = resolveUpvalue(m_currentState, nameComponents[0]);
                if (upvalue != -1) {
                    // 加载upvalue
                    emitABC(OpCode::GetUpval, funcReg + 1, upvalue, 0, 0);
                } else {
                    // 错误：未找到局部变量或upvalue
                    throw CompileError("Undefined local variable: " + nameComponents[0]);
                }
            }
        } else {
            // 全局变量
            // 加载_ENV
            emitABC(OpCode::GetUpval, funcReg + 1, 0, 0, 0); // _ENV总是第一个upvalue
            
            // 添加表名到常量表
            i32 constant = addStringConstant(nameComponents[0]);
            
            // 获取全局变量
            emitABC(OpCode::GetTable, funcReg + 1, funcReg + 1, constant, 0);
        }
        
        // 遍历中间字段
        for (usize i = 1; i < nameComponents.size() - 1; ++i) {
            // 添加字段名到常量表
            i32 constant = addStringConstant(nameComponents[i]);
            
            // 获取表字段
            emitABC(OpCode::GetTable, funcReg + 1, funcReg + 1, constant, 0);
        }
        
        // 添加最后一个字段到常量表
        i32 constant = addStringConstant(nameComponents.back());
        
        // 设置表字段
        emitABC(OpCode::SetTable, funcReg + 1, constant, funcReg, 0);
    }
}

void Compiler::compileReturnStmt(Ptr<ReturnStmt> stmt) {
    const auto& exprs = stmt->getExpressions();
    i32 exprCount = static_cast<i32>(exprs.size());
    
    if (exprCount == 0) {
        // 无返回值
        emitReturn(0, 0, 0);
    } else if (exprCount == 1 && isFunctionCall(exprs[0])) {
        // 单个函数调用，直接返回其所有结果
        compileFunctionCallForTailCall(std::dynamic_pointer_cast<FunctionCallExpr>(exprs[0]), 0);
    } else {
        // 普通返回值列表
        for (i32 i = 0; i < exprCount; ++i) {
            compileExpression(exprs[i], i);
        }
        emitReturn(0, exprCount, 0);
    }
    
    m_lastInstructionWasReturn = true;
}

void Compiler::compileBreakStmt(Ptr<BreakStmt> stmt) {
    // TODO: 实现break语句
    // 需要跟踪循环和break跳转指令，以便在离开循环时修补这些指令
}

} // namespace Lua
