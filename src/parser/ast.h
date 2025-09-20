/**
 * @file ast.h
 * @brief Abstract Syntax Tree (AST) 节点定义
 * @description 定义Lua语法树的所有节点类型，实现访问者模式，支持完整的Lua 5.1.5语法
 * @date 2025-09-20
 */

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include "core/lua_common.h"
#include "lexer/token.h"

namespace lua_cpp {

/* ========================================================================== */
/* AST节点类型枚举 */
/* ========================================================================== */

enum class ASTNodeType {
    // 程序根节点
    Program,
    Block,
    
    // 表达式节点
    // 字面量表达式
    NilLiteral,
    BooleanLiteral,
    NumberLiteral,
    StringLiteral,
    VarargLiteral,
    
    // 变量表达式
    Identifier,
    IndexExpression,
    MemberExpression,
    
    // 运算表达式
    BinaryExpression,
    UnaryExpression,
    
    // 调用表达式
    CallExpression,
    MethodCallExpression,
    
    // 表构造表达式
    TableConstructor,
    TableField,
    
    // 函数表达式
    FunctionExpression,
    
    // 语句节点
    // 赋值语句
    AssignmentStatement,
    LocalDeclaration,
    
    // 控制流语句
    IfStatement,
    WhileStatement,
    RepeatStatement,
    
    // 循环语句
    NumericForStatement,
    GenericForStatement,
    BreakStatement,
    
    // 函数语句
    FunctionDefinition,
    LocalFunctionDefinition,
    ReturnStatement,
    
    // 其他语句
    ExpressionStatement,
    DoStatement
};

/* ========================================================================== */
/* 运算符枚举 */
/* ========================================================================== */

enum class BinaryOperator {
    // 算术运算符
    Add,        // +
    Subtract,   // -
    Multiply,   // *
    Divide,     // /
    Modulo,     // %
    Power,      // ^
    
    // 关系运算符
    Equal,        // ==
    NotEqual,     // ~=
    Less,         // <
    LessEqual,    // <=
    Greater,      // >
    GreaterEqual, // >=
    
    // 逻辑运算符
    And,        // and
    Or,         // or
    
    // 字符串运算符
    Concat      // ..
};

enum class UnaryOperator {
    Minus,      // -
    Not,        // not
    Length      // #
};

/* ========================================================================== */
/* 辅助函数 */
/* ========================================================================== */

bool IsArithmeticOperator(BinaryOperator op);
bool IsRelationalOperator(BinaryOperator op);
bool IsLogicalOperator(BinaryOperator op);

/* ========================================================================== */
/* 前向声明 */
/* ========================================================================== */

class ASTNode;
class ASTVisitor;

// 表达式节点
class Expression;
class NilLiteral;
class BooleanLiteral;
class NumberLiteral;
class StringLiteral;
class VarargLiteral;
class Identifier;
class IndexExpression;
class MemberExpression;
class BinaryExpression;
class UnaryExpression;
class CallExpression;
class MethodCallExpression;
class TableConstructor;
class TableField;
class FunctionExpression;

// 语句节点
class Statement;
class BlockNode;
class AssignmentStatement;
class LocalDeclaration;
class IfStatement;
class WhileStatement;
class RepeatStatement;
class NumericForStatement;
class GenericForStatement;
class BreakStatement;
class FunctionDefinition;
class LocalFunctionDefinition;
class ReturnStatement;
class ExpressionStatement;
class DoStatement;

/* ========================================================================== */
/* AST访问者接口 */
/* ========================================================================== */

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // 表达式访问方法
    virtual void Visit(NilLiteral* node) {}
    virtual void Visit(BooleanLiteral* node) {}
    virtual void Visit(NumberLiteral* node) {}
    virtual void Visit(StringLiteral* node) {}
    virtual void Visit(VarargLiteral* node) {}
    virtual void Visit(Identifier* node) {}
    virtual void Visit(IndexExpression* node) {}
    virtual void Visit(MemberExpression* node) {}
    virtual void Visit(BinaryExpression* node) {}
    virtual void Visit(UnaryExpression* node) {}
    virtual void Visit(CallExpression* node) {}
    virtual void Visit(MethodCallExpression* node) {}
    virtual void Visit(TableConstructor* node) {}
    virtual void Visit(TableField* node) {}
    virtual void Visit(FunctionExpression* node) {}

    // 语句访问方法
    virtual void Visit(BlockNode* node) {}
    virtual void Visit(AssignmentStatement* node) {}
    virtual void Visit(LocalDeclaration* node) {}
    virtual void Visit(IfStatement* node) {}
    virtual void Visit(WhileStatement* node) {}
    virtual void Visit(RepeatStatement* node) {}
    virtual void Visit(NumericForStatement* node) {}
    virtual void Visit(GenericForStatement* node) {}
    virtual void Visit(BreakStatement* node) {}
    virtual void Visit(FunctionDefinition* node) {}
    virtual void Visit(LocalFunctionDefinition* node) {}
    virtual void Visit(ReturnStatement* node) {}
    virtual void Visit(ExpressionStatement* node) {}
    virtual void Visit(DoStatement* node) {}
};

/* ========================================================================== */
/* AST节点基类 */
/* ========================================================================== */

class ASTNode {
public:
    explicit ASTNode(ASTNodeType type, const SourcePosition& position = SourcePosition{1, 1});
    virtual ~ASTNode() = default;

    // 基础信息
    ASTNodeType GetType() const { return type_; }
    const SourcePosition& GetPosition() const { return position_; }
    void SetPosition(const SourcePosition& position) { position_ = position; }

    // 父子关系
    ASTNode* GetParent() const { return parent_; }
    void SetParent(ASTNode* parent) { parent_ = parent; }
    
    Size GetChildCount() const { return children_.size(); }
    ASTNode* GetChild(Size index) const;
    void AddChild(std::unique_ptr<ASTNode> child);
    void RemoveChild(Size index);
    void ReplaceChild(Size index, std::unique_ptr<ASTNode> new_child);

    // 类型判断
    virtual bool IsExpression() const { return false; }
    virtual bool IsStatement() const { return false; }
    virtual bool IsLiteral() const { return false; }

    // 访问者模式
    virtual void Accept(ASTVisitor* visitor) = 0;

    // 调试工具
    virtual std::string ToString() const;
    void PrintTree(int indent = 0) const;

protected:
    ASTNodeType type_;
    SourcePosition position_;
    ASTNode* parent_ = nullptr;
    std::vector<std::unique_ptr<ASTNode>> children_;
};

/* ========================================================================== */
/* 表达式基类 */
/* ========================================================================== */

class Expression : public ASTNode {
public:
    explicit Expression(ASTNodeType type, const SourcePosition& position = SourcePosition{1, 1});
    
    bool IsExpression() const override { return true; }
    
    // 表达式的类型信息（用于语义分析）
    virtual bool IsConstant() const { return false; }
    virtual bool HasSideEffects() const { return false; }
};

/* ========================================================================== */
/* 字面量表达式 */
/* ========================================================================== */

class NilLiteral : public Expression {
public:
    explicit NilLiteral(const SourcePosition& position = SourcePosition{1, 1});
    
    bool IsLiteral() const override { return true; }
    bool IsConstant() const override { return true; }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override { return "nil"; }
};

class BooleanLiteral : public Expression {
public:
    explicit BooleanLiteral(bool value, const SourcePosition& position = SourcePosition{1, 1});
    
    bool GetValue() const { return value_; }
    void SetValue(bool value) { value_ = value; }
    
    bool IsLiteral() const override { return true; }
    bool IsConstant() const override { return true; }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override { return value_ ? "true" : "false"; }

private:
    bool value_;
};

class NumberLiteral : public Expression {
public:
    explicit NumberLiteral(double value, const SourcePosition& position = SourcePosition{1, 1});
    
    double GetValue() const { return value_; }
    void SetValue(double value) { value_ = value; }
    
    bool IsLiteral() const override { return true; }
    bool IsConstant() const override { return true; }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    double value_;
};

class StringLiteral : public Expression {
public:
    explicit StringLiteral(const std::string& value, const SourcePosition& position = SourcePosition{1, 1});
    
    const std::string& GetValue() const { return value_; }
    void SetValue(const std::string& value) { value_ = value; }
    
    bool IsLiteral() const override { return true; }
    bool IsConstant() const override { return true; }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::string value_;
};

class VarargLiteral : public Expression {
public:
    explicit VarargLiteral(const SourcePosition& position = SourcePosition{1, 1});
    
    bool IsLiteral() const override { return true; }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override { return "..."; }
};

/* ========================================================================== */
/* 变量表达式 */
/* ========================================================================== */

class Identifier : public Expression {
public:
    explicit Identifier(const std::string& name, const SourcePosition& position = SourcePosition{1, 1});
    
    const std::string& GetName() const { return name_; }
    void SetName(const std::string& name) { name_ = name; }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override { return name_; }

private:
    std::string name_;
};

class IndexExpression : public Expression {
public:
    IndexExpression(std::unique_ptr<Expression> table, std::unique_ptr<Expression> index,
                   const SourcePosition& position = SourcePosition{1, 1});
    
    Expression* GetTableExpression() const { return table_.get(); }
    Expression* GetIndexExpression() const { return index_.get(); }
    
    void SetTableExpression(std::unique_ptr<Expression> table) { table_ = std::move(table); }
    void SetIndexExpression(std::unique_ptr<Expression> index) { index_ = std::move(index); }
    
    bool HasSideEffects() const override;
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::unique_ptr<Expression> table_;
    std::unique_ptr<Expression> index_;
};

class MemberExpression : public Expression {
public:
    MemberExpression(std::unique_ptr<Expression> object, const std::string& member,
                    const SourcePosition& position = SourcePosition{1, 1});
    
    Expression* GetObjectExpression() const { return object_.get(); }
    const std::string& GetMemberName() const { return member_; }
    
    void SetObjectExpression(std::unique_ptr<Expression> object) { object_ = std::move(object); }
    void SetMemberName(const std::string& member) { member_ = member; }
    
    bool HasSideEffects() const override;
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::unique_ptr<Expression> object_;
    std::string member_;
};

/* ========================================================================== */
/* 运算表达式 */
/* ========================================================================== */

class BinaryExpression : public Expression {
public:
    BinaryExpression(BinaryOperator op, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right,
                    const SourcePosition& position = SourcePosition{1, 1});
    
    BinaryOperator GetOperator() const { return operator_; }
    Expression* GetLeftOperand() const { return left_.get(); }
    Expression* GetRightOperand() const { return right_.get(); }
    
    void SetOperator(BinaryOperator op) { operator_ = op; }
    void SetLeftOperand(std::unique_ptr<Expression> left) { left_ = std::move(left); }
    void SetRightOperand(std::unique_ptr<Expression> right) { right_ = std::move(right); }
    
    bool IsConstant() const override;
    bool HasSideEffects() const override;
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    BinaryOperator operator_;
    std::unique_ptr<Expression> left_;
    std::unique_ptr<Expression> right_;
};

class UnaryExpression : public Expression {
public:
    UnaryExpression(UnaryOperator op, std::unique_ptr<Expression> operand,
                   const SourcePosition& position = SourcePosition{1, 1});
    
    UnaryOperator GetOperator() const { return operator_; }
    Expression* GetOperand() const { return operand_.get(); }
    
    void SetOperator(UnaryOperator op) { operator_ = op; }
    void SetOperand(std::unique_ptr<Expression> operand) { operand_ = std::move(operand); }
    
    bool IsConstant() const override;
    bool HasSideEffects() const override;
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    UnaryOperator operator_;
    std::unique_ptr<Expression> operand_;
};

/* ========================================================================== */
/* 调用表达式 */
/* ========================================================================== */

class CallExpression : public Expression {
public:
    explicit CallExpression(std::unique_ptr<Expression> function,
                           const SourcePosition& position = SourcePosition{1, 1});
    
    Expression* GetFunction() const { return function_.get(); }
    Size GetArgumentCount() const { return arguments_.size(); }
    Expression* GetArgument(Size index) const;
    
    void SetFunction(std::unique_ptr<Expression> function) { function_ = std::move(function); }
    void AddArgument(std::unique_ptr<Expression> argument);
    void RemoveArgument(Size index);
    void ReplaceArgument(Size index, std::unique_ptr<Expression> argument);
    
    bool HasSideEffects() const override { return true; }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::unique_ptr<Expression> function_;
    std::vector<std::unique_ptr<Expression>> arguments_;
};

class MethodCallExpression : public Expression {
public:
    MethodCallExpression(std::unique_ptr<Expression> object, const std::string& method,
                        const SourcePosition& position = SourcePosition{1, 1});
    
    Expression* GetObject() const { return object_.get(); }
    const std::string& GetMethodName() const { return method_; }
    Size GetArgumentCount() const { return arguments_.size(); }
    Expression* GetArgument(Size index) const;
    
    void SetObject(std::unique_ptr<Expression> object) { object_ = std::move(object); }
    void SetMethodName(const std::string& method) { method_ = method; }
    void AddArgument(std::unique_ptr<Expression> argument);
    void RemoveArgument(Size index);
    void ReplaceArgument(Size index, std::unique_ptr<Expression> argument);
    
    bool HasSideEffects() const override { return true; }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::unique_ptr<Expression> object_;
    std::string method_;
    std::vector<std::unique_ptr<Expression>> arguments_;
};

/* ========================================================================== */
/* 表构造表达式 */
/* ========================================================================== */

class TableField : public ASTNode {
public:
    // 键值对字段
    TableField(std::unique_ptr<Expression> key, std::unique_ptr<Expression> value,
              const SourcePosition& position = SourcePosition{1, 1});
    
    // 数组字段（只有值）
    explicit TableField(std::unique_ptr<Expression> value,
                       const SourcePosition& position = SourcePosition{1, 1});
    
    Expression* GetKey() const { return key_.get(); }
    Expression* GetValue() const { return value_.get(); }
    
    bool IsArrayField() const { return key_ == nullptr; }
    bool IsKeyValueField() const { return key_ != nullptr; }
    
    void SetKey(std::unique_ptr<Expression> key) { key_ = std::move(key); }
    void SetValue(std::unique_ptr<Expression> value) { value_ = std::move(value); }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::unique_ptr<Expression> key_;   // null表示数组字段
    std::unique_ptr<Expression> value_;
};

class TableConstructor : public Expression {
public:
    explicit TableConstructor(const SourcePosition& position = SourcePosition{1, 1});
    
    Size GetFieldCount() const { return fields_.size(); }
    TableField* GetField(Size index) const;
    
    void AddField(std::unique_ptr<TableField> field);
    void AddField(std::unique_ptr<Expression> value);  // 便利方法：添加数组字段
    void RemoveField(Size index);
    void ReplaceField(Size index, std::unique_ptr<TableField> field);
    
    bool IsEmpty() const { return fields_.empty(); }
    bool HasArrayPart() const;
    bool HasHashPart() const;
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::vector<std::unique_ptr<TableField>> fields_;
};

/* ========================================================================== */
/* 函数表达式 */
/* ========================================================================== */

class FunctionExpression : public Expression {
public:
    explicit FunctionExpression(const SourcePosition& position = SourcePosition{1, 1});
    
    Size GetParameterCount() const { return parameters_.size(); }
    const std::string& GetParameter(Size index) const;
    bool IsVariadic() const { return is_variadic_; }
    BlockNode* GetBody() const { return body_.get(); }
    
    void AddParameter(const std::string& parameter);
    void RemoveParameter(Size index);
    void SetVariadic(bool variadic) { is_variadic_ = variadic; }
    void SetBody(std::unique_ptr<BlockNode> body) { body_ = std::move(body); }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::vector<std::string> parameters_;
    bool is_variadic_ = false;
    std::unique_ptr<BlockNode> body_;
};

/* ========================================================================== */
/* 语句基类 */
/* ========================================================================== */

class Statement : public ASTNode {
public:
    explicit Statement(ASTNodeType type, const SourcePosition& position = SourcePosition{1, 1});
    
    bool IsStatement() const override { return true; }
};

/* ========================================================================== */
/* 块语句 */
/* ========================================================================== */

class BlockNode : public Statement {
public:
    explicit BlockNode(const SourcePosition& position = SourcePosition{1, 1});
    
    Size GetStatementCount() const { return statements_.size(); }
    Statement* GetStatement(Size index) const;
    
    void AddStatement(std::unique_ptr<Statement> statement);
    void RemoveStatement(Size index);
    void ReplaceStatement(Size index, std::unique_ptr<Statement> statement);
    
    bool IsEmpty() const { return statements_.empty(); }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::vector<std::unique_ptr<Statement>> statements_;
};

/* ========================================================================== */
/* 赋值语句 */
/* ========================================================================== */

class AssignmentStatement : public Statement {
public:
    explicit AssignmentStatement(const SourcePosition& position = SourcePosition{1, 1});
    
    Size GetTargetCount() const { return targets_.size(); }
    Size GetValueCount() const { return values_.size(); }
    Expression* GetTarget(Size index) const;
    Expression* GetValue(Size index) const;
    
    void AddTarget(std::unique_ptr<Expression> target);
    void AddValue(std::unique_ptr<Expression> value);
    void RemoveTarget(Size index);
    void RemoveValue(Size index);
    void ReplaceTarget(Size index, std::unique_ptr<Expression> target);
    void ReplaceValue(Size index, std::unique_ptr<Expression> value);
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::vector<std::unique_ptr<Expression>> targets_;
    std::vector<std::unique_ptr<Expression>> values_;
};

class LocalDeclaration : public Statement {
public:
    explicit LocalDeclaration(const SourcePosition& position = SourcePosition{1, 1});
    
    Size GetVariableCount() const { return variables_.size(); }
    Size GetInitializerCount() const { return initializers_.size(); }
    const std::string& GetVariable(Size index) const;
    Expression* GetInitializer(Size index) const;
    
    void AddVariable(const std::string& variable);
    void AddInitializer(std::unique_ptr<Expression> initializer);
    void RemoveVariable(Size index);
    void RemoveInitializer(Size index);
    void ReplaceInitializer(Size index, std::unique_ptr<Expression> initializer);
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::vector<std::string> variables_;
    std::vector<std::unique_ptr<Expression>> initializers_;
};

/* ========================================================================== */
/* 控制流语句 */
/* ========================================================================== */

class IfStatement : public Statement {
public:
    IfStatement(std::unique_ptr<Expression> condition, std::unique_ptr<BlockNode> then_block,
               const SourcePosition& position = SourcePosition{1, 1});
    
    Expression* GetCondition() const { return condition_.get(); }
    BlockNode* GetThenBlock() const { return then_block_.get(); }
    BlockNode* GetElseBlock() const { return else_block_.get(); }
    
    Size GetElseIfCount() const { return else_if_conditions_.size(); }
    Expression* GetElseIfCondition(Size index) const;
    BlockNode* GetElseIfBlock(Size index) const;
    
    void SetCondition(std::unique_ptr<Expression> condition) { condition_ = std::move(condition); }
    void SetThenBlock(std::unique_ptr<BlockNode> then_block) { then_block_ = std::move(then_block); }
    void SetElseBlock(std::unique_ptr<BlockNode> else_block) { else_block_ = std::move(else_block); }
    
    void AddElseIf(std::unique_ptr<Expression> condition, std::unique_ptr<BlockNode> block);
    void RemoveElseIf(Size index);
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<BlockNode> then_block_;
    std::unique_ptr<BlockNode> else_block_;
    std::vector<std::unique_ptr<Expression>> else_if_conditions_;
    std::vector<std::unique_ptr<BlockNode>> else_if_blocks_;
};

class WhileStatement : public Statement {
public:
    WhileStatement(std::unique_ptr<Expression> condition, std::unique_ptr<BlockNode> body,
                  const SourcePosition& position = SourcePosition{1, 1});
    
    Expression* GetCondition() const { return condition_.get(); }
    BlockNode* GetBody() const { return body_.get(); }
    
    void SetCondition(std::unique_ptr<Expression> condition) { condition_ = std::move(condition); }
    void SetBody(std::unique_ptr<BlockNode> body) { body_ = std::move(body); }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<BlockNode> body_;
};

class RepeatStatement : public Statement {
public:
    RepeatStatement(std::unique_ptr<BlockNode> body, std::unique_ptr<Expression> condition,
                   const SourcePosition& position = SourcePosition{1, 1});
    
    BlockNode* GetBody() const { return body_.get(); }
    Expression* GetCondition() const { return condition_.get(); }
    
    void SetBody(std::unique_ptr<BlockNode> body) { body_ = std::move(body); }
    void SetCondition(std::unique_ptr<Expression> condition) { condition_ = std::move(condition); }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::unique_ptr<BlockNode> body_;
    std::unique_ptr<Expression> condition_;
};

/* ========================================================================== */
/* 循环语句 */
/* ========================================================================== */

class NumericForStatement : public Statement {
public:
    NumericForStatement(const std::string& variable,
                       std::unique_ptr<Expression> start,
                       std::unique_ptr<Expression> end,
                       std::unique_ptr<Expression> step,
                       std::unique_ptr<BlockNode> body,
                       const SourcePosition& position = SourcePosition{1, 1});
    
    const std::string& GetVariable() const { return variable_; }
    Expression* GetStart() const { return start_.get(); }
    Expression* GetEnd() const { return end_.get(); }
    Expression* GetStep() const { return step_.get(); }
    BlockNode* GetBody() const { return body_.get(); }
    
    void SetVariable(const std::string& variable) { variable_ = variable; }
    void SetStart(std::unique_ptr<Expression> start) { start_ = std::move(start); }
    void SetEnd(std::unique_ptr<Expression> end) { end_ = std::move(end); }
    void SetStep(std::unique_ptr<Expression> step) { step_ = std::move(step); }
    void SetBody(std::unique_ptr<BlockNode> body) { body_ = std::move(body); }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::string variable_;
    std::unique_ptr<Expression> start_;
    std::unique_ptr<Expression> end_;
    std::unique_ptr<Expression> step_;
    std::unique_ptr<BlockNode> body_;
};

class GenericForStatement : public Statement {
public:
    explicit GenericForStatement(const SourcePosition& position = SourcePosition{1, 1});
    
    Size GetVariableCount() const { return variables_.size(); }
    Size GetIteratorCount() const { return iterators_.size(); }
    const std::string& GetVariable(Size index) const;
    Expression* GetIterator(Size index) const;
    BlockNode* GetBody() const { return body_.get(); }
    
    void AddVariable(const std::string& variable);
    void AddIterator(std::unique_ptr<Expression> iterator);
    void RemoveVariable(Size index);
    void RemoveIterator(Size index);
    void SetBody(std::unique_ptr<BlockNode> body) { body_ = std::move(body); }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::vector<std::string> variables_;
    std::vector<std::unique_ptr<Expression>> iterators_;
    std::unique_ptr<BlockNode> body_;
};

class BreakStatement : public Statement {
public:
    explicit BreakStatement(const SourcePosition& position = SourcePosition{1, 1});
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override { return "break"; }
};

/* ========================================================================== */
/* 函数语句 */
/* ========================================================================== */

class FunctionDefinition : public Statement {
public:
    explicit FunctionDefinition(const std::string& name,
                               const SourcePosition& position = SourcePosition{1, 1});
    
    const std::string& GetName() const { return name_; }
    Size GetParameterCount() const { return parameters_.size(); }
    const std::string& GetParameter(Size index) const;
    bool IsVariadic() const { return is_variadic_; }
    BlockNode* GetBody() const { return body_.get(); }
    
    void SetName(const std::string& name) { name_ = name; }
    void AddParameter(const std::string& parameter);
    void RemoveParameter(Size index);
    void SetVariadic(bool variadic) { is_variadic_ = variadic; }
    void SetBody(std::unique_ptr<BlockNode> body) { body_ = std::move(body); }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::string name_;
    std::vector<std::string> parameters_;
    bool is_variadic_ = false;
    std::unique_ptr<BlockNode> body_;
};

class LocalFunctionDefinition : public Statement {
public:
    explicit LocalFunctionDefinition(const std::string& name,
                                    const SourcePosition& position = SourcePosition{1, 1});
    
    const std::string& GetName() const { return name_; }
    Size GetParameterCount() const { return parameters_.size(); }
    const std::string& GetParameter(Size index) const;
    bool IsVariadic() const { return is_variadic_; }
    BlockNode* GetBody() const { return body_.get(); }
    
    void SetName(const std::string& name) { name_ = name; }
    void AddParameter(const std::string& parameter);
    void RemoveParameter(Size index);
    void SetVariadic(bool variadic) { is_variadic_ = variadic; }
    void SetBody(std::unique_ptr<BlockNode> body) { body_ = std::move(body); }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::string name_;
    std::vector<std::string> parameters_;
    bool is_variadic_ = false;
    std::unique_ptr<BlockNode> body_;
};

class ReturnStatement : public Statement {
public:
    explicit ReturnStatement(const SourcePosition& position = SourcePosition{1, 1});
    
    Size GetValueCount() const { return values_.size(); }
    Expression* GetValue(Size index) const;
    
    void AddValue(std::unique_ptr<Expression> value);
    void RemoveValue(Size index);
    void ReplaceValue(Size index, std::unique_ptr<Expression> value);
    
    bool HasValues() const { return !values_.empty(); }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::vector<std::unique_ptr<Expression>> values_;
};

/* ========================================================================== */
/* 其他语句 */
/* ========================================================================== */

class ExpressionStatement : public Statement {
public:
    explicit ExpressionStatement(std::unique_ptr<Expression> expression = nullptr,
                                const SourcePosition& position = SourcePosition{1, 1});
    
    Expression* GetExpression() const { return expression_.get(); }
    void SetExpression(std::unique_ptr<Expression> expression) { expression_ = std::move(expression); }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::unique_ptr<Expression> expression_;
};

class DoStatement : public Statement {
public:
    explicit DoStatement(std::unique_ptr<BlockNode> body = nullptr,
                        const SourcePosition& position = SourcePosition{1, 1});
    
    BlockNode* GetBody() const { return body_.get(); }
    void SetBody(std::unique_ptr<BlockNode> body) { body_ = std::move(body); }
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override;

private:
    std::unique_ptr<BlockNode> body_;
};

/* ========================================================================== */
/* 程序根节点 */
/* ========================================================================== */

class Program : public BlockNode {
public:
    explicit Program(const SourcePosition& position = SourcePosition{1, 1});
    
    void Accept(ASTVisitor* visitor) override { visitor->Visit(this); }
    std::string ToString() const override { return "Program"; }
};

} // namespace lua_cpp