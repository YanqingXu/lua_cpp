#include "compiler.hpp"
#include "types.hpp"
#include "vm/instruction.hpp"
#include <iostream>
#include <cassert>

namespace Lua {

void Compiler::compileExpression(Ptr<Expression> expr, i32 reg) {
    if (auto nilExpr = ::std::dynamic_pointer_cast<NilExpr>(expr)) {
        // 编译nil字面量
        emitABC(OpCode::LoadNil, reg, 0, 0, 0);
    } else if (auto boolExpr = ::std::dynamic_pointer_cast<BoolExpr>(expr)) {
        // 编译布尔字面量
        if (boolExpr->getValue()) {
            emitABC(OpCode::LoadTrue, reg, 0, 0, 0);
        } else {
            emitABC(OpCode::LoadFalse, reg, 0, 0, 0);
        }
    } else if (auto numberExpr = ::std::dynamic_pointer_cast<NumberExpr>(expr)) {
        // 编译数字字面量
        i32 constant = addConstant(Object::Value::number(numberExpr->getValue()));
        emitABx(OpCode::LoadK, reg, constant, 0);
    } else if (auto stringExpr = ::std::dynamic_pointer_cast<StringExpr>(expr)) {
        // 编译字符串字面量
        i32 constant = addStringConstant(stringExpr->getValue());
        emitABx(OpCode::LoadK, reg, constant, 0);
    } else if (auto literalExpr = ::std::dynamic_pointer_cast<LiteralExpr>(expr)) {
        compileLiteralExpr(literalExpr, reg);
    } else if (auto varExpr = ::std::dynamic_pointer_cast<VariableExpr>(expr)) {
        compileVariableExpr(varExpr, reg);
    } else if (auto binaryExpr = ::std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        compileBinaryExpr(binaryExpr, reg);
    } else if (auto unaryExpr = ::std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        compileUnaryExpr(unaryExpr, reg);
    } else if (auto tableAccessExpr = ::std::dynamic_pointer_cast<TableAccessExpr>(expr)) {
        compileTableAccessExpr(tableAccessExpr, reg);
    } else if (auto fieldAccessExpr = ::std::dynamic_pointer_cast<FieldAccessExpr>(expr)) {
        compileFieldAccessExpr(fieldAccessExpr, reg);
    } else if (auto functionCallExpr = ::std::dynamic_pointer_cast<FunctionCallExpr>(expr)) {
        compileFunctionCallExpr(functionCallExpr, reg);
    } else if (auto tableConstructorExpr = ::std::dynamic_pointer_cast<TableConstructorExpr>(expr)) {
        compileTableConstructorExpr(tableConstructorExpr, reg);
    } else if (auto functionDefExpr = ::std::dynamic_pointer_cast<FunctionDefExpr>(expr)) {
        compileFunctionDefExpr(functionDefExpr, reg);
    } else {
        // 未知表达式类型
        std::cerr << "Unknown expression type!" << std::endl;
        assert(false && "Unknown expression type");
    }
}

void Compiler::compileLiteralExpr(Ptr<LiteralExpr> expr, i32 reg) {
    const Object::Value& value = expr->getValue();
    
    switch (value.type()) {
        case Object::Value::Type::Nil:
            emitABC(OpCode::LoadNil, reg, 0, 0, 0);
            break;
            
        case Object::Value::Type::Boolean:
            if (value.asBoolean()) {
                emitABC(OpCode::LoadTrue, reg, 0, 0, 0);
            } else {
                emitABC(OpCode::LoadFalse, reg, 0, 0, 0);
            }
            break;
            
        case Object::Value::Type::Number:
        case Object::Value::Type::String:
            {
                // 将常量添加到常量表
                i32 constant = addConstant(value);
                emitABx(OpCode::LoadK, reg, constant, 0);
            }
            break;
            
        default:
            // 不应该出现其他类型的字面量
            std::cerr << "Unexpected literal type: " << static_cast<int>(value.getType()) << std::endl;
            assert(false && "Unexpected literal type");
            break;
    }
}

void Compiler::compileVariableExpr(Ptr<VariableExpr> expr, i32 reg) {
    const Str& name = expr->getName();
    
    // 尝试解析为局部变量
    i32 local = resolveLocal(m_current, name);
    if (local != -1) {
        // 加载局部变量
        emitABC(OpCode::Move, reg, local, 0, 0);
        return;
    }
    
    // 尝试解析为upvalue
    i32 upvalue = resolveUpvalue(m_current, name);
    if (upvalue != -1) {
        // 加载upvalue
        emitABC(OpCode::GetUpval, reg, upvalue, 0, 0);
        return;
    }
    
    // 解析为全局变量
    // 首先加载_ENV
    emitABC(OpCode::GetUpval, reg, 0, 0, 0); // _ENV总是第一个upvalue
    
    // 添加变量名到常量表
    i32 constant = addStringConstant(name);
    
    // 从_ENV中获取全局变量
    emitABC(OpCode::GetTable, reg, reg, constant, 0);
}

void Compiler::compileBinaryExpr(Ptr<BinaryExpr> expr, i32 reg) {
    BinaryExpr::Op op = expr->getOperator();
    
    // 短路操作符的特殊处理
    if (op == BinaryExpr::Op::And) {
        // 编译左操作数
        compileExpression(expr->getLeft(), reg);
        
        // 如果左操作数为假，跳过右操作数
        usize jumpFalse = emitJump(OpCode::JumpIfFalse, 0);
        
        // 弹出左操作数
        emitABC(OpCode::Pop, 1, 0, 0, 0);
        
        // 编译右操作数
        compileExpression(expr->getRight(), reg);
        
        // 修补跳转指令
        patchJump(jumpFalse, m_function->getCode().size());
        return;
    }
    
    if (op == BinaryExpr::Op::Or) {
        // 编译左操作数
        compileExpression(expr->getLeft(), reg);
        
        // 如果左操作数为真，跳过右操作数
        Common::usize jumpTrue = emitJump(OpCode::JumpIfTrue, 0);
        
        // 弹出左操作数
        emitABC(OpCode::Pop, 1, 0, 0, 0);
        
        // 编译右操作数
        compileExpression(expr->getRight(), reg);
        
        // 修补跳转指令
        patchJump(jumpTrue, m_function->getCode().size());
        return;
    }
    
    // 其他二元操作符的处理
    
    // 编译左操作数
    compileExpression(expr->getLeft(), reg);
    
    // 编译右操作数
    compileExpression(expr->getRight(), reg + 1);
    
    // 根据操作符生成相应指令
    switch (op) {
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
            emitABC(OpCode::Eq, reg, reg, reg + 1, 0);
            break;
            
        case BinaryExpr::Op::NotEqual:
            emitABC(OpCode::Eq, reg, reg, reg + 1, 0);
            emitABC(OpCode::Not, reg, reg, 0, 0);
            break;
            
        case BinaryExpr::Op::LessThan:
            emitABC(OpCode::Lt, reg, reg, reg + 1, 0);
            break;
            
        case BinaryExpr::Op::LessEqual:
            emitABC(OpCode::Le, reg, reg, reg + 1, 0);
            break;
            
        case BinaryExpr::Op::GreaterThan:
            emitABC(OpCode::Lt, reg, reg + 1, reg, 0);
            break;
            
        case BinaryExpr::Op::GreaterEqual:
            emitABC(OpCode::Le, reg, reg + 1, reg, 0);
            break;
            
        default:
            // 不应该到达这里
            std::cerr << "Unexpected binary operator: " << static_cast<int>(op) << std::endl;
            assert(false && "Unexpected binary operator");
            break;
    }
}

void Compiler::compileUnaryExpr(Common::Ptr<UnaryExpr> expr, Common::i32 reg) {
    // 编译操作数
    compileExpression(expr->getOperand(), reg);
    
    // 根据一元操作符生成相应指令
    switch (expr->getOperator()) {
        case UnaryExpr::Op::Negate:
            emitABC(OpCode::Neg, reg, reg, 0, 0);
            break;
            
        case UnaryExpr::Op::Not:
            emitABC(OpCode::Not, reg, reg, 0, 0);
            break;
            
        case UnaryExpr::Op::Length:
            emitABC(OpCode::Len, reg, reg, 0, 0);
            break;
            
        default:
            // 不应该到达这里
            std::cerr << "Unexpected unary operator: " << static_cast<int>(expr->getOperator()) << std::endl;
            assert(false && "Unexpected unary operator");
            break;
    }
}

void Compiler::compileTableAccessExpr(Ptr<TableAccessExpr> expr, i32 reg) {
    // 编译表达式
    compileExpression(expr->getTable(), reg);
    
    // 编译键表达式
    compileExpression(expr->getKey(), reg + 1);
    
    // 获取表元素
    emitABC(OpCode::GetTable, reg, reg, reg + 1, 0);
}

void Compiler::compileFieldAccessExpr(Ptr<FieldAccessExpr> expr, i32 reg) {
    // 编译表达式
    compileExpression(expr->getTable(), reg);
    
    // 添加字段名到常量表
    i32 constant = addStringConstant(expr->getField());
    
    // 获取表字段
    emitABC(OpCode::GetTable, reg, reg, constant, 0);
}

void Compiler::compileFunctionCallExpr(Ptr<FunctionCallExpr> expr, i32 reg) {
    // 编译函数表达式
    compileExpression(expr->getFunction(), reg);
    
    // 编译参数
    const Vec<Ptr<Expression>>& args = expr->getArguments();
    for (usize i = 0; i < args.size(); ++i) {
        compileExpression(args[i], reg + 1 + i);
    }
    
    // 生成调用指令
    emitABC(OpCode::Call, reg, args.size() + 1, 1, 0);
}

void Compiler::compileTableConstructorExpr(Ptr<TableConstructorExpr> expr, i32 reg) {
    // 创建新表
    emitABC(OpCode::NewTable, reg, 0, 0, 0);
    
    // 处理表构造器的每个字段
    const Vec<TableConstructorExpr::Field>& fields = expr->getFields();
    for (usize i = 0; i < fields.size(); ++i) {
        const TableConstructorExpr::Field& field = fields[i];
        
        if (field.type == TableConstructorExpr::Field::Type::List) {
            // 列表字段
            compileExpression(field.value, reg + 1);
            emitABC(OpCode::SetList, reg, i + 1, 0, 0);
        } else if (field.type == TableConstructorExpr::Field::Type::Record) {
            // 记录字段
            // 编译键
            compileExpression(field.key, reg + 1);
            // 编译值
            compileExpression(field.value, reg + 2);
            // 设置表字段
            emitABC(OpCode::SetTable, reg, reg + 1, reg + 2, 0);
        }
    }
}

void Compiler::compileFunctionDefExpr(Ptr<FunctionDefExpr> expr, i32 reg) {
    // 保存当前编译状态
    CompileState* previousState = m_current;
    
    // 创建新的编译状态
    CompileState newState;
    newState.proto = make_ptr<FunctionProto>();
    newState.enclosing = previousState;
    newState.scopeDepth = 0;
    newState.localCount = 0;
    newState.stackSize = 0;
    
    // 设置当前编译状态
    m_current = &newState;
    
    // 进入新作用域
    beginScope();
    
    // 处理参数
    const Vec<Str>& params = expr->getParameters();
    for (const Str& param : params) {
        // 添加参数作为局部变量
        addLocal(param);
    }
    
    // 编译函数体
    compileBlock(expr->getBody());
    
    // 确保有返回指令
    if (!m_lastInstructionWasReturn) {
        emitABC(OpCode::Return, 0, 1, 0, 0); // 返回0个结果
    }
    
    // 离开作用域
    endScope();
    
    // 恢复之前的编译状态
    m_current = previousState;
    
    // 添加子函数原型
    i32 protoIndex = m_current->proto->addProto(newState.proto);
    
    // 生成创建闭包的指令
    emitABx(OpCode::Closure, reg, protoIndex, 0);
    
    // 为每个upvalue生成额外指令
    for (const auto& upvalue : newState.upvalues) {
        emitABC(upvalue.isLocal ? OpCode::Move : OpCode::GetUpval, 0, upvalue.index, 0, 0);
    }
}

} // namespace Lua
