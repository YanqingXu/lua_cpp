#pragma once

#include "Value.h"
#include "types.h"
#include <vector>
#include <memory>
#include <utility>

namespace LuaCore {

/**
 * @brief 表示Lua代码中的表达式或语句
 */
class ASTNode {
public:
    virtual ~ASTNode() = default;
};

//============================= 表达式节点 =============================

/**
 * @brief 表达式基类
 */
class Expression : public ASTNode {
public:
    virtual ~Expression() = default;
};

/**
 * @brief 字面量表达式（数字、字符串、布尔值、nil）
 */
class LiteralExpr : public Expression {
public:
    explicit LiteralExpr(const Value& value) : m_value(value) {}
    const Value& getValue() const { return m_value; }
    
private:
    Value m_value;
};

/**
 * @brief 变量引用表达式
 */
class VariableExpr : public Expression {
public:
    explicit VariableExpr(const Str& name) : m_name(name) {}
    const Str& getName() const { return m_name; }
    
private:
    Str m_name;
};

/**
 * @brief 表达式列表，用于函数调用参数、表构造器等
 */
class ExpressionList : public Expression {
public:
    void addExpression(std::shared_ptr<Expression> expr) {
        m_expressions.push_back(std::move(expr));
    }
    
    const std::vector<std::shared_ptr<Expression>>& getExpressions() const {
        return m_expressions;
    }
    
private:
    std::vector<std::shared_ptr<Expression>> m_expressions;
};

/**
 * @brief 一元表达式（-、not、#）
 */
class UnaryExpr : public Expression {
public:
    enum class Op {
        Negate,     // -
        Not,        // not
        Length      // #
    };
    
    UnaryExpr(Op op, std::shared_ptr<Expression> expr)
        : m_op(op), m_expr(std::move(expr)) {}
    
    Op getOp() const { return m_op; }
    const std::shared_ptr<Expression>& getExpression() const { return m_expr; }
    
private:
    Op m_op;
    std::shared_ptr<Expression> m_expr;
};

/**
 * @brief 二元表达式（+、-、*、/、%、^、..、==、~=、<、<=、>、>=、and、or）
 */
class BinaryExpr : public Expression {
public:
    enum class Op {
        Add,        // +
        Subtract,   // -
        Multiply,   // *
        Divide,     // /
        Modulo,     // %
        Power,      // ^
        Concat,     // ..
        Equal,      // ==
        NotEqual,   // ~=
        LessThan,   // <
        LessEqual,  // <=
        GreaterThan,// >
        GreaterEqual,// >=
        And,        // and
        Or          // or
    };
    
    BinaryExpr(Op op, std::shared_ptr<Expression> left, std::shared_ptr<Expression> right)
        : m_op(op), m_left(std::move(left)), m_right(std::move(right)) {}
    
    Op getOp() const { return m_op; }
    const std::shared_ptr<Expression>& getLeft() const { return m_left; }
    const std::shared_ptr<Expression>& getRight() const { return m_right; }
    
private:
    Op m_op;
    std::shared_ptr<Expression> m_left;
    std::shared_ptr<Expression> m_right;
};

/**
 * @brief 表访问表达式（t[k]）
 */
class TableAccessExpr : public Expression {
public:
    TableAccessExpr(std::shared_ptr<Expression> table, std::shared_ptr<Expression> key)
        : m_table(std::move(table)), m_key(std::move(key)) {}
    
    const std::shared_ptr<Expression>& getTable() const { return m_table; }
    const std::shared_ptr<Expression>& getKey() const { return m_key; }
    
private:
    std::shared_ptr<Expression> m_table;
    std::shared_ptr<Expression> m_key;
};

/**
 * @brief 字段访问表达式（t.k）
 */
class FieldAccessExpr : public Expression {
public:
    FieldAccessExpr(std::shared_ptr<Expression> table, const Str& field)
            : m_table(std::move(table)), m_field(field) {}
    const std::shared_ptr<Expression>& getTable() const { return m_table; }
    const Str& getField() const { return m_field; }
    
private:
    std::shared_ptr<Expression> m_table;
    Str m_field;
};

/**
 * @brief 函数调用表达式
 */
class FunctionCallExpr : public Expression {
public:
    FunctionCallExpr(std::shared_ptr<Expression> function, std::shared_ptr<ExpressionList> args)
        : m_function(std::move(function)), m_args(std::move(args)) {}
    
    const std::shared_ptr<Expression>& getFunction() const { return m_function; }
    const std::shared_ptr<ExpressionList>& getArgs() const { return m_args; }
    
private:
    std::shared_ptr<Expression> m_function;
    std::shared_ptr<ExpressionList> m_args;
};

/**
 * @brief 表构造器表达式
 */
class TableConstructorExpr : public Expression {
public:
    // 表字段类型：数组项、键值对
    struct Field {
        std::shared_ptr<Expression> key;   // 如果为空，则是数组项
        std::shared_ptr<Expression> value;
    };
    
    void addField(Field field) {
        m_fields.push_back(std::move(field));
    }
    
    const std::vector<Field>& getFields() const {
        return m_fields;
    }
    
private:
    std::vector<Field> m_fields;
};

/**
 * @brief 函数定义表达式
 */
class FunctionDefExpr : public Expression {
public:
    FunctionDefExpr(
        std::vector<Str> params,
        bool isVararg,
        std::shared_ptr<class Block> body
    ) : m_params(std::move(params)), 
        m_isVararg(isVararg), 
        m_body(std::move(body)) {}
    
    const std::vector<Str>& getParams() const { return m_params; }
    bool isVararg() const { return m_isVararg; }
    const std::shared_ptr<class Block>& getBody() const { return m_body; }
    
private:
    std::vector<Str> m_params;
    bool m_isVararg;
    std::shared_ptr<class Block> m_body;
};

//============================= 语句节点 =============================

/**
 * @brief 语句基类
 */
class Statement : public ASTNode {
public:
    virtual ~Statement() = default;
};

/**
 * @brief 代码块（语句列表）
 */
class Block : public Statement {
public:
    void addStatement(std::shared_ptr<Statement> stmt) {
        m_statements.push_back(std::move(stmt));
    }
    
    const std::vector<std::shared_ptr<Statement>>& getStatements() const {
        return m_statements;
    }
    
private:
    std::vector<std::shared_ptr<Statement>> m_statements;
};

/**
 * @brief 赋值语句
 */
class AssignmentStmt : public Statement {
public:
    AssignmentStmt(
        std::vector<std::shared_ptr<Expression>> vars,
        std::vector<std::shared_ptr<Expression>> values
    ) : m_vars(std::move(vars)), m_values(std::move(values)) {}
    
    const std::vector<std::shared_ptr<Expression>>& getVars() const { return m_vars; }
    const std::vector<std::shared_ptr<Expression>>& getValues() const { return m_values; }
    
private:
    std::vector<std::shared_ptr<Expression>> m_vars;
    std::vector<std::shared_ptr<Expression>> m_values;
};

/**
 * @brief 局部变量声明语句
 */
class LocalVarDeclStmt : public Statement {
public:
    LocalVarDeclStmt(
        std::vector<Str> names,
        std::vector<std::shared_ptr<Expression>> initializers
    ) : m_names(std::move(names)), m_initializers(std::move(initializers)) {}
    
    const std::vector<Str>& getNames() const { return m_names; }
    const std::vector<std::shared_ptr<Expression>>& getInitializers() const { return m_initializers; }
    
private:
    std::vector<Str> m_names;
    std::vector<std::shared_ptr<Expression>> m_initializers;
};

/**
 * @brief 函数调用语句
 */
class FunctionCallStmt : public Statement {
public:
    explicit FunctionCallStmt(std::shared_ptr<FunctionCallExpr> call)
        : m_call(std::move(call)) {}
    
    const std::shared_ptr<FunctionCallExpr>& getCall() const { return m_call; }
    
private:
    std::shared_ptr<FunctionCallExpr> m_call;
};

/**
 * @brief do语句
 */
class DoStmt : public Statement {
public:
    explicit DoStmt(std::shared_ptr<Block> body)
        : m_body(std::move(body)) {}
    
    const std::shared_ptr<Block>& getBody() const { return m_body; }
    
private:
    std::shared_ptr<Block> m_body;
};

/**
 * @brief while语句
 */
class WhileStmt : public Statement {
public:
    WhileStmt(std::shared_ptr<Expression> condition, std::shared_ptr<Block> body)
        : m_condition(std::move(condition)), m_body(std::move(body)) {}
    
    const std::shared_ptr<Expression>& getCondition() const { return m_condition; }
    const std::shared_ptr<Block>& getBody() const { return m_body; }
    
private:
    std::shared_ptr<Expression> m_condition;
    std::shared_ptr<Block> m_body;
};

/**
 * @brief repeat语句
 */
class RepeatStmt : public Statement {
public:
    RepeatStmt(std::shared_ptr<Block> body, std::shared_ptr<Expression> condition)
        : m_body(std::move(body)), m_condition(std::move(condition)) {}
    
    const std::shared_ptr<Block>& getBody() const { return m_body; }
    const std::shared_ptr<Expression>& getCondition() const { return m_condition; }
    
private:
    std::shared_ptr<Block> m_body;
    std::shared_ptr<Expression> m_condition;
};

/**
 * @brief if语句
 */
class IfStmt : public Statement {
public:
    // 条件和对应的代码块
    struct Branch {
        std::shared_ptr<Expression> condition;
        std::shared_ptr<Block> body;
    };
    
    IfStmt(
        std::shared_ptr<Expression> condition,
        std::shared_ptr<Block> thenBranch,
        std::vector<Branch> elseIfBranches,
        std::shared_ptr<Block> elseBranch
    ) : m_mainBranch{std::move(condition), std::move(thenBranch)},
        m_elseIfBranches(std::move(elseIfBranches)),
        m_elseBranch(std::move(elseBranch)) {}
    
    const Branch& getMainBranch() const { return m_mainBranch; }
    const std::vector<Branch>& getElseIfBranches() const { return m_elseIfBranches; }
    const std::shared_ptr<Block>& getElseBranch() const { return m_elseBranch; }
    
private:
    Branch m_mainBranch;
    std::vector<Branch> m_elseIfBranches;
    std::shared_ptr<Block> m_elseBranch;
};

/**
 * @brief 数值for循环语句
 */
class NumericForStmt : public Statement {
public:
    NumericForStmt(
        const Str& var,
        std::shared_ptr<Expression> start,
        std::shared_ptr<Expression> end,
        std::shared_ptr<Expression> step,
        std::shared_ptr<Block> body
    ) : m_var(var),
        m_start(std::move(start)),
        m_end(std::move(end)),
        m_step(std::move(step)),
        m_body(std::move(body)) {}
    
    const Str& getVar() const { return m_var; }
    const std::shared_ptr<Expression>& getStart() const { return m_start; }
    const std::shared_ptr<Expression>& getEnd() const { return m_end; }
    const std::shared_ptr<Expression>& getStep() const { return m_step; }
    const std::shared_ptr<Block>& getBody() const { return m_body; }
    
private:
    Str m_var;
    std::shared_ptr<Expression> m_start;
    std::shared_ptr<Expression> m_end;
    std::shared_ptr<Expression> m_step;
    std::shared_ptr<Block> m_body;
};

/**
 * @brief 通用for循环语句
 */
class GenericForStmt : public Statement {
public:
    GenericForStmt(
        std::vector<Str> vars,
        std::vector<std::shared_ptr<Expression>> iterators,
        std::shared_ptr<Block> body
    ) : m_vars(std::move(vars)),
        m_iterators(std::move(iterators)),
        m_body(std::move(body)) {}
    
    const std::vector<Str>& getVars() const { return m_vars; }
    const std::vector<std::shared_ptr<Expression>>& getIterators() const { return m_iterators; }
    const std::shared_ptr<Block>& getBody() const { return m_body; }
    
private:
    std::vector<Str> m_vars;
    std::vector<std::shared_ptr<Expression>> m_iterators;
    std::shared_ptr<Block> m_body;
};

/**
 * @brief 函数声明语句
 */
class FunctionDeclStmt : public Statement {
public:
    FunctionDeclStmt(
        std::vector<Str> nameComponents, // 例如 "a.b.c" 的 ["a", "b", "c"]
        bool isLocal,
        bool isMethod,
        std::vector<Str> params,
        bool isVararg,
        std::shared_ptr<Block> body
    ) : m_nameComponents(std::move(nameComponents)),
        m_isLocal(isLocal),
        m_isMethod(isMethod),
        m_params(std::move(params)),
        m_isVararg(isVararg),
        m_body(std::move(body)) {}
    
    const std::vector<Str>& getNameComponents() const { return m_nameComponents; }
    bool isLocal() const { return m_isLocal; }
    bool isMethod() const { return m_isMethod; }
    const std::vector<Str>& getParams() const { return m_params; }
    bool isVararg() const { return m_isVararg; }
    const std::shared_ptr<Block>& getBody() const { return m_body; }
    
private:
    std::vector<Str> m_nameComponents;
    bool m_isLocal;
    bool m_isMethod;
    std::vector<Str> m_params;
    bool m_isVararg;
    std::shared_ptr<Block> m_body;
};

/**
 * @brief return语句
 */
class ReturnStmt : public Statement {
public:
    explicit ReturnStmt(std::vector<std::shared_ptr<Expression>> values)
        : m_values(std::move(values)) {}
    
    const std::vector<std::shared_ptr<Expression>>& getValues() const { return m_values; }
    
private:
    std::vector<std::shared_ptr<Expression>> m_values;
};

/**
 * @brief break语句
 */
class BreakStmt : public Statement {
public:
    BreakStmt() = default;
};

} // namespace LuaCore
