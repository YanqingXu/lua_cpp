/**
 * @file ast_base.h
 * @brief AST基础类型定义
 * @description 定义编译器所需的AST基础接口和类型
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#pragma once

#include "../core/lua_common.h"
#include <memory>
#include <vector>
#include <string>

namespace lua_cpp {

/* ========================================================================== */
/* AST节点类型枚举 */
/* ========================================================================== */

enum class ASTNodeType {
    Program,
    
    // 表达式
    NilLiteral,
    BooleanLiteral,
    NumberLiteral,
    StringLiteral,
    Variable,
    BinaryExpression,
    UnaryExpression,
    CallExpression,
    IndexExpression,
    MemberExpression,
    TableConstructor,
    
    // 语句
    ExpressionStatement,
    AssignmentStatement,
    LocalStatement,
    IfStatement,
    WhileStatement,
    ForStatement,
    FunctionStatement,
    ReturnStatement,
    BreakStatement,
    BlockStatement
};

enum class ExpressionType {
    NilLiteral,
    BooleanLiteral,
    NumberLiteral,
    StringLiteral,
    Variable,
    BinaryExpression,
    UnaryExpression,
    CallExpression,
    IndexExpression,
    MemberExpression,
    TableConstructor
};

enum class StatementType {
    ExpressionStatement,
    AssignmentStatement,
    LocalStatement,
    IfStatement,
    WhileStatement,
    ForStatement,
    FunctionStatement,
    ReturnStatement,
    BreakStatement,
    BlockStatement
};

enum class BinaryOperator {
    Add, Sub, Mul, Div, Mod, Pow,
    Concat,
    Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual,
    And, Or
};

enum class UnaryOperator {
    Minus, Not, Length
};

/* ========================================================================== */
/* AST基类 */
/* ========================================================================== */

class ASTNode {
public:
    explicit ASTNode(ASTNodeType type) : type_(type) {}
    virtual ~ASTNode() = default;
    
    ASTNodeType GetType() const { return type_; }

private:
    ASTNodeType type_;
};

/* ========================================================================== */
/* 表达式基类 */
/* ========================================================================== */

class Expression : public ASTNode {
public:
    explicit Expression(ExpressionType type) 
        : ASTNode(ASTNodeType::Program), expr_type_(type) {} // 临时使用Program
    
    ExpressionType GetType() const { return expr_type_; }

private:
    ExpressionType expr_type_;
};

/* ========================================================================== */
/* 语句基类 */
/* ========================================================================== */

class Statement : public ASTNode {
public:
    explicit Statement(StatementType type) 
        : ASTNode(ASTNodeType::Program), stmt_type_(type) {} // 临时使用Program
    
    StatementType GetType() const { return stmt_type_; }

private:
    StatementType stmt_type_;
};

/* ========================================================================== */
/* 程序根节点 */
/* ========================================================================== */

class Program : public ASTNode {
public:
    Program() : ASTNode(ASTNodeType::Program) {}
    
    void AddStatement(std::shared_ptr<Statement> stmt) {
        statements_.push_back(stmt);
    }
    
    const std::vector<std::shared_ptr<Statement>>& GetStatements() const {
        return statements_;
    }

private:
    std::vector<std::shared_ptr<Statement>> statements_;
};

/* ========================================================================== */
/* 字面量表达式 */
/* ========================================================================== */

class NilLiteralExpression : public Expression {
public:
    NilLiteralExpression() : Expression(ExpressionType::NilLiteral) {}
};

class BooleanLiteralExpression : public Expression {
public:
    explicit BooleanLiteralExpression(bool value) 
        : Expression(ExpressionType::BooleanLiteral), value_(value) {}
    
    bool GetValue() const { return value_; }

private:
    bool value_;
};

class NumberLiteralExpression : public Expression {
public:
    explicit NumberLiteralExpression(double value) 
        : Expression(ExpressionType::NumberLiteral), value_(value) {}
    
    double GetValue() const { return value_; }

private:
    double value_;
};

class StringLiteralExpression : public Expression {
public:
    explicit StringLiteralExpression(const std::string& value) 
        : Expression(ExpressionType::StringLiteral), value_(value) {}
    
    const std::string& GetValue() const { return value_; }

private:
    std::string value_;
};

/* ========================================================================== */
/* 变量表达式 */
/* ========================================================================== */

class VariableExpression : public Expression {
public:
    explicit VariableExpression(const std::string& name) 
        : Expression(ExpressionType::Variable), name_(name) {}
    
    const std::string& GetName() const { return name_; }

private:
    std::string name_;
};

/* ========================================================================== */
/* 二元表达式 */
/* ========================================================================== */

class BinaryExpression : public Expression {
public:
    BinaryExpression(BinaryOperator op, 
                     std::shared_ptr<Expression> left,
                     std::shared_ptr<Expression> right)
        : Expression(ExpressionType::BinaryExpression), 
          operator_(op), left_(left), right_(right) {}
    
    BinaryOperator GetOperator() const { return operator_; }
    std::shared_ptr<Expression> GetLeft() const { return left_; }
    std::shared_ptr<Expression> GetRight() const { return right_; }

private:
    BinaryOperator operator_;
    std::shared_ptr<Expression> left_;
    std::shared_ptr<Expression> right_;
};

/* ========================================================================== */
/* 一元表达式 */
/* ========================================================================== */

class UnaryExpression : public Expression {
public:
    UnaryExpression(UnaryOperator op, std::shared_ptr<Expression> operand)
        : Expression(ExpressionType::UnaryExpression), 
          operator_(op), operand_(operand) {}
    
    UnaryOperator GetOperator() const { return operator_; }
    std::shared_ptr<Expression> GetOperand() const { return operand_; }

private:
    UnaryOperator operator_;
    std::shared_ptr<Expression> operand_;
};

/* ========================================================================== */
/* 调用表达式 */
/* ========================================================================== */

class CallExpression : public Expression {
public:
    CallExpression(std::shared_ptr<Expression> callee,
                   std::vector<std::shared_ptr<Expression>> args)
        : Expression(ExpressionType::CallExpression), 
          callee_(callee), arguments_(args) {}
    
    std::shared_ptr<Expression> GetCallee() const { return callee_; }
    const std::vector<std::shared_ptr<Expression>>& GetArguments() const { 
        return arguments_; 
    }

private:
    std::shared_ptr<Expression> callee_;
    std::vector<std::shared_ptr<Expression>> arguments_;
};

/* ========================================================================== */
/* 索引表达式 */
/* ========================================================================== */

class IndexExpression : public Expression {
public:
    IndexExpression(std::shared_ptr<Expression> object,
                    std::shared_ptr<Expression> index)
        : Expression(ExpressionType::IndexExpression), 
          object_(object), index_(index) {}
    
    std::shared_ptr<Expression> GetObject() const { return object_; }
    std::shared_ptr<Expression> GetIndex() const { return index_; }

private:
    std::shared_ptr<Expression> object_;
    std::shared_ptr<Expression> index_;
};

/* ========================================================================== */
/* 成员表达式 */
/* ========================================================================== */

class MemberExpression : public Expression {
public:
    MemberExpression(std::shared_ptr<Expression> object,
                     const std::string& property)
        : Expression(ExpressionType::MemberExpression), 
          object_(object), property_(property) {}
    
    std::shared_ptr<Expression> GetObject() const { return object_; }
    const std::string& GetProperty() const { return property_; }

private:
    std::shared_ptr<Expression> object_;
    std::string property_;
};

/* ========================================================================== */
/* 表构造表达式 */
/* ========================================================================== */

struct TableField {
    std::shared_ptr<Expression> key;   // nullptr表示数组部分
    std::shared_ptr<Expression> value;
};

class TableConstructorExpression : public Expression {
public:
    explicit TableConstructorExpression(std::vector<TableField> fields)
        : Expression(ExpressionType::TableConstructor), fields_(fields) {}
    
    const std::vector<TableField>& GetFields() const { return fields_; }

private:
    std::vector<TableField> fields_;
};

/* ========================================================================== */
/* 语句类型 */
/* ========================================================================== */

class ExpressionStatement : public Statement {
public:
    explicit ExpressionStatement(std::shared_ptr<Expression> expr)
        : Statement(StatementType::ExpressionStatement), expression_(expr) {}
    
    std::shared_ptr<Expression> GetExpression() const { return expression_; }

private:
    std::shared_ptr<Expression> expression_;
};

class AssignmentStatement : public Statement {
public:
    AssignmentStatement(std::vector<std::shared_ptr<Expression>> targets,
                        std::vector<std::shared_ptr<Expression>> values)
        : Statement(StatementType::AssignmentStatement), 
          targets_(targets), values_(values) {}
    
    const std::vector<std::shared_ptr<Expression>>& GetTargets() const { 
        return targets_; 
    }
    const std::vector<std::shared_ptr<Expression>>& GetValues() const { 
        return values_; 
    }

private:
    std::vector<std::shared_ptr<Expression>> targets_;
    std::vector<std::shared_ptr<Expression>> values_;
};

class LocalStatement : public Statement {
public:
    LocalStatement(std::vector<std::string> names,
                   std::vector<std::shared_ptr<Expression>> values)
        : Statement(StatementType::LocalStatement), 
          names_(names), values_(values) {}
    
    const std::vector<std::string>& GetNames() const { return names_; }
    const std::vector<std::shared_ptr<Expression>>& GetValues() const { 
        return values_; 
    }

private:
    std::vector<std::string> names_;
    std::vector<std::shared_ptr<Expression>> values_;
};

class IfStatement : public Statement {
public:
    IfStatement(std::shared_ptr<Expression> condition,
                std::shared_ptr<Statement> then_stmt,
                std::shared_ptr<Statement> else_stmt = nullptr)
        : Statement(StatementType::IfStatement), 
          condition_(condition), then_stmt_(then_stmt), else_stmt_(else_stmt) {}
    
    std::shared_ptr<Expression> GetCondition() const { return condition_; }
    std::shared_ptr<Statement> GetThenStatement() const { return then_stmt_; }
    std::shared_ptr<Statement> GetElseStatement() const { return else_stmt_; }

private:
    std::shared_ptr<Expression> condition_;
    std::shared_ptr<Statement> then_stmt_;
    std::shared_ptr<Statement> else_stmt_;
};

class WhileStatement : public Statement {
public:
    WhileStatement(std::shared_ptr<Expression> condition,
                   std::shared_ptr<Statement> body)
        : Statement(StatementType::WhileStatement), 
          condition_(condition), body_(body) {}
    
    std::shared_ptr<Expression> GetCondition() const { return condition_; }
    std::shared_ptr<Statement> GetBody() const { return body_; }

private:
    std::shared_ptr<Expression> condition_;
    std::shared_ptr<Statement> body_;
};

class ForStatement : public Statement {
public:
    // 数值for循环
    ForStatement(const std::string& variable,
                 std::shared_ptr<Expression> init,
                 std::shared_ptr<Expression> limit,
                 std::shared_ptr<Expression> step,
                 std::shared_ptr<Statement> body)
        : Statement(StatementType::ForStatement), 
          variable_(variable), init_(init), limit_(limit), step_(step), body_(body) {}
    
    bool IsNumericFor() const { return init_ != nullptr; }
    const std::string& GetVariable() const { return variable_; }
    std::shared_ptr<Expression> GetInit() const { return init_; }
    std::shared_ptr<Expression> GetLimit() const { return limit_; }
    std::shared_ptr<Expression> GetStep() const { return step_; }
    std::shared_ptr<Statement> GetBody() const { return body_; }

private:
    std::string variable_;
    std::shared_ptr<Expression> init_;
    std::shared_ptr<Expression> limit_;
    std::shared_ptr<Expression> step_;
    std::shared_ptr<Statement> body_;
};

class FunctionStatement : public Statement {
public:
    FunctionStatement(const std::string& name,
                      std::vector<std::string> parameters,
                      std::shared_ptr<Statement> body,
                      bool is_vararg = false)
        : Statement(StatementType::FunctionStatement), 
          name_(name), parameters_(parameters), body_(body), is_vararg_(is_vararg) {}
    
    const std::string& GetName() const { return name_; }
    const std::vector<std::string>& GetParameters() const { return parameters_; }
    std::shared_ptr<Statement> GetBody() const { return body_; }
    bool IsVararg() const { return is_vararg_; }

private:
    std::string name_;
    std::vector<std::string> parameters_;
    std::shared_ptr<Statement> body_;
    bool is_vararg_;
};

class ReturnStatement : public Statement {
public:
    explicit ReturnStatement(std::vector<std::shared_ptr<Expression>> values)
        : Statement(StatementType::ReturnStatement), values_(values) {}
    
    const std::vector<std::shared_ptr<Expression>>& GetValues() const { 
        return values_; 
    }

private:
    std::vector<std::shared_ptr<Expression>> values_;
};

class BreakStatement : public Statement {
public:
    BreakStatement() : Statement(StatementType::BreakStatement) {}
};

class BlockStatement : public Statement {
public:
    explicit BlockStatement(std::vector<std::shared_ptr<Statement>> statements)
        : Statement(StatementType::BlockStatement), statements_(statements) {}
    
    const std::vector<std::shared_ptr<Statement>>& GetStatements() const { 
        return statements_; 
    }

private:
    std::vector<std::shared_ptr<Statement>> statements_;
};

} // namespace lua_cpp