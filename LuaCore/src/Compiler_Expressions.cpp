#include "Compiler.h"
#include "types.h"
#include <iostream>
#include <cassert>

namespace LuaCore {

void Compiler::compileExpression(Ptr<Expression> expr, i32 reg) {
    if (auto literalExpr = std::dynamic_pointer_cast<LiteralExpr>(expr)) {
        compileLiteralExpr(literalExpr, reg);
    } else if (auto varExpr = std::dynamic_pointer_cast<VariableExpr>(expr)) {
        compileVariableExpr(varExpr, reg);
    } else if (auto binaryExpr = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        compileBinaryExpr(binaryExpr, reg);
    } else if (auto unaryExpr = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        compileUnaryExpr(unaryExpr, reg);
    } else if (auto tableAccessExpr = std::dynamic_pointer_cast<TableAccessExpr>(expr)) {
        compileTableAccessExpr(tableAccessExpr, reg);
    } else if (auto fieldAccessExpr = std::dynamic_pointer_cast<FieldAccessExpr>(expr)) {
        compileFieldAccessExpr(fieldAccessExpr, reg);
    } else if (auto functionCallExpr = std::dynamic_pointer_cast<FunctionCallExpr>(expr)) {
        compileFunctionCallExpr(functionCallExpr, reg);
    } else if (auto tableConstructorExpr = std::dynamic_pointer_cast<TableConstructorExpr>(expr)) {
        compileTableConstructorExpr(tableConstructorExpr, reg);
    } else if (auto functionDefExpr = std::dynamic_pointer_cast<FunctionDefExpr>(expr)) {
        compileFunctionDefExpr(functionDefExpr, reg);
    } else {
        // 未知表达式类型
        std::cerr << "Unknown expression type!" << std::endl;
        assert(false && "Unknown expression type");
    }
}

void Compiler::compileLiteralExpr(Ptr<LiteralExpr> expr, i32 reg) {
    const Value& value = expr->getValue();
    
    switch (value.type()) {
    case Value::Type::Nil:
            emitABC(OpCode::LoadNil, reg, 0, 0, 0);
            break;
            
        case Value::Type::Boolean:
            if (value.asBoolean()) {
                emitABC(OpCode::LoadTrue, reg, 0, 0, 0);
            } else {
                emitABC(OpCode::LoadFalse, reg, 0, 0, 0);
            }
            break;
            
        case Value::Type::Number:
        case Value::Type::String:
        case Value::Type::Table:
        case Value::Type::Function:
        case Value::Type::UserData:
            // 添加到常量表并加载
            {
                i32 constIndex = addConstant(value);
                emitABx(OpCode::LoadK, reg, constIndex, 0);
            }
            break;
    }
}

void Compiler::compileVariableExpr(Ptr<VariableExpr> expr, i32 reg) {
    // 解析变量名
    const Str& name = expr->getName();
    
    // 首先检查局部变量
    i32 local = resolveLocal(m_current, name);
    if (local != -1) {
        // 如果是局部变量，直接将其值移动到目标寄存器
        emitABC(OpCode::Move, reg, local, 0, 0);
        return;
    }
    
    // 检查upvalue
    i32 upvalue = resolveUpvalue(m_current, name);
    if (upvalue != -1) {
        // 如果是upvalue，获取其值
        emitABC(OpCode::GetUpvalue, reg, upvalue, 0, 0);
        return;
    }
    
    // 作为全局变量处理
    i32 nameIndex = addStringConstant(name);
    // 全局变量保存在特殊的注册表中
    emitABC(OpCode::GetField, reg, 0, nameIndex, 0); // 假设寄存器0存放全局环境
}

void Compiler::compileBinaryExpr(Ptr<BinaryExpr> expr, i32 reg) {
    // 编译运算符左侧表达式
    compileExpression(expr->getLeft(), reg);
    
    // 编译运算符右侧表达式
    compileExpression(expr->getRight(), reg + 1);
    
    // 根据运算符生成操作码
    switch (expr->getOp()) {
        case BinaryExpr::Op::Add:
            emitABC(OpCode::Add, reg, reg, reg + 1, 0);
            break;
            
        case BinaryExpr::Op::Subtract:
            emitABC(OpCode::Sub, reg, reg, reg + 1, 0);
            break;
            
        case BinaryExpr::Op::Multiply:
            emitABC(OpCode::Mul, reg, reg, reg + 1, 0);
            break;
            
        case BinaryExpr::Op::Divide:
            emitABC(OpCode::Div, reg, reg, reg + 1, 0);
            break;
            
        case BinaryExpr::Op::Modulo:
            emitABC(OpCode::Mod, reg, reg, reg + 1, 0);
            break;
            
        case BinaryExpr::Op::Power:
            emitABC(OpCode::Pow, reg, reg, reg + 1, 0);
            break;
            
        case BinaryExpr::Op::Concat:
            emitABC(OpCode::Concat, reg, reg, reg + 1, 0);
            break;
            
        case BinaryExpr::Op::Equal:
        case BinaryExpr::Op::NotEqual:
        case BinaryExpr::Op::LessThan:
        case BinaryExpr::Op::LessEqual:
        case BinaryExpr::Op::GreaterThan:
        case BinaryExpr::Op::GreaterEqual:
            // 比较操作需要条件跳转
            {
                OpCode opCode;
                bool invert = false;
                
                switch (expr->getOp()) {
                    case BinaryExpr::Op::Equal:
                        opCode = OpCode::Eq;
                        invert = false;
                        break;
                        
                    case BinaryExpr::Op::NotEqual:
                        opCode = OpCode::Eq;
                        invert = true;
                        break;
                        
                    case BinaryExpr::Op::LessThan:
                        opCode = OpCode::Lt;
                        invert = false;
                        break;
                        
                    case BinaryExpr::Op::GreaterEqual:
                        opCode = OpCode::Lt;
                        invert = true;
                        break;
                        
                    case BinaryExpr::Op::LessEqual:
                        opCode = OpCode::Le;
                        invert = false;
                        break;
                        
                    case BinaryExpr::Op::GreaterThan:
                        opCode = OpCode::Le;
                        invert = true;
                        break;
                        
                    default:
                        assert(false && "Unreachable");
                        break;
                }
                
                // 生成比较和条件跳转
                emitABC(opCode, invert ? 1 : 0, reg, reg + 1, 0);
                size_t jmpInstr = emitJump(OpCode::Jump, 0);
                
                // 将结果设置为false
                emitABC(OpCode::LoadFalse, reg, 0, 0, 0);
                
                // 跳过下一条指令
                size_t jmpOverInstr = emitJump(OpCode::Jump, 0);
                
                // 设置第一个跳转位置，将结果设置为true
                patchJump(jmpInstr, emitABC(OpCode::LoadTrue, reg, 0, 0, 0));
                
                // 设置第二个跳转的位置（跳过true赋值部分）
                patchJump(jmpOverInstr, m_current->proto->getCode().size());
            }
            break;
            
        case BinaryExpr::Op::And:
        case BinaryExpr::Op::Or:
            // 逻辑操作需要短路
            {
                if (expr->getOp() == BinaryExpr::Op::And) {
                    // 如果左侧为假，跳过右侧计算，直接返回左侧值（假）
                    emitABC(OpCode::Test, reg, 0, 0, 0);
                    size_t jmpInstr = emitJump(OpCode::JumpIfFalse, 0);
                    
                    // 计算右侧表达式，覆盖左侧值
                    compileExpression(expr->getRight(), reg);
                    
                    // 设置跳转位置
                    patchJump(jmpInstr, m_current->proto->getCode().size());
                } else { // Or
                    // 如果左侧为真，跳过右侧计算，直接返回左侧值（真）
                    emitABC(OpCode::Test, reg, 0, 1, 0);
                    size_t jmpInstr = emitJump(OpCode::JumpIfTrue, 0);
                    
                    // 计算右侧表达式，覆盖左侧值
                    compileExpression(expr->getRight(), reg);
                    
                    // 设置跳转位置
                    patchJump(jmpInstr, m_current->proto->getCode().size());
                }
            }
            break;
    }
}

void Compiler::compileUnaryExpr(Ptr<UnaryExpr> expr, i32 reg) {
    // 编译操作数
    compileExpression(expr->getExpression(), reg);
    
    // 根据操作类型生成指令
    switch (expr->getOp()) {
        case UnaryExpr::Op::Negate:
            emitABC(OpCode::Neg, reg, reg, 0, 0);
            break;
            
        case UnaryExpr::Op::Not:
            emitABC(OpCode::Not, reg, reg, 0, 0);
            break;
            
        case UnaryExpr::Op::Length:
            emitABC(OpCode::Len, reg, reg, 0, 0);
            break;
    }
}

void Compiler::compileTableAccessExpr(Ptr<TableAccessExpr> expr, i32 reg) {
    // 编译表达式
    compileExpression(expr->getTable(), reg);
    
    // 编译键
    compileExpression(expr->getKey(), reg + 1);
    
    // 生成获取表项指令
    emitABC(OpCode::GetTable, reg, reg, reg + 1, 0);
}

void Compiler::compileFieldAccessExpr(Ptr<FieldAccessExpr> expr, i32 reg) {
    // 编译表达式
    compileExpression(expr->getTable(), reg);
    
    // 添加字段名到常量表
    i32 fieldIndex = addStringConstant(expr->getField());
    
    // 生成获取字段指令
    emitABC(OpCode::GetField, reg, reg, fieldIndex, 0);
}

void Compiler::compileFunctionCallExpr(Ptr<FunctionCallExpr> expr, i32 reg) {
    // 编译函数表达式
    compileExpression(expr->getFunction(), reg);
    
    // 编译参数列表
    i32 nargs = 0;
    for (const auto& arg : expr->getArgs()->getExpressions()) {
        compileExpression(arg, reg + 1 + nargs);
        nargs++;
    }
    
    // 生成调用指令
    emitABC(OpCode::Call, reg, nargs + 1, 2, 0); // B是参数数量+1，C是期望返回值数量+1
}

void Compiler::compileTableConstructorExpr(Ptr<TableConstructorExpr> expr, i32 reg) {
    // 创建新表
    emitABC(OpCode::NewTable, reg, 0, 0, 0);
    
    // 添加表字段
    i32 arrayIndex = 1;
    for (const auto& field : expr->getFields()) {
        if (field.key == nullptr) {
            // 数组项
            compileExpression(field.value, reg + 1);
            
            // 添加键（数组索引）到常量表
            i32 keyIndex = addConstant(Value(static_cast<f64>(arrayIndex++)));
            
            // 设置表字段
            emitABC(OpCode::SetField, reg, keyIndex, reg + 1, 0);
        } else {
            // 键值对
            compileExpression(field.key, reg + 1);
            compileExpression(field.value, reg + 2);
            
            // 设置表字段
            emitABC(OpCode::SetTable, reg, reg + 1, reg + 2, 0);
        }
    }
}

void Compiler::compileFunctionDefExpr(Ptr<FunctionDefExpr> expr, i32 reg) {
    // 保存当前编译状态
    CompileState* enclosingState = m_current;
    
    // 创建新的函数原型
    auto proto = std::make_shared<FunctionProto>("anonymous", expr->getParams().size(), expr->isVararg());
    
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
    for (const auto& param : expr->getParams()) {
        addLocal(param);
    }
    
    // 编译函数体
    compileBlock(expr->getBody());
    
    // 添加默认返回指令
    emitABC(OpCode::Return, 0, 1, 0, 0);
    
    // 结束作用域
    endScope();
    
    // 恢复之前的编译状态
    m_current = enclosingState;
    
    // 添加子函数原型
    enclosingState->proto->addProto(proto);
    
    // 生成创建闭包指令
    emitABx(OpCode::Closure, reg, enclosingState->proto->getProtos().size() - 1, 0);
    
    // 处理upvalue
    for (const auto& upvalue : proto->getUpvalues()) {
        emitABC(upvalue.isLocal ? OpCode::Move : OpCode::GetUpvalue, 0, upvalue.index, 0, 0);
    }
}

} // namespace LuaCore
