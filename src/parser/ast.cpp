/**
 * @file ast.cpp
 * @brief Abstract Syntax Tree (AST) 节点实现
 * @description 实现Lua语法树的所有节点类型，支持完整的Lua 5.1.5语法
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#include "ast.h"
#include <sstream>
#include <iostream>

namespace lua_cpp {

/* ========================================================================== */
/* 辅助函数实现 */
/* ========================================================================== */

bool IsArithmeticOperator(BinaryOperator op) {
    return op == BinaryOperator::Add || op == BinaryOperator::Subtract ||
           op == BinaryOperator::Multiply || op == BinaryOperator::Divide ||
           op == BinaryOperator::Modulo || op == BinaryOperator::Power;
}

bool IsRelationalOperator(BinaryOperator op) {
    return op == BinaryOperator::Equal || op == BinaryOperator::NotEqual ||
           op == BinaryOperator::Less || op == BinaryOperator::LessEqual ||
           op == BinaryOperator::Greater || op == BinaryOperator::GreaterEqual;
}

bool IsLogicalOperator(BinaryOperator op) {
    return op == BinaryOperator::And || op == BinaryOperator::Or;
}

/* ========================================================================== */
/* AST节点基类实现 */
/* ========================================================================== */

ASTNode::ASTNode(ASTNodeType type, const SourcePosition& position)
    : type_(type), position_(position) {
}

ASTNode* ASTNode::GetChild(Size index) const {
    if (index >= children_.size()) {
        return nullptr;
    }
    return children_[index].get();
}

void ASTNode::AddChild(std::unique_ptr<ASTNode> child) {
    if (child) {
        child->SetParent(this);
        children_.push_back(std::move(child));
    }
}

void ASTNode::RemoveChild(Size index) {
    if (index < children_.size()) {
        children_[index]->SetParent(nullptr);
        children_.erase(children_.begin() + index);
    }
}

void ASTNode::ReplaceChild(Size index, std::unique_ptr<ASTNode> new_child) {
    if (index < children_.size() && new_child) {
        children_[index]->SetParent(nullptr);
        new_child->SetParent(this);
        children_[index] = std::move(new_child);
    }
}

std::string ASTNode::ToString() const {
    switch (type_) {
        case ASTNodeType::Program: return "Program";
        case ASTNodeType::Block: return "Block";
        case ASTNodeType::NilLiteral: return "nil";
        case ASTNodeType::BooleanLiteral: return "BooleanLiteral";
        case ASTNodeType::NumberLiteral: return "NumberLiteral";
        case ASTNodeType::StringLiteral: return "StringLiteral";
        case ASTNodeType::VarargLiteral: return "...";
        case ASTNodeType::Identifier: return "Identifier";
        case ASTNodeType::IndexExpression: return "IndexExpression";
        case ASTNodeType::MemberExpression: return "MemberExpression";
        case ASTNodeType::BinaryExpression: return "BinaryExpression";
        case ASTNodeType::UnaryExpression: return "UnaryExpression";
        case ASTNodeType::CallExpression: return "CallExpression";
        case ASTNodeType::MethodCallExpression: return "MethodCallExpression";
        case ASTNodeType::TableConstructor: return "TableConstructor";
        case ASTNodeType::TableField: return "TableField";
        case ASTNodeType::FunctionExpression: return "FunctionExpression";
        case ASTNodeType::AssignmentStatement: return "AssignmentStatement";
        case ASTNodeType::LocalDeclaration: return "LocalDeclaration";
        case ASTNodeType::IfStatement: return "IfStatement";
        case ASTNodeType::WhileStatement: return "WhileStatement";
        case ASTNodeType::RepeatStatement: return "RepeatStatement";
        case ASTNodeType::NumericForStatement: return "NumericForStatement";
        case ASTNodeType::GenericForStatement: return "GenericForStatement";
        case ASTNodeType::BreakStatement: return "BreakStatement";
        case ASTNodeType::FunctionDefinition: return "FunctionDefinition";
        case ASTNodeType::LocalFunctionDefinition: return "LocalFunctionDefinition";
        case ASTNodeType::ReturnStatement: return "ReturnStatement";
        case ASTNodeType::ExpressionStatement: return "ExpressionStatement";
        case ASTNodeType::DoStatement: return "DoStatement";
        default: return "UnknownNode";
    }
}

void ASTNode::PrintTree(int indent) const {
    std::string prefix(indent * 2, ' ');
    std::cout << prefix << ToString() << " [" << position_.line << ":" << position_.column << "]" << std::endl;
    
    for (const auto& child : children_) {
        if (child) {
            child->PrintTree(indent + 1);
        }
    }
}

/* ========================================================================== */
/* 表达式基类实现 */
/* ========================================================================== */

Expression::Expression(ASTNodeType type, const SourcePosition& position)
    : ASTNode(type, position) {
}

/* ========================================================================== */
/* 字面量表达式实现 */
/* ========================================================================== */

NilLiteral::NilLiteral(const SourcePosition& position)
    : Expression(ASTNodeType::NilLiteral, position) {
}

BooleanLiteral::BooleanLiteral(bool value, const SourcePosition& position)
    : Expression(ASTNodeType::BooleanLiteral, position), value_(value) {
}

NumberLiteral::NumberLiteral(double value, const SourcePosition& position)
    : Expression(ASTNodeType::NumberLiteral, position), value_(value) {
}

StringLiteral::StringLiteral(const std::string& value, const SourcePosition& position)
    : Expression(ASTNodeType::StringLiteral, position), value_(value) {
}

VarargLiteral::VarargLiteral(const SourcePosition& position)
    : Expression(ASTNodeType::VarargLiteral, position) {
}

/* ========================================================================== */
/* 变量表达式实现 */
/* ========================================================================== */

Identifier::Identifier(const std::string& name, const SourcePosition& position)
    : Expression(ASTNodeType::Identifier, position), name_(name) {
}

IndexExpression::IndexExpression(std::unique_ptr<Expression> object, 
                                 std::unique_ptr<Expression> index,
                                 const SourcePosition& position)
    : Expression(ASTNodeType::IndexExpression, position) {
    AddChild(std::move(object));
    AddChild(std::move(index));
}

Expression* IndexExpression::GetObject() const {
    return static_cast<Expression*>(GetChild(0));
}

Expression* IndexExpression::GetIndex() const {
    return static_cast<Expression*>(GetChild(1));
}

MemberExpression::MemberExpression(std::unique_ptr<Expression> object,
                                   const std::string& property,
                                   const SourcePosition& position)
    : Expression(ASTNodeType::MemberExpression, position), property_(property) {
    AddChild(std::move(object));
}

Expression* MemberExpression::GetObject() const {
    return static_cast<Expression*>(GetChild(0));
}

/* ========================================================================== */
/* 运算表达式实现 */
/* ========================================================================== */

BinaryExpression::BinaryExpression(BinaryOperator operator_,
                                   std::unique_ptr<Expression> left,
                                   std::unique_ptr<Expression> right,
                                   const SourcePosition& position)
    : Expression(ASTNodeType::BinaryExpression, position), operator_(operator_) {
    AddChild(std::move(left));
    AddChild(std::move(right));
}

Expression* BinaryExpression::GetLeft() const {
    return static_cast<Expression*>(GetChild(0));
}

Expression* BinaryExpression::GetRight() const {
    return static_cast<Expression*>(GetChild(1));
}

bool BinaryExpression::HasSideEffects() const {
    return GetLeft()->HasSideEffects() || GetRight()->HasSideEffects();
}

UnaryExpression::UnaryExpression(UnaryOperator operator_,
                                 std::unique_ptr<Expression> operand,
                                 const SourcePosition& position)
    : Expression(ASTNodeType::UnaryExpression, position), operator_(operator_) {
    AddChild(std::move(operand));
}

Expression* UnaryExpression::GetOperand() const {
    return static_cast<Expression*>(GetChild(0));
}

bool UnaryExpression::HasSideEffects() const {
    return GetOperand()->HasSideEffects();
}

/* ========================================================================== */
/* 调用表达式实现 */
/* ========================================================================== */

CallExpression::CallExpression(std::unique_ptr<Expression> callee,
                               std::vector<std::unique_ptr<Expression>> arguments,
                               const SourcePosition& position)
    : Expression(ASTNodeType::CallExpression, position) {
    AddChild(std::move(callee));
    for (auto& arg : arguments) {
        AddChild(std::move(arg));
    }
}

Expression* CallExpression::GetCallee() const {
    return static_cast<Expression*>(GetChild(0));
}

Size CallExpression::GetArgumentCount() const {
    return GetChildCount() > 0 ? GetChildCount() - 1 : 0;
}

Expression* CallExpression::GetArgument(Size index) const {
    return static_cast<Expression*>(GetChild(index + 1));
}

bool CallExpression::HasSideEffects() const {
    return true; // 函数调用总是可能有副作用
}

MethodCallExpression::MethodCallExpression(std::unique_ptr<Expression> object,
                                           const std::string& method,
                                           std::vector<std::unique_ptr<Expression>> arguments,
                                           const SourcePosition& position)
    : Expression(ASTNodeType::MethodCallExpression, position), method_(method) {
    AddChild(std::move(object));
    for (auto& arg : arguments) {
        AddChild(std::move(arg));
    }
}

Expression* MethodCallExpression::GetObject() const {
    return static_cast<Expression*>(GetChild(0));
}

Size MethodCallExpression::GetArgumentCount() const {
    return GetChildCount() > 0 ? GetChildCount() - 1 : 0;
}

Expression* MethodCallExpression::GetArgument(Size index) const {
    return static_cast<Expression*>(GetChild(index + 1));
}

bool MethodCallExpression::HasSideEffects() const {
    return true; // 方法调用总是可能有副作用
}

/* ========================================================================== */
/* 表构造和函数表达式实现 */
/* ========================================================================== */

TableConstructor::TableConstructor(std::vector<std::unique_ptr<TableField>> fields,
                                   const SourcePosition& position)
    : Expression(ASTNodeType::TableConstructor, position) {
    for (auto& field : fields) {
        AddChild(std::move(field));
    }
}

Size TableConstructor::GetFieldCount() const {
    return GetChildCount();
}

TableField* TableConstructor::GetField(Size index) const {
    return static_cast<TableField*>(GetChild(index));
}

void TableConstructor::AddField(std::unique_ptr<TableField> field) {
    AddChild(std::move(field));
}

TableField::TableField(std::unique_ptr<Expression> key,
                       std::unique_ptr<Expression> value,
                       const SourcePosition& position)
    : ASTNode(ASTNodeType::TableField, position) {
    if (key) {
        AddChild(std::move(key));
        has_key_ = true;
    } else {
        has_key_ = false;
    }
    AddChild(std::move(value));
}

Expression* TableField::GetKey() const {
    return has_key_ ? static_cast<Expression*>(GetChild(0)) : nullptr;
}

Expression* TableField::GetValue() const {
    return static_cast<Expression*>(GetChild(has_key_ ? 1 : 0));
}

FunctionExpression::FunctionExpression(std::vector<std::string> parameters,
                                       std::unique_ptr<BlockNode> body,
                                       bool is_vararg,
                                       const SourcePosition& position)
    : Expression(ASTNodeType::FunctionExpression, position),
      parameters_(std::move(parameters)),
      is_vararg_(is_vararg) {
    AddChild(std::move(body));
}

BlockNode* FunctionExpression::GetBody() const {
    return static_cast<BlockNode*>(GetChild(0));
}

/* ========================================================================== */
/* 语句基类和块节点实现 */
/* ========================================================================== */

Statement::Statement(ASTNodeType type, const SourcePosition& position)
    : ASTNode(type, position) {
}

BlockNode::BlockNode(std::vector<std::unique_ptr<Statement>> statements,
                     const SourcePosition& position)
    : Statement(ASTNodeType::Block, position) {
    for (auto& stmt : statements) {
        AddChild(std::move(stmt));
    }
}

Size BlockNode::GetStatementCount() const {
    return GetChildCount();
}

Statement* BlockNode::GetStatement(Size index) const {
    return static_cast<Statement*>(GetChild(index));
}

void BlockNode::AddStatement(std::unique_ptr<Statement> statement) {
    AddChild(std::move(statement));
}

/* ========================================================================== */
/* 赋值和声明语句实现 */
/* ========================================================================== */

AssignmentStatement::AssignmentStatement(std::vector<std::unique_ptr<Expression>> targets,
                                         std::vector<std::unique_ptr<Expression>> values,
                                         const SourcePosition& position)
    : Statement(ASTNodeType::AssignmentStatement, position) {
    // 先添加目标表达式
    for (auto& target : targets) {
        AddChild(std::move(target));
    }
    target_count_ = targets.size();
    
    // 再添加值表达式
    for (auto& value : values) {
        AddChild(std::move(value));
    }
}

Size AssignmentStatement::GetTargetCount() const {
    return target_count_;
}

Size AssignmentStatement::GetValueCount() const {
    return GetChildCount() - target_count_;
}

Expression* AssignmentStatement::GetTarget(Size index) const {
    return static_cast<Expression*>(GetChild(index));
}

Expression* AssignmentStatement::GetValue(Size index) const {
    return static_cast<Expression*>(GetChild(target_count_ + index));
}

LocalDeclaration::LocalDeclaration(std::vector<std::string> names,
                                   std::vector<std::unique_ptr<Expression>> values,
                                   const SourcePosition& position)
    : Statement(ASTNodeType::LocalDeclaration, position),
      names_(std::move(names)) {
    for (auto& value : values) {
        AddChild(std::move(value));
    }
}

Size LocalDeclaration::GetValueCount() const {
    return GetChildCount();
}

Expression* LocalDeclaration::GetValue(Size index) const {
    return static_cast<Expression*>(GetChild(index));
}

/* ========================================================================== */
/* 控制流语句实现 */
/* ========================================================================== */

IfStatement::IfStatement(std::unique_ptr<Expression> condition,
                         std::unique_ptr<BlockNode> then_block,
                         std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<BlockNode>>> elseif_clauses,
                         std::unique_ptr<BlockNode> else_block,
                         const SourcePosition& position)
    : Statement(ASTNodeType::IfStatement, position) {
    AddChild(std::move(condition));
    AddChild(std::move(then_block));
    
    // 添加elseif子句
    for (auto& clause : elseif_clauses) {
        AddChild(std::move(clause.first));  // condition
        AddChild(std::move(clause.second)); // block
    }
    elseif_count_ = elseif_clauses.size();
    
    if (else_block) {
        AddChild(std::move(else_block));
        has_else_ = true;
    } else {
        has_else_ = false;
    }
}

Expression* IfStatement::GetCondition() const {
    return static_cast<Expression*>(GetChild(0));
}

BlockNode* IfStatement::GetThenBlock() const {
    return static_cast<BlockNode*>(GetChild(1));
}

Size IfStatement::GetElseifCount() const {
    return elseif_count_;
}

Expression* IfStatement::GetElseifCondition(Size index) const {
    return static_cast<Expression*>(GetChild(2 + index * 2));
}

BlockNode* IfStatement::GetElseifBlock(Size index) const {
    return static_cast<BlockNode*>(GetChild(3 + index * 2));
}

BlockNode* IfStatement::GetElseBlock() const {
    if (!has_else_) return nullptr;
    Size else_index = 2 + elseif_count_ * 2;
    return static_cast<BlockNode*>(GetChild(else_index));
}

WhileStatement::WhileStatement(std::unique_ptr<Expression> condition,
                               std::unique_ptr<BlockNode> body,
                               const SourcePosition& position)
    : Statement(ASTNodeType::WhileStatement, position) {
    AddChild(std::move(condition));
    AddChild(std::move(body));
}

Expression* WhileStatement::GetCondition() const {
    return static_cast<Expression*>(GetChild(0));
}

BlockNode* WhileStatement::GetBody() const {
    return static_cast<BlockNode*>(GetChild(1));
}

RepeatStatement::RepeatStatement(std::unique_ptr<BlockNode> body,
                                 std::unique_ptr<Expression> condition,
                                 const SourcePosition& position)
    : Statement(ASTNodeType::RepeatStatement, position) {
    AddChild(std::move(body));
    AddChild(std::move(condition));
}

BlockNode* RepeatStatement::GetBody() const {
    return static_cast<BlockNode*>(GetChild(0));
}

Expression* RepeatStatement::GetCondition() const {
    return static_cast<Expression*>(GetChild(1));
}

/* ========================================================================== */
/* 循环语句实现 */
/* ========================================================================== */

NumericForStatement::NumericForStatement(const std::string& variable,
                                         std::unique_ptr<Expression> start,
                                         std::unique_ptr<Expression> end,
                                         std::unique_ptr<Expression> step,
                                         std::unique_ptr<BlockNode> body,
                                         const SourcePosition& position)
    : Statement(ASTNodeType::NumericForStatement, position),
      variable_(variable) {
    AddChild(std::move(start));
    AddChild(std::move(end));
    if (step) {
        AddChild(std::move(step));
        has_step_ = true;
    } else {
        has_step_ = false;
    }
    AddChild(std::move(body));
}

Expression* NumericForStatement::GetStart() const {
    return static_cast<Expression*>(GetChild(0));
}

Expression* NumericForStatement::GetEnd() const {
    return static_cast<Expression*>(GetChild(1));
}

Expression* NumericForStatement::GetStep() const {
    return has_step_ ? static_cast<Expression*>(GetChild(2)) : nullptr;
}

BlockNode* NumericForStatement::GetBody() const {
    return static_cast<BlockNode*>(GetChild(has_step_ ? 3 : 2));
}

GenericForStatement::GenericForStatement(std::vector<std::string> variables,
                                         std::vector<std::unique_ptr<Expression>> expressions,
                                         std::unique_ptr<BlockNode> body,
                                         const SourcePosition& position)
    : Statement(ASTNodeType::GenericForStatement, position),
      variables_(std::move(variables)) {
    for (auto& expr : expressions) {
        AddChild(std::move(expr));
    }
    expression_count_ = expressions.size();
    AddChild(std::move(body));
}

Size GenericForStatement::GetExpressionCount() const {
    return expression_count_;
}

Expression* GenericForStatement::GetExpression(Size index) const {
    return static_cast<Expression*>(GetChild(index));
}

BlockNode* GenericForStatement::GetBody() const {
    return static_cast<BlockNode*>(GetChild(expression_count_));
}

BreakStatement::BreakStatement(const SourcePosition& position)
    : Statement(ASTNodeType::BreakStatement, position) {
}

/* ========================================================================== */
/* 函数定义语句实现 */
/* ========================================================================== */

FunctionDefinition::FunctionDefinition(std::unique_ptr<Expression> name,
                                       std::vector<std::string> parameters,
                                       std::unique_ptr<BlockNode> body,
                                       bool is_vararg,
                                       const SourcePosition& position)
    : Statement(ASTNodeType::FunctionDefinition, position),
      parameters_(std::move(parameters)),
      is_vararg_(is_vararg) {
    AddChild(std::move(name));
    AddChild(std::move(body));
}

Expression* FunctionDefinition::GetName() const {
    return static_cast<Expression*>(GetChild(0));
}

BlockNode* FunctionDefinition::GetBody() const {
    return static_cast<BlockNode*>(GetChild(1));
}

LocalFunctionDefinition::LocalFunctionDefinition(const std::string& name,
                                                  std::vector<std::string> parameters,
                                                  std::unique_ptr<BlockNode> body,
                                                  bool is_vararg,
                                                  const SourcePosition& position)
    : Statement(ASTNodeType::LocalFunctionDefinition, position),
      name_(name),
      parameters_(std::move(parameters)),
      is_vararg_(is_vararg) {
    AddChild(std::move(body));
}

BlockNode* LocalFunctionDefinition::GetBody() const {
    return static_cast<BlockNode*>(GetChild(0));
}

ReturnStatement::ReturnStatement(std::vector<std::unique_ptr<Expression>> values,
                                 const SourcePosition& position)
    : Statement(ASTNodeType::ReturnStatement, position) {
    for (auto& value : values) {
        AddChild(std::move(value));
    }
}

Size ReturnStatement::GetValueCount() const {
    return GetChildCount();
}

Expression* ReturnStatement::GetValue(Size index) const {
    return static_cast<Expression*>(GetChild(index));
}

/* ========================================================================== */
/* 其他语句实现 */
/* ========================================================================== */

ExpressionStatement::ExpressionStatement(std::unique_ptr<Expression> expression,
                                         const SourcePosition& position)
    : Statement(ASTNodeType::ExpressionStatement, position) {
    AddChild(std::move(expression));
}

Expression* ExpressionStatement::GetExpression() const {
    return static_cast<Expression*>(GetChild(0));
}

DoStatement::DoStatement(std::unique_ptr<BlockNode> body,
                         const SourcePosition& position)
    : Statement(ASTNodeType::DoStatement, position) {
    AddChild(std::move(body));
}

BlockNode* DoStatement::GetBody() const {
    return static_cast<BlockNode*>(GetChild(0));
}

/* ========================================================================== */
/* 具体toString实现 */
/* ========================================================================== */

std::string NumberLiteral::ToString() const {
    std::ostringstream oss;
    oss << value_;
    return oss.str();
}

std::string StringLiteral::ToString() const {
    return "\"" + value_ + "\"";
}

std::string BinaryExpression::ToString() const {
    std::string op_str;
    switch (operator_) {
        case BinaryOperator::Add: op_str = "+"; break;
        case BinaryOperator::Subtract: op_str = "-"; break;
        case BinaryOperator::Multiply: op_str = "*"; break;
        case BinaryOperator::Divide: op_str = "/"; break;
        case BinaryOperator::Modulo: op_str = "%"; break;
        case BinaryOperator::Power: op_str = "^"; break;
        case BinaryOperator::Equal: op_str = "=="; break;
        case BinaryOperator::NotEqual: op_str = "~="; break;
        case BinaryOperator::Less: op_str = "<"; break;
        case BinaryOperator::LessEqual: op_str = "<="; break;
        case BinaryOperator::Greater: op_str = ">"; break;
        case BinaryOperator::GreaterEqual: op_str = ">="; break;
        case BinaryOperator::And: op_str = "and"; break;
        case BinaryOperator::Or: op_str = "or"; break;
        case BinaryOperator::Concat: op_str = ".."; break;
        default: op_str = "?"; break;
    }
    return "(" + GetLeft()->ToString() + " " + op_str + " " + GetRight()->ToString() + ")";
}

std::string UnaryExpression::ToString() const {
    std::string op_str;
    switch (operator_) {
        case UnaryOperator::Minus: op_str = "-"; break;
        case UnaryOperator::Not: op_str = "not "; break;
        case UnaryOperator::Length: op_str = "#"; break;
        default: op_str = "?"; break;
    }
    return op_str + GetOperand()->ToString();
}

/* ========================================================================== */
/* 程序根节点实现 */
/* ========================================================================== */

Program::Program(const SourcePosition& position)
    : BlockNode(position) {
    type_ = ASTNodeType::Program;
}

} // namespace lua_cpp