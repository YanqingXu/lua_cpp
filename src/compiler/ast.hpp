#pragma once

#include "types.hpp"
#include "object/value.hpp"

#include <memory>

namespace Lua {

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
 * @brief 字面量表达式基类（数字、字符串、布尔值、nil）
 */
class LiteralExpr : public Expression {
public:
    explicit LiteralExpr(const Object::Value& value) : m_value(value) {}
    const Object::Value& getValue() const { return m_value; }
    
private:
    Object::Value m_value;
};

/**
 * @brief nil字面量表达式
 */
class NilExpr : public Expression {
public:
    NilExpr() = default;
};

/**
 * @brief 布尔字面量表达式
 */
class BoolExpr : public Expression {
public:
    explicit BoolExpr(bool value) : m_value(value) {}
    bool getValue() const { return m_value; }
    
private:
    bool m_value;
};

/**
 * @brief 数字字面量表达式
 */
class NumberExpr : public Expression {
public:
    explicit NumberExpr(double value) : m_value(value) {}
    double getValue() const { return m_value; }
    
private:
    double m_value;
};

/**
 * @brief 字符串字面量表达式
 */
class StringExpr : public Expression {
public:
    explicit StringExpr(const Str& value) : m_value(value) {}
    const Str& getValue() const { return m_value; }
    
private:
    Str m_value;
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
class ExpressionList : public ASTNode {
public:
    void addExpression(Ptr<Expression> expr) {
        m_expressions.push_back(move(expr));
    }
    
    const Vec<Ptr<Expression>>& getExpressions() const {
        return m_expressions;
    }
    
private:
    Vec<Ptr<Expression>> m_expressions;
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
    
    UnaryExpr(Op op, Ptr<Expression> expr)
        : m_op(op), m_expr(move(expr)) {}
    
    Op getOp() const { return m_op; }
    const Ptr<Expression>& getExpression() const { return m_expr; }
    
private:
    Op m_op;
    Ptr<Expression> m_expr;
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
        FloorDivide, // //
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
    
    BinaryExpr(Op op, Ptr<Expression> left, Ptr<Expression> right)
        : m_op(op), m_left(move(left)), m_right(move(right)) {}
    
    Op getOp() const { return m_op; }
    const Ptr<Expression>& getLeft() const { return m_left; }
    const Ptr<Expression>& getRight() const { return m_right; }
    
private:
    Op m_op;
    Ptr<Expression> m_left;
    Ptr<Expression> m_right;
};

/**
 * @brief 表访问表达式（t[k]）
 */
class TableAccessExpr : public Expression {
public:
    TableAccessExpr(Ptr<Expression> table, Ptr<Expression> key)
        : m_table(move(table)), m_key(move(key)) {}
    
    const Ptr<Expression>& getTable() const { return m_table; }
    const Ptr<Expression>& getKey() const { return m_key; }
    
private:
    Ptr<Expression> m_table;
    Ptr<Expression> m_key;
};

/**
 * @brief 字段访问表达式（t.k）
 */
class FieldAccessExpr : public Expression {
public:
    FieldAccessExpr(Ptr<Expression> table, const Str& field)
        : m_table(move(table)), m_field(field) {}
    
    const Ptr<Expression>& getTable() const { return m_table; }
    const Str& getField() const { return m_field; }
    
private:
    Ptr<Expression> m_table;
    Str m_field;
};

/**
 * @brief 函数调用表达式
 */
class FunctionCallExpr : public Expression {
public:
    FunctionCallExpr(Ptr<Expression> function, Ptr<ExpressionList> args)
        : m_function(move(function)), m_args(move(args)) {}
    
    const Ptr<Expression>& getFunction() const { return m_function; }
    const Ptr<ExpressionList>& getArgs() const { return m_args; }
    
private:
    Ptr<Expression> m_function;
    Ptr<ExpressionList> m_args;
};

/**
 * @brief 表构造器表达式
 */
class TableConstructorExpr : public Expression {
public:
    // 表字段类型：数组项、键值对
    struct Field {
        Ptr<Expression> key;   // 如果为空，则是数组项
        Ptr<Expression> value;
    };
    
    void addField(Field field) {
        m_fields.push_back(field);
    }
    
    const Vec<Field>& getFields() const {
        return m_fields;
    }
    
private:
    Vec<Field> m_fields;
};

/**
 * @brief 函数定义表达式
 */
class FunctionDefExpr : public Expression {
public:
    FunctionDefExpr(
        Vec<Str> params,
        bool isVararg,
        Ptr<class Block> body
    ) : m_params(move(params)), 
        m_isVararg(isVararg), 
        m_body(move(body)) {}
    
    const Vec<Str>& getParams() const { return m_params; }
    bool isVararg() const { return m_isVararg; }
    const Ptr<class Block>& getBody() const { return m_body; }
    
private:
    Vec<Str> m_params;
    bool m_isVararg;
    Ptr<class Block> m_body;
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
class Block : public ASTNode {
public:
    void addStatement(Ptr<Statement> stmt) {
        m_statements.push_back(move(stmt));
    }
    
    const Vec<Ptr<Statement>>& getStatements() const {
        return m_statements;
    }
    
private:
    Vec<Ptr<Statement>> m_statements;
};

/**
 * @brief 赋值语句
 */
class AssignmentStmt : public Statement {
public:
    AssignmentStmt(
        Vec<Ptr<Expression>> vars,
        Vec<Ptr<Expression>> values
    ) : m_vars(move(vars)), m_values(move(values)) {}
    
    const Vec<Ptr<Expression>>& getVars() const { return m_vars; }
    const Vec<Ptr<Expression>>& getValues() const { return m_values; }
    
private:
    Vec<Ptr<Expression>> m_vars;
    Vec<Ptr<Expression>> m_values;
};

/**
 * @brief 局部变量声明语句
 */
class LocalVarDeclStmt : public Statement {
public:
    LocalVarDeclStmt(
        Vec<Str> names,
        Vec<Ptr<Expression>> initializers
    ) : m_names(move(names)), m_initializers(move(initializers)) {}
    
    const Vec<Str>& getNames() const { return m_names; }
    const Vec<Ptr<Expression>>& getInitializers() const { return m_initializers; }
    
private:
    Vec<Str> m_names;
    Vec<Ptr<Expression>> m_initializers;
};

/**
 * @brief 函数调用语句
 */
class FunctionCallStmt : public Statement {
public:
    explicit FunctionCallStmt(Ptr<FunctionCallExpr> call)
        : m_call(move(call)) {}
    
    const Ptr<FunctionCallExpr>& getCall() const { return m_call; }
    
private:
    Ptr<FunctionCallExpr> m_call;
};

/**
 * @brief do语句
 */
class DoStmt : public Statement {
public:
    explicit DoStmt(Ptr<Block> body)
        : m_body(move(body)) {}
    
    const Ptr<Block>& getBody() const { return m_body; }
    
private:
    Ptr<Block> m_body;
};

/**
 * @brief while语句
 */
class WhileStmt : public Statement {
public:
    WhileStmt(Ptr<Expression> condition, Ptr<Block> body)
        : m_condition(move(condition)), m_body(move(body)) {}
    
    const Ptr<Expression>& getCondition() const { return m_condition; }
    const Ptr<Block>& getBody() const { return m_body; }
    
private:
    Ptr<Expression> m_condition;
    Ptr<Block> m_body;
};

/**
 * @brief repeat语句
 */
class RepeatStmt : public Statement {
public:
    RepeatStmt(Ptr<Block> body, Ptr<Expression> condition)
        : m_body(move(body)), m_condition(move(condition)) {}
    
    const Ptr<Block>& getBody() const { return m_body; }
    const Ptr<Expression>& getCondition() const { return m_condition; }
    
private:
    Ptr<Block> m_body;
    Ptr<Expression> m_condition;
};

/**
 * @brief if语句
 */
class IfStmt : public Statement {
public:
    // 条件和对应的代码块
    struct Branch {
        Ptr<Expression> condition;
        Ptr<Block> body;
    };
    
    IfStmt(
        Ptr<Expression> condition,
        Ptr<Block> thenBranch,
        Vec<Branch> elseIfBranches,
        Ptr<Block> elseBranch
    ) : m_mainBranch{move(condition), move(thenBranch)},
        m_elseIfBranches(move(elseIfBranches)),
        m_elseBranch(move(elseBranch)) {}
    
    const Branch& getMainBranch() const { return m_mainBranch; }
    const Vec<Branch>& getElseIfBranches() const { return m_elseIfBranches; }
    const Ptr<Block>& getElseBranch() const { return m_elseBranch; }
    
private:
    Branch m_mainBranch;
    Vec<Branch> m_elseIfBranches;
    Ptr<Block> m_elseBranch;
};

/**
 * @brief 数值for循环语句
 */
class NumericForStmt : public Statement {
public:
    NumericForStmt(
        const Str& var,
        Ptr<Expression> start,
        Ptr<Expression> end,
        Ptr<Expression> step,
        Ptr<Block> body
    ) : m_var(var),
        m_start(move(start)),
        m_end(move(end)),
        m_step(move(step)),
        m_body(move(body)) {}
    
    const Str& getVar() const { return m_var; }
    const Ptr<Expression>& getStart() const { return m_start; }
    const Ptr<Expression>& getEnd() const { return m_end; }
    const Ptr<Expression>& getStep() const { return m_step; }
    const Ptr<Block>& getBody() const { return m_body; }
    
private:
    Str m_var;
    Ptr<Expression> m_start;
    Ptr<Expression> m_end;
    Ptr<Expression> m_step;
    Ptr<Block> m_body;
};

/**
 * @brief 通用for循环语句
 */
class GenericForStmt : public Statement {
public:
    GenericForStmt(
        Vec<Str> vars,
        Vec<Ptr<Expression>> iterators,
        Ptr<Block> body
    ) : m_vars(move(vars)),
        m_iterators(move(iterators)),
        m_body(move(body)) {}
    
    const Vec<Str>& getVars() const { return m_vars; }
    const Vec<Ptr<Expression>>& getIterators() const { return m_iterators; }
    const Ptr<Block>& getBody() const { return m_body; }
    
private:
    Vec<Str> m_vars;
    Vec<Ptr<Expression>> m_iterators;
    Ptr<Block> m_body;
};

/**
 * @brief 函数声明语句
 */
class FunctionDeclStmt : public Statement {
public:
    FunctionDeclStmt(
        Vec<Str> nameComponents, // 例如 "a.b.c" 的 ["a", "b", "c"]
        bool isLocal,
        bool isMethod,
        Vec<Str> params,
        bool isVararg,
        Ptr<Block> body
    ) : m_nameComponents(move(nameComponents)),
        m_isLocal(isLocal),
        m_isMethod(isMethod),
        m_params(move(params)),
        m_isVararg(isVararg),
        m_body(move(body)) {}
    
    const Vec<Str>& getNameComponents() const { return m_nameComponents; }
    bool isLocal() const { return m_isLocal; }
    bool isMethod() const { return m_isMethod; }
    const Vec<Str>& getParams() const { return m_params; }
    bool isVararg() const { return m_isVararg; }
    const Ptr<Block>& getBody() const { return m_body; }
    
private:
    Vec<Str> m_nameComponents;
    bool m_isLocal;
    bool m_isMethod;
    Vec<Str> m_params;
    bool m_isVararg;
    Ptr<Block> m_body;
};

/**
 * @brief return语句
 */
class ReturnStmt : public Statement {
public:
    explicit ReturnStmt(Vec<Ptr<Expression>> values)
        : m_values(move(values)) {}
    
    const Vec<Ptr<Expression>>& getValues() const { return m_values; }
    
private:
    Vec<Ptr<Expression>> m_values;
};

/**
 * @brief break语句
 */
class BreakStmt : public Statement {
public:
    BreakStmt() = default;
};

/**
 * @brief 表达式语句（如函数调用等）
 */
class ExpressionStmt : public Statement {
public:
    explicit ExpressionStmt(Ptr<Expression> expr)
        : m_expr(move(expr)) {}
    
    const Ptr<Expression>& getExpression() const { return m_expr; }
    
 private:
    Ptr<Expression> m_expr;
};

} // namespace Lua
