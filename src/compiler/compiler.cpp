/**
 * @file compiler.cpp
 * @brief Lua字节码编译器实现
 * @description 将AST编译成Lua 5.1.5兼容的字节码
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#include "compiler.h"
#include "optimizer.h"
#include "ast_base.h"
#include <algorithm>
#include <sstream>

namespace lua_cpp {

/* ========================================================================== */
/* 作用域管理器实现 */
/* ========================================================================== */

void ScopeManager::EnterScope() {
    scope_markers_.push_back(locals_.size());
    scope_level_++;
}

int ScopeManager::ExitScope() {
    if (scope_markers_.empty()) {
        return 0;
    }
    
    Size marker = scope_markers_.back();
    scope_markers_.pop_back();
    scope_level_--;
    
    Size removed_count = locals_.size() - marker;
    locals_.resize(marker);
    
    return static_cast<int>(removed_count);
}

void ScopeManager::DeclareLocal(const std::string& name, RegisterIndex register_idx) {
    locals_.emplace_back(name, register_idx, scope_level_);
}

const LocalVariable* ScopeManager::FindLocal(const std::string& name) const {
    // 从最近声明的变量开始查找（后进先出）
    for (auto it = locals_.rbegin(); it != locals_.rend(); ++it) {
        if (it->name == name) {
            return &(*it);
        }
    }
    return nullptr;
}

/* ========================================================================== */
/* 寄存器分配器实现 */
/* ========================================================================== */

RegisterIndex RegisterAllocator::Allocate() {
    RegisterIndex reg = next_register_++;
    register_top_ = std::max(register_top_, static_cast<Size>(next_register_));
    return reg;
}

void RegisterAllocator::Free(RegisterIndex reg) {
    // 简单的释放策略：只有当释放的是最高寄存器时才回收
    if (reg == next_register_ - 1) {
        next_register_--;
    }
}

RegisterIndex RegisterAllocator::AllocateTemporary() {
    return Allocate(); // 临时寄存器使用相同的分配策略
}

void RegisterAllocator::FreeTemporaries(Size saved_top) {
    next_register_ = static_cast<RegisterIndex>(saved_top);
    register_top_ = saved_top;
}

Size RegisterAllocator::GetTop() const {
    return register_top_;
}

void RegisterAllocator::SetTop(Size top) {
    register_top_ = top;
    next_register_ = static_cast<RegisterIndex>(top);
}

/* ========================================================================== */
/* 字节码生成器实现 */
/* ========================================================================== */

BytecodeGenerator::BytecodeGenerator() : current_line_(1) {
}

void BytecodeGenerator::EmitInstruction(OpCode op, RegisterIndex a, int b, int c) {
    Instruction instruction = EncodeABC(op, a, b, c);
    instructions_.push_back(instruction);
    line_info_.push_back(current_line_);
}

void BytecodeGenerator::EmitInstruction(OpCode op, RegisterIndex a, int bx) {
    Instruction instruction = EncodeABx(op, a, bx);
    instructions_.push_back(instruction);
    line_info_.push_back(current_line_);
}

void BytecodeGenerator::EmitInstruction(OpCode op, RegisterIndex a, int sbx) {
    Instruction instruction = EncodeAsBx(op, a, sbx);
    instructions_.push_back(instruction);
    line_info_.push_back(current_line_);
}

Size BytecodeGenerator::EmitJump(JumpDirection direction) {
    Size pc = instructions_.size();
    EmitInstruction(OpCode::JMP, 0, 0); // 占位符跳转指令
    return pc;
}

void BytecodeGenerator::PatchJump(Size instruction_index, int offset) {
    if (instruction_index >= instructions_.size()) {
        throw CompilerError("Invalid jump instruction index");
    }
    
    // 修改跳转偏移
    Instruction& instruction = instructions_[instruction_index];
    instruction = EncodeAsBx(OpCode::JMP, 0, offset);
}

Size BytecodeGenerator::GetCurrentPC() const {
    return instructions_.size();
}

void BytecodeGenerator::SetCurrentLine(int line) {
    current_line_ = line;
}

int BytecodeGenerator::AddConstant(const LuaValue& value) {
    // 查找是否已存在相同常量
    for (Size i = 0; i < constants_.size(); ++i) {
        if (constants_[i] == value) {
            return static_cast<int>(i);
        }
    }
    
    // 添加新常量
    constants_.push_back(value);
    return static_cast<int>(constants_.size() - 1);
}

int BytecodeGenerator::AddString(const std::string& str) {
    return AddConstant(LuaValue(str));
}

void BytecodeGenerator::AddUpvalue(const std::string& name, bool is_local, RegisterIndex reg) {
    upvalues_.emplace_back(name, static_cast<int>(upvalues_.size()), is_local, reg);
}

Instruction BytecodeGenerator::EncodeABC(OpCode op, RegisterIndex a, int b, int c) {
    return static_cast<Instruction>(op) |
           (static_cast<Instruction>(a) << 6) |
           (static_cast<Instruction>(b) << 14) |
           (static_cast<Instruction>(c) << 23);
}

Instruction BytecodeGenerator::EncodeABx(OpCode op, RegisterIndex a, int bx) {
    return static_cast<Instruction>(op) |
           (static_cast<Instruction>(a) << 6) |
           (static_cast<Instruction>(bx) << 14);
}

Instruction BytecodeGenerator::EncodeAsBx(OpCode op, RegisterIndex a, int sbx) {
    // sBx 使用偏置编码，加上 MAXARG_sBx
    constexpr int MAXARG_sBx = 0x20000 - 1; // (1 << 18) / 2 - 1
    return static_cast<Instruction>(op) |
           (static_cast<Instruction>(a) << 6) |
           (static_cast<Instruction>(sbx + MAXARG_sBx) << 14);
}

std::unique_ptr<LuaFunction> BytecodeGenerator::CreateFunction(
    const std::string& name,
    int line_defined,
    int last_line_defined,
    int num_params,
    bool is_vararg,
    int max_stack_size) {
    
    auto function = std::make_unique<LuaFunction>();
    function->name = name;
    function->line_defined = line_defined;
    function->last_line_defined = last_line_defined;
    function->num_params = num_params;
    function->is_vararg = is_vararg;
    function->max_stack_size = max_stack_size;
    function->instructions = std::move(instructions_);
    function->constants = std::move(constants_);
    function->line_info = std::move(line_info_);
    function->upvalue_names.reserve(upvalues_.size());
    
    for (const auto& upval : upvalues_) {
        function->upvalue_names.push_back(upval.name);
    }
    
    // 重置生成器状态
    instructions_.clear();
    constants_.clear();
    line_info_.clear();
    upvalues_.clear();
    current_line_ = 1;
    
    return function;
}

/* ========================================================================== */
/* 编译器上下文实现 */
/* ========================================================================== */

CompilerContext::CompilerContext(const OptimizationConfig& config)
    : optimization_config_(config), function_level_(0) {
}

void CompilerContext::EnterFunction(const std::string& name, int line_defined) {
    // 保存当前状态
    saved_contexts_.push({
        generator_,
        scope_manager_,
        register_allocator_,
        function_level_,
        current_function_name_
    });
    
    // 重置状态为新函数
    generator_ = BytecodeGenerator();
    scope_manager_ = ScopeManager();
    register_allocator_ = RegisterAllocator();
    function_level_++;
    current_function_name_ = name;
    current_function_line_ = line_defined;
}

std::unique_ptr<LuaFunction> CompilerContext::ExitFunction(int last_line_defined, 
                                                           int num_params, 
                                                           bool is_vararg) {
    // 创建函数对象
    auto function = generator_.CreateFunction(
        current_function_name_,
        current_function_line_,
        last_line_defined,
        num_params,
        is_vararg,
        static_cast<int>(register_allocator_.GetTop())
    );
    
    // 恢复上一层状态
    if (!saved_contexts_.empty()) {
        auto saved = saved_contexts_.top();
        saved_contexts_.pop();
        
        generator_ = std::move(saved.generator);
        scope_manager_ = std::move(saved.scope_manager);
        register_allocator_ = std::move(saved.register_allocator);
        function_level_ = saved.function_level;
        current_function_name_ = saved.function_name;
    }
    
    return function;
}

RegisterIndex CompilerContext::ResolveVariable(const std::string& name) {
    // 首先查找局部变量
    const LocalVariable* local = scope_manager_.FindLocal(name);
    if (local) {
        return local->register_idx;
    }
    
    // 查找上值
    for (const auto& upval : generator_.GetUpvalues()) {
        if (upval.name == name) {
            // 生成 GETUPVAL 指令
            RegisterIndex target_reg = register_allocator_.Allocate();
            generator_.EmitInstruction(OpCode::GETUPVAL, target_reg, upval.index, 0);
            return target_reg;
        }
    }
    
    // 全局变量
    RegisterIndex target_reg = register_allocator_.Allocate();
    int name_const = generator_.AddString(name);
    generator_.EmitInstruction(OpCode::GETGLOBAL, target_reg, name_const);
    return target_reg;
}

void CompilerContext::AssignVariable(const std::string& name, RegisterIndex value_reg) {
    // 首先查找局部变量
    const LocalVariable* local = scope_manager_.FindLocal(name);
    if (local) {
        generator_.EmitInstruction(OpCode::MOVE, local->register_idx, value_reg, 0);
        return;
    }
    
    // 查找上值
    for (const auto& upval : generator_.GetUpvalues()) {
        if (upval.name == name) {
            generator_.EmitInstruction(OpCode::SETUPVAL, value_reg, upval.index, 0);
            return;
        }
    }
    
    // 全局变量
    int name_const = generator_.AddString(name);
    generator_.EmitInstruction(OpCode::SETGLOBAL, value_reg, name_const);
}

RegisterIndex CompilerContext::CompileConstant(const LuaValue& value) {
    RegisterIndex target_reg = register_allocator_.Allocate();
    
    if (value.IsNil()) {
        generator_.EmitInstruction(OpCode::LOADNIL, target_reg, target_reg, 0);
    } else if (value.IsBool()) {
        int bool_val = value.AsBool() ? 1 : 0;
        generator_.EmitInstruction(OpCode::LOADBOOL, target_reg, bool_val, 0);
    } else {
        int const_idx = generator_.AddConstant(value);
        generator_.EmitInstruction(OpCode::LOADK, target_reg, const_idx);
    }
    
    return target_reg;
}

/* ========================================================================== */
/* 表达式编译器实现 */
/* ========================================================================== */

ExpressionCompiler::ExpressionCompiler(CompilerContext& context)
    : context_(context) {
}

RegisterIndex ExpressionCompiler::Compile(const Expression* expr) {
    if (!expr) {
        throw CompilerError("Null expression");
    }
    
    context_.GetGenerator().SetCurrentLine(expr->GetPosition().line);
    
    switch (expr->GetType()) {
        case ASTNodeType::NilLiteral:
            return CompileNilLiteral(static_cast<const NilLiteral*>(expr));
        case ASTNodeType::BooleanLiteral:
            return CompileBooleanLiteral(static_cast<const BooleanLiteral*>(expr));
        case ASTNodeType::NumberLiteral:
            return CompileNumberLiteral(static_cast<const NumberLiteral*>(expr));
        case ASTNodeType::StringLiteral:
            return CompileStringLiteral(static_cast<const StringLiteral*>(expr));
        case ASTNodeType::Identifier:
            return CompileIdentifier(static_cast<const Identifier*>(expr));
        case ASTNodeType::BinaryExpression:
            return CompileBinaryExpression(static_cast<const BinaryExpression*>(expr));
        case ASTNodeType::UnaryExpression:
            return CompileUnaryExpression(static_cast<const UnaryExpression*>(expr));
        case ASTNodeType::CallExpression:
            return CompileCallExpression(static_cast<const CallExpression*>(expr));
        case ASTNodeType::IndexExpression:
            return CompileIndexExpression(static_cast<const IndexExpression*>(expr));
        case ASTNodeType::MemberExpression:
            return CompileMemberExpression(static_cast<const MemberExpression*>(expr));
        case ASTNodeType::TableConstructor:
            return CompileTableConstructor(static_cast<const TableConstructor*>(expr));
        default:
            throw CompilerError("Unsupported expression type");
    }
}

RegisterIndex ExpressionCompiler::CompileNilLiteral(const NilLiteral* expr) {
    return context_.CompileConstant(LuaValue());
}

RegisterIndex ExpressionCompiler::CompileBooleanLiteral(const BooleanLiteral* expr) {
    return context_.CompileConstant(LuaValue(expr->GetValue()));
}

RegisterIndex ExpressionCompiler::CompileNumberLiteral(const NumberLiteral* expr) {
    return context_.CompileConstant(LuaValue(expr->GetValue()));
}

RegisterIndex ExpressionCompiler::CompileStringLiteral(const StringLiteral* expr) {
    return context_.CompileConstant(LuaValue(expr->GetValue()));
}

RegisterIndex ExpressionCompiler::CompileIdentifier(const Identifier* expr) {
    return context_.ResolveVariable(expr->GetName());
}

RegisterIndex ExpressionCompiler::CompileBinaryExpression(const BinaryExpression* expr) {
    BinaryOperator op = expr->GetOperator();
    
    // 处理短路运算符
    if (op == BinaryOperator::And || op == BinaryOperator::Or) {
        return CompileShortCircuitExpression(expr);
    }
    
    // 编译操作数
    RegisterIndex left_reg = Compile(expr->GetLeft());
    RegisterIndex right_reg = Compile(expr->GetRight());
    RegisterIndex result_reg = context_.GetRegisterAllocator().Allocate();
    
    // 生成对应的指令
    OpCode opcode = BinaryOperatorToOpCode(op);
    context_.GetGenerator().EmitInstruction(opcode, result_reg, left_reg, right_reg);
    
    // 释放临时寄存器
    context_.GetRegisterAllocator().Free(left_reg);
    context_.GetRegisterAllocator().Free(right_reg);
    
    return result_reg;
}

RegisterIndex ExpressionCompiler::CompileShortCircuitExpression(const BinaryExpression* expr) {
    RegisterIndex left_reg = Compile(expr->GetLeft());
    RegisterIndex result_reg = context_.GetRegisterAllocator().Allocate();
    
    // 移动左操作数到结果寄存器
    context_.GetGenerator().EmitInstruction(OpCode::MOVE, result_reg, left_reg, 0);
    
    // 生成条件跳转
    Size jump_pc;
    if (expr->GetOperator() == BinaryOperator::And) {
        // and: 如果左操作数为false，跳过右操作数
        context_.GetGenerator().EmitInstruction(OpCode::TEST, result_reg, 0, 0);
        jump_pc = context_.GetGenerator().EmitJump(JumpDirection::Forward);
    } else {
        // or: 如果左操作数为true，跳过右操作数
        context_.GetGenerator().EmitInstruction(OpCode::TEST, result_reg, 0, 1);
        jump_pc = context_.GetGenerator().EmitJump(JumpDirection::Forward);
    }
    
    // 编译右操作数
    RegisterIndex right_reg = Compile(expr->GetRight());
    context_.GetGenerator().EmitInstruction(OpCode::MOVE, result_reg, right_reg, 0);
    
    // 修正跳转目标
    Size current_pc = context_.GetGenerator().GetCurrentPC();
    context_.GetGenerator().PatchJump(jump_pc, static_cast<int>(current_pc - jump_pc - 1));
    
    context_.GetRegisterAllocator().Free(left_reg);
    context_.GetRegisterAllocator().Free(right_reg);
    
    return result_reg;
}

RegisterIndex ExpressionCompiler::CompileUnaryExpression(const UnaryExpression* expr) {
    RegisterIndex operand_reg = Compile(expr->GetOperand());
    RegisterIndex result_reg = context_.GetRegisterAllocator().Allocate();
    
    OpCode opcode;
    switch (expr->GetOperator()) {
        case UnaryOperator::Minus:
            opcode = OpCode::UNM;
            break;
        case UnaryOperator::Not:
            opcode = OpCode::NOT;
            break;
        case UnaryOperator::Length:
            opcode = OpCode::LEN;
            break;
        default:
            throw CompilerError("Unsupported unary operator");
    }
    
    context_.GetGenerator().EmitInstruction(opcode, result_reg, operand_reg, 0);
    context_.GetRegisterAllocator().Free(operand_reg);
    
    return result_reg;
}

RegisterIndex ExpressionCompiler::CompileCallExpression(const CallExpression* expr) {
    // 编译函数表达式
    RegisterIndex func_reg = Compile(expr->GetCallee());
    
    // 编译参数
    std::vector<RegisterIndex> arg_regs;
    for (Size i = 0; i < expr->GetArgumentCount(); ++i) {
        RegisterIndex arg_reg = Compile(expr->GetArgument(i));
        arg_regs.push_back(arg_reg);
    }
    
    // 分配结果寄存器
    RegisterIndex result_reg = context_.GetRegisterAllocator().Allocate();
    
    // 生成 CALL 指令
    int num_args = static_cast<int>(arg_regs.size());
    context_.GetGenerator().EmitInstruction(OpCode::CALL, func_reg, num_args + 1, 2); // +1包含函数本身，2表示返回1个值
    
    // 移动结果
    context_.GetGenerator().EmitInstruction(OpCode::MOVE, result_reg, func_reg, 0);
    
    // 释放寄存器
    context_.GetRegisterAllocator().Free(func_reg);
    for (RegisterIndex arg_reg : arg_regs) {
        context_.GetRegisterAllocator().Free(arg_reg);
    }
    
    return result_reg;
}

RegisterIndex ExpressionCompiler::CompileIndexExpression(const IndexExpression* expr) {
    RegisterIndex object_reg = Compile(expr->GetObject());
    RegisterIndex index_reg = Compile(expr->GetIndex());
    RegisterIndex result_reg = context_.GetRegisterAllocator().Allocate();
    
    context_.GetGenerator().EmitInstruction(OpCode::GETTABLE, result_reg, object_reg, index_reg);
    
    context_.GetRegisterAllocator().Free(object_reg);
    context_.GetRegisterAllocator().Free(index_reg);
    
    return result_reg;
}

RegisterIndex ExpressionCompiler::CompileMemberExpression(const MemberExpression* expr) {
    RegisterIndex object_reg = Compile(expr->GetObject());
    RegisterIndex result_reg = context_.GetRegisterAllocator().Allocate();
    
    // 成员访问转换为字符串索引
    int member_const = context_.GetGenerator().AddString(expr->GetProperty());
    context_.GetGenerator().EmitInstruction(OpCode::GETTABLE, result_reg, object_reg, member_const | 0x100); // RK(C)
    
    context_.GetRegisterAllocator().Free(object_reg);
    
    return result_reg;
}

RegisterIndex ExpressionCompiler::CompileTableConstructor(const TableConstructor* expr) {
    RegisterIndex table_reg = context_.GetRegisterAllocator().Allocate();
    
    // 创建新表
    context_.GetGenerator().EmitInstruction(OpCode::NEWTABLE, table_reg, 0, 0);
    
    // 处理表字段
    int array_index = 1; // Lua数组从1开始
    for (Size i = 0; i < expr->GetFieldCount(); ++i) {
        const TableField* field = expr->GetField(i);
        
        if (field->GetKey()) {
            // 键值对字段
            RegisterIndex key_reg = Compile(field->GetKey());
            RegisterIndex value_reg = Compile(field->GetValue());
            context_.GetGenerator().EmitInstruction(OpCode::SETTABLE, table_reg, key_reg, value_reg);
            context_.GetRegisterAllocator().Free(key_reg);
            context_.GetRegisterAllocator().Free(value_reg);
        } else {
            // 数组字段
            RegisterIndex value_reg = Compile(field->GetValue());
            RegisterIndex index_reg = context_.CompileConstant(LuaValue(static_cast<double>(array_index++)));
            context_.GetGenerator().EmitInstruction(OpCode::SETTABLE, table_reg, index_reg, value_reg);
            context_.GetRegisterAllocator().Free(index_reg);
            context_.GetRegisterAllocator().Free(value_reg);
        }
    }
    
    return table_reg;
}

OpCode ExpressionCompiler::BinaryOperatorToOpCode(BinaryOperator op) {
    switch (op) {
        case BinaryOperator::Add: return OpCode::ADD;
        case BinaryOperator::Subtract: return OpCode::SUB;
        case BinaryOperator::Multiply: return OpCode::MUL;
        case BinaryOperator::Divide: return OpCode::DIV;
        case BinaryOperator::Modulo: return OpCode::MOD;
        case BinaryOperator::Power: return OpCode::POW;
        case BinaryOperator::Concat: return OpCode::CONCAT;
        case BinaryOperator::Equal: return OpCode::EQ;
        case BinaryOperator::NotEqual: return OpCode::EQ; // 需要反转结果
        case BinaryOperator::Less: return OpCode::LT;
        case BinaryOperator::LessEqual: return OpCode::LE;
        case BinaryOperator::Greater: return OpCode::LT; // 交换操作数
        case BinaryOperator::GreaterEqual: return OpCode::LE; // 交换操作数
        default:
            throw CompilerError("Unsupported binary operator");
    }
}

/* ========================================================================== */
/* 语句编译器实现 */
/* ========================================================================== */

class StatementCompiler {
public:
    StatementCompiler(BytecodeGenerator& bytecode_gen, CompilerContext& context)
        : bytecode_gen_(bytecode_gen), context_(context), expr_compiler_(bytecode_gen, context) {
    }
    
    void CompileStatement(const std::shared_ptr<Statement>& stmt) {
        switch (stmt->GetType()) {
            case StatementType::ExpressionStatement:
                CompileExpressionStatement(std::static_pointer_cast<ExpressionStatement>(stmt));
                break;
            case StatementType::AssignmentStatement:
                CompileAssignmentStatement(std::static_pointer_cast<AssignmentStatement>(stmt));
                break;
            case StatementType::LocalStatement:
                CompileLocalStatement(std::static_pointer_cast<LocalStatement>(stmt));
                break;
            case StatementType::IfStatement:
                CompileIfStatement(std::static_pointer_cast<IfStatement>(stmt));
                break;
            case StatementType::WhileStatement:
                CompileWhileStatement(std::static_pointer_cast<WhileStatement>(stmt));
                break;
            case StatementType::ForStatement:
                CompileForStatement(std::static_pointer_cast<ForStatement>(stmt));
                break;
            case StatementType::FunctionStatement:
                CompileFunctionStatement(std::static_pointer_cast<FunctionStatement>(stmt));
                break;
            case StatementType::ReturnStatement:
                CompileReturnStatement(std::static_pointer_cast<ReturnStatement>(stmt));
                break;
            case StatementType::BreakStatement:
                CompileBreakStatement(std::static_pointer_cast<BreakStatement>(stmt));
                break;
            case StatementType::BlockStatement:
                CompileBlockStatement(std::static_pointer_cast<BlockStatement>(stmt));
                break;
        }
    }

private:
    BytecodeGenerator& bytecode_gen_;
    CompilerContext& context_;
    ExpressionCompiler expr_compiler_;
    
    void CompileExpressionStatement(const std::shared_ptr<ExpressionStatement>& stmt) {
        // 编译表达式但不保存结果
        RegisterIndex temp_reg = context_.AllocateRegister();
        expr_compiler_.CompileExpression(stmt->GetExpression(), temp_reg);
        context_.FreeRegister(temp_reg);
    }
    
    void CompileAssignmentStatement(const std::shared_ptr<AssignmentStatement>& stmt) {
        const auto& targets = stmt->GetTargets();
        const auto& values = stmt->GetValues();
        
        // 先编译所有右值
        std::vector<RegisterIndex> value_regs;
        for (const auto& value : values) {
            RegisterIndex reg = context_.AllocateRegister();
            expr_compiler_.CompileExpression(value, reg);
            value_regs.push_back(reg);
        }
        
        // 然后赋值给左值
        for (Size i = 0; i < targets.size(); ++i) {
            RegisterIndex value_reg = (i < value_regs.size()) ? value_regs[i] : value_regs.back();
            CompileAssignmentTarget(targets[i], value_reg);
        }
        
        // 释放临时寄存器
        for (RegisterIndex reg : value_regs) {
            context_.FreeRegister(reg);
        }
    }
    
    void CompileAssignmentTarget(const std::shared_ptr<Expression>& target, RegisterIndex value_reg) {
        switch (target->GetType()) {
            case ExpressionType::Variable: {
                auto var_expr = std::static_pointer_cast<VariableExpression>(target);
                const std::string& name = var_expr->GetName();
                
                // 检查是否是局部变量
                if (auto local_reg = context_.FindLocal(name)) {
                    // MOVE dst, src
                    bytecode_gen_.EmitABC(OpCode::MOVE, *local_reg, value_reg, 0);
                } else {
                    // 全局变量：SETGLOBAL
                    int const_idx = context_.AddConstant(LuaValue(name));
                    bytecode_gen_.EmitABx(OpCode::SETGLOBAL, value_reg, const_idx);
                }
                break;
            }
            case ExpressionType::IndexExpression: {
                auto index_expr = std::static_pointer_cast<IndexExpression>(target);
                
                RegisterIndex table_reg = context_.AllocateRegister();
                RegisterIndex key_reg = context_.AllocateRegister();
                
                expr_compiler_.CompileExpression(index_expr->GetObject(), table_reg);
                expr_compiler_.CompileExpression(index_expr->GetIndex(), key_reg);
                
                // SETTABLE table, key, value
                bytecode_gen_.EmitABC(OpCode::SETTABLE, table_reg, key_reg, value_reg);
                
                context_.FreeRegister(table_reg);
                context_.FreeRegister(key_reg);
                break;
            }
            case ExpressionType::MemberExpression: {
                auto member_expr = std::static_pointer_cast<MemberExpression>(target);
                
                RegisterIndex table_reg = context_.AllocateRegister();
                expr_compiler_.CompileExpression(member_expr->GetObject(), table_reg);
                
                // 成员名作为常量
                int const_idx = context_.AddConstant(LuaValue(member_expr->GetProperty()));
                
                // SETTABLE table, const_key, value
                bytecode_gen_.EmitABC(OpCode::SETTABLE, table_reg, const_idx + 256, value_reg);
                
                context_.FreeRegister(table_reg);
                break;
            }
        }
    }
    
    void CompileLocalStatement(const std::shared_ptr<LocalStatement>& stmt) {
        const auto& names = stmt->GetNames();
        const auto& values = stmt->GetValues();
        
        // 先编译所有初值
        std::vector<RegisterIndex> value_regs;
        for (const auto& value : values) {
            RegisterIndex reg = context_.AllocateRegister();
            expr_compiler_.CompileExpression(value, reg);
            value_regs.push_back(reg);
        }
        
        // 声明局部变量并赋值
        for (Size i = 0; i < names.size(); ++i) {
            RegisterIndex local_reg = context_.DeclareLocal(names[i]);
            
            if (i < value_regs.size()) {
                // 有对应的初值
                bytecode_gen_.EmitABC(OpCode::MOVE, local_reg, value_regs[i], 0);
            } else {
                // 没有初值，设为nil
                bytecode_gen_.EmitABC(OpCode::LOADNIL, local_reg, 0, 0);
            }
        }
        
        // 释放临时寄存器
        for (RegisterIndex reg : value_regs) {
            context_.FreeRegister(reg);
        }
    }
    
    void CompileIfStatement(const std::shared_ptr<IfStatement>& stmt) {
        RegisterIndex condition_reg = context_.AllocateRegister();
        expr_compiler_.CompileExpression(stmt->GetCondition(), condition_reg);
        
        // TEST condition_reg, 0
        bytecode_gen_.EmitABC(OpCode::TEST, condition_reg, 0, 0);
        
        // JMP to else/end
        int jmp_to_else = bytecode_gen_.EmitJump();
        
        // 编译then块
        CompileStatement(stmt->GetThenStatement());
        
        int jmp_to_end = -1;
        if (stmt->GetElseStatement()) {
            // JMP to end
            jmp_to_end = bytecode_gen_.EmitJump();
        }
        
        // 回填跳转到else的地址
        bytecode_gen_.PatchJump(jmp_to_else);
        
        // 编译else块
        if (stmt->GetElseStatement()) {
            CompileStatement(stmt->GetElseStatement());
            // 回填跳转到end的地址
            bytecode_gen_.PatchJump(jmp_to_end);
        }
        
        context_.FreeRegister(condition_reg);
    }
    
    void CompileWhileStatement(const std::shared_ptr<WhileStatement>& stmt) {
        // 保存循环开始位置
        int loop_start = bytecode_gen_.GetCurrentPC();
        
        // 编译条件
        RegisterIndex condition_reg = context_.AllocateRegister();
        expr_compiler_.CompileExpression(stmt->GetCondition(), condition_reg);
        
        // TEST condition_reg, 0
        bytecode_gen_.EmitABC(OpCode::TEST, condition_reg, 0, 0);
        
        // JMP to end
        int jmp_to_end = bytecode_gen_.EmitJump();
        
        // 编译循环体
        CompileStatement(stmt->GetBody());
        
        // JMP back to start
        int offset = loop_start - (bytecode_gen_.GetCurrentPC() + 1);
        bytecode_gen_.EmitAsBx(OpCode::JMP, 0, offset);
        
        // 回填跳转到end的地址
        bytecode_gen_.PatchJump(jmp_to_end);
        
        context_.FreeRegister(condition_reg);
    }
    
    void CompileForStatement(const std::shared_ptr<ForStatement>& stmt) {
        if (stmt->IsNumericFor()) {
            CompileNumericFor(stmt);
        } else {
            CompileGenericFor(stmt);
        }
    }
    
    void CompileNumericFor(const std::shared_ptr<ForStatement>& stmt) {
        // 为数值for循环分配三个寄存器：初值、限值、步长
        RegisterIndex base_reg = context_.AllocateRegister();
        RegisterIndex limit_reg = context_.AllocateRegister();
        RegisterIndex step_reg = context_.AllocateRegister();
        
        // 编译初值、限值、步长
        expr_compiler_.CompileExpression(stmt->GetInit(), base_reg);
        expr_compiler_.CompileExpression(stmt->GetLimit(), limit_reg);
        
        if (stmt->GetStep()) {
            expr_compiler_.CompileExpression(stmt->GetStep(), step_reg);
        } else {
            // 默认步长为1
            int const_idx = context_.AddConstant(LuaValue(1.0));
            bytecode_gen_.EmitABx(OpCode::LOADK, step_reg, const_idx);
        }
        
        // FORPREP base_reg, jump_to_end
        int forprep_jump = bytecode_gen_.EmitAsBx(OpCode::FORPREP, base_reg, 0);
        
        // 循环变量
        RegisterIndex loop_var = context_.DeclareLocal(stmt->GetVariable());
        
        // 编译循环体
        CompileStatement(stmt->GetBody());
        
        // FORLOOP base_reg, jump_to_start
        int loop_start_offset = forprep_jump + 1 - (bytecode_gen_.GetCurrentPC() + 1);
        bytecode_gen_.EmitAsBx(OpCode::FORLOOP, base_reg, loop_start_offset);
        
        // 回填FORPREP的跳转地址
        bytecode_gen_.PatchJump(forprep_jump);
        
        context_.FreeRegister(base_reg);
        context_.FreeRegister(limit_reg);
        context_.FreeRegister(step_reg);
    }
    
    void CompileGenericFor(const std::shared_ptr<ForStatement>& stmt) {
        // TODO: 实现通用for循环（for in）
        // 这需要实现迭代器协议
    }
    
    void CompileFunctionStatement(const std::shared_ptr<FunctionStatement>& stmt) {
        // TODO: 实现函数定义编译
        // 这需要创建新的函数原型并编译函数体
    }
    
    void CompileReturnStatement(const std::shared_ptr<ReturnStatement>& stmt) {
        const auto& values = stmt->GetValues();
        
        if (values.empty()) {
            // 无返回值：RETURN 0, 1
            bytecode_gen_.EmitABC(OpCode::RETURN, 0, 1, 0);
        } else {
            // 编译返回值到连续寄存器
            RegisterIndex base_reg = context_.AllocateRegister();
            
            for (Size i = 0; i < values.size(); ++i) {
                RegisterIndex reg = base_reg + static_cast<RegisterIndex>(i);
                expr_compiler_.CompileExpression(values[i], reg);
            }
            
            // RETURN base_reg, count+1
            bytecode_gen_.EmitABC(OpCode::RETURN, base_reg, 
                                  static_cast<int>(values.size()) + 1, 0);
        }
    }
    
    void CompileBreakStatement(const std::shared_ptr<BreakStatement>& stmt) {
        // 跳转到循环结束位置
        // TODO: 需要实现break标签栈来正确处理嵌套循环
        bytecode_gen_.EmitJump();
    }
    
    void CompileBlockStatement(const std::shared_ptr<BlockStatement>& stmt) {
        // 进入新作用域
        context_.EnterScope();
        
        // 编译块中的所有语句
        for (const auto& statement : stmt->GetStatements()) {
            CompileStatement(statement);
        }
        
        // 退出作用域
        context_.ExitScope();
    }
};

/* ========================================================================== */
/* 主编译器类实现 */
/* ========================================================================== */

class MainCompiler {
public:
    MainCompiler() : context_(), bytecode_gen_(), stmt_compiler_(bytecode_gen_, context_) {
    }
    
    CompiledFunction Compile(const std::shared_ptr<Program>& program) {
        // 编译主函数
        for (const auto& stmt : program->GetStatements()) {
            stmt_compiler_.CompileStatement(stmt);
        }
        
        // 添加隐式返回
        bytecode_gen_.EmitABC(OpCode::RETURN, 0, 1, 0);
        
        // 创建编译结果
        CompiledFunction result;
        result.instructions = bytecode_gen_.GetInstructions();
        result.constants = context_.GetConstants();
        result.line_info = bytecode_gen_.GetLineInfo();
        result.max_stack_size = context_.GetMaxRegisters();
        result.num_params = 0; // 主函数无参数
        result.is_vararg = true; // 主函数支持变参
        
        return result;
    }

private:
    CompilerContext context_;
    BytecodeGenerator bytecode_gen_;
    StatementCompiler stmt_compiler_;
};

/* ========================================================================== */
/* Compiler类实现 */
/* ========================================================================== */

Compiler::Compiler(const OptimizationConfig& config, bool strict_mode)
    : config_(config), strict_mode_(strict_mode) {
}

std::unique_ptr<Proto> Compiler::CompileProgram(const Program* program, 
                                                const std::string& source_name) {
    // 创建主编译器
    MainCompiler main_compiler;
    
    // 转换为共享指针（临时）
    auto program_ptr = std::shared_ptr<Program>(const_cast<Program*>(program), [](Program*){});
    
    // 编译程序
    CompiledFunction compiled = main_compiler.Compile(program_ptr);
    
    // 创建Proto对象
    auto proto = std::make_unique<Proto>();
    proto->instructions = std::move(compiled.instructions);
    proto->constants = std::move(compiled.constants);
    proto->line_info = std::move(compiled.line_info);
    proto->max_stack_size = compiled.max_stack_size;
    proto->num_params = compiled.num_params;
    proto->is_vararg = compiled.is_vararg;
    proto->source_name = source_name;
    
    // 执行优化
    if (config_.IsEnabled(OptimizationType::ConstantFolding) ||
        config_.IsEnabled(OptimizationType::DeadCodeElimination) ||
        config_.IsEnabled(OptimizationType::JumpOptimization)) {
        
        BytecodeOptimizer optimizer(config_);
        optimizer.Optimize(proto->instructions, proto->constants, proto->line_info);
    }
    
    return proto;
}

void Compiler::BeginFunction(const std::string& name, 
                            const std::vector<std::string>& parameters,
                            bool is_vararg) {
    // TODO: 实现函数编译栈管理
    // 这需要支持嵌套函数编译
}

std::unique_ptr<Proto> Compiler::EndFunction() {
    // TODO: 实现函数编译结束
    return nullptr;
}

Proto* Compiler::GetCurrentFunction() {
    // TODO: 返回当前编译的函数
    return nullptr;
}

const Proto* Compiler::GetCurrentFunction() const {
    // TODO: 返回当前编译的函数（只读）
    return nullptr;
}

ExpressionContext Compiler::CompileExpression(const Expression* expr) {
    if (!expr) {
        throw CompilerError("Cannot compile null expression");
    }
    
    ExpressionCompiler compiler(bytecode_generator_, context_);
    RegisterIndex reg = compiler.Compile(expr);
    
    return ExpressionContext{reg, false, false}; // register, not constant, not temporary
}

ExpressionContext Compiler::CompileExpressionToRegister(const Expression* expr, 
                                                       RegisterIndex target_reg) {
    if (!expr) {
        throw CompilerError("Cannot compile null expression");
    }
    
    RegisterIndex source_reg = CompileExpression(expr).register_idx;
    
    if (source_reg != target_reg) {
        // 需要移动到目标寄存器
        bytecode_generator_.EmitABC(OpCode::MOVE, target_reg, source_reg, 0);
        register_allocator_.Free(source_reg);
    }
    
    return ExpressionContext{target_reg, false, false};
}

int Compiler::CompileExpressionAsRK(const Expression* expr) {
    if (!expr) {
        throw CompilerError("Cannot compile null expression");
    }
    
    // 尝试将表达式编译为常量
    if (expr->IsConstant()) {
        LuaValue constant_value = expr->EvaluateConstant();
        int const_idx = constant_pool_.FindConstant(constant_value);
        if (const_idx >= 0) {
            return ConstantIndexToRK(const_idx);
        } else {
            const_idx = constant_pool_.AddConstant(constant_value);
            return ConstantIndexToRK(const_idx);
        }
    }
    
    // 否则编译到寄存器
    RegisterIndex reg = CompileExpression(expr).register_idx;
    return static_cast<int>(reg); // 寄存器直接作为RK值
}

void Compiler::CompileCondition(const Expression* expr, 
                               std::vector<int>& true_jumps,
                               std::vector<int>& false_jumps) {
    if (!expr) {
        throw CompilerError("Cannot compile null condition");
    }
    
    // 编译条件表达式到寄存器
    RegisterIndex condition_reg = CompileExpression(expr).register_idx;
    
    // 生成TEST指令来测试条件
    // TEST A C - 如果C和(RK(A) != 0) != bool(C)，跳过下一条指令
    Size pc = bytecode_generator_.EmitABC(OpCode::TEST, condition_reg, 0, 1); // 测试为真
    
    // 如果条件为假，跳转
    int jump_pc = bytecode_generator_.EmitJump(OpCode::JMP);
    false_jumps.push_back(jump_pc);
    
    register_allocator_.Free(condition_reg);
}

void Compiler::CompileStatement(const Statement* stmt) {
    if (!stmt) {
        return; // 允许空语句
    }
    
    StatementCompiler compiler(bytecode_generator_, context_);
    
    // 根据语句类型分发编译
    switch (stmt->GetType()) {
        case StatementType::ExpressionStatement: {
            const ExpressionStatement* expr_stmt = static_cast<const ExpressionStatement*>(stmt);
            RegisterIndex reg = CompileExpression(expr_stmt->GetExpression()).register_idx;
            register_allocator_.Free(reg); // 表达式语句的结果不需要保留
            break;
        }
        case StatementType::Block: {
            const BlockStatement* block = static_cast<const BlockStatement*>(stmt);
            scope_manager_.EnterScope();
            for (const auto& child_stmt : block->GetStatements()) {
                CompileStatement(child_stmt.get());
            }
            scope_manager_.ExitScope();
            break;
        }
        case StatementType::Assignment: {
            const AssignmentStatement* assign = static_cast<const AssignmentStatement*>(stmt);
            compiler.CompileAssignmentStatement(assign);
            break;
        }
        case StatementType::LocalDeclaration: {
            const LocalDeclarationStatement* local_decl = static_cast<const LocalDeclarationStatement*>(stmt);
            compiler.CompileLocalDeclarationStatement(local_decl);
            break;
        }
        case StatementType::If: {
            const IfStatement* if_stmt = static_cast<const IfStatement*>(stmt);
            compiler.CompileIfStatement(if_stmt);
            break;
        }
        case StatementType::While: {
            const WhileStatement* while_stmt = static_cast<const WhileStatement*>(stmt);
            compiler.CompileWhileStatement(while_stmt);
            break;
        }
        case StatementType::For: {
            const ForStatement* for_stmt = static_cast<const ForStatement*>(stmt);
            compiler.CompileForStatement(for_stmt);
            break;
        }
        case StatementType::Return: {
            const ReturnStatement* return_stmt = static_cast<const ReturnStatement*>(stmt);
            compiler.CompileReturnStatement(return_stmt);
            break;
        }
        case StatementType::Break: {
            compiler.CompileBreakStatement();
            break;
        }
        case StatementType::Continue: {
            compiler.CompileContinueStatement();
            break;
        }
        default:
            throw CompilerError("Unsupported statement type");
    }
}

RegisterIndex Compiler::AllocateRegister() {
    return register_allocator_.Allocate();
}

void Compiler::FreeRegister(RegisterIndex reg) {
    register_allocator_.Free(reg);
}

RegisterIndex Compiler::AllocateTemporary() {
    return register_allocator_.AllocateTemporary();
}

void Compiler::FreeTemporaries(Size saved_top) {
    register_allocator_.RestoreTempTop();
}

Size Compiler::GetRegisterTop() const {
    return register_allocator_.GetTop();
}

void Compiler::SetRegisterTop(Size top) {
    register_allocator_.SetTop(top);
}

RegisterIndex Compiler::DeclareLocal(const std::string& name) {
    RegisterIndex reg = register_allocator_.AllocateNamed(name);
    scope_manager_.DeclareLocal(name, reg);
    return reg;
}

std::optional<RegisterIndex> Compiler::FindLocal(const std::string& name) const {
    const LocalVariable* local = scope_manager_.FindLocal(name);
    if (local) {
        return local->register_idx;
    }
    return std::nullopt;
}

void Compiler::EnterScope() {
    scope_manager_.EnterScope();
}

void Compiler::ExitScope() {
    scope_manager_.ExitScope();
}

int Compiler::AddConstant(const LuaValue& value) {
    return constant_pool_.AddConstant(value);
}

const std::vector<LuaValue>& Compiler::GetConstants() const {
    return constant_pool_.GetConstants();
}

int Compiler::EmitInstruction(Instruction inst) {
    Size pc = bytecode_generator_.GetCurrentPC();
    bytecode_generator_.AddInstruction(inst);
    return static_cast<int>(pc);
}

int Compiler::EmitABC(OpCode op, RegisterIndex a, int b, int c) {
    Size pc = bytecode_generator_.EmitABC(op, a, b, c);
    return static_cast<int>(pc);
}

int Compiler::EmitABx(OpCode op, RegisterIndex a, int bx) {
    Size pc = bytecode_generator_.EmitABx(op, a, bx);
    return static_cast<int>(pc);
}

int Compiler::EmitAsBx(OpCode op, RegisterIndex a, int sbx) {
    Size pc = bytecode_generator_.EmitAsBx(op, a, sbx);
    return static_cast<int>(pc);
}

int Compiler::EmitJump() {
    Size pc = bytecode_generator_.EmitJump(OpCode::JMP);
    return static_cast<int>(pc);
}

void Compiler::PatchJump(int jump_pc) {
    Size current_pc = bytecode_generator_.GetCurrentPC();
    bytecode_generator_.PatchJump(static_cast<Size>(jump_pc), current_pc);
}

void Compiler::PatchJumpToHere(int jump_pc) {
    bytecode_generator_.PatchJumpToHere(static_cast<Size>(jump_pc));
}

int Compiler::GetCurrentPC() const {
    return static_cast<int>(bytecode_generator_.GetCurrentPC());
}

void Compiler::SetLineInfo(int line) {
    bytecode_generator_.SetCurrentLine(static_cast<Size>(line));
}

} // namespace lua_cpp