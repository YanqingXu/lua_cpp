#pragma once

#include "Lexer.h"
#include "AST.h"
#include "types.h"
#include <memory>
#include <vector>
#include <functional>
#include <stdexcept>

namespace LuaCore {

/**
 * @brief 解析错误异常
 */
class ParseError : public std::runtime_error {
public:
    ParseError(const Str& message, i32 line, i32 column)
        : std::runtime_error(message), m_line(line), m_column(column) {}
    
    i32 getLine() const { return m_line; }
    i32 getColumn() const { return m_column; }
    
private:
    i32 m_line;
    i32 m_column;
};

/**
 * @brief Lua代码的语法分析器
 * 
 * Parser类负责将词法分析器生成的标记序列转换为抽象语法树（AST）。
 * 它实现了Lua语言的递归下降语法分析。
 */
class Parser {
public:
    /**
     * 构造函数
     * @param lexer 词法分析器
     */
    explicit Parser(Lexer& lexer);
    
    /**
     * 解析Lua代码块
     * @return 解析得到的代码块
     */
    Ptr<Block> parse();
    
private:
    Lexer& m_lexer;                 // 词法分析器
    Token m_current;                // 当前标记
    bool m_panicMode = false;       // 错误恢复标志
    
    // 辅助方法
    void advance();                 // 前进到下一个标记
    bool check(TokenType type);     // 检查当前标记类型
    bool match(TokenType type);     // 如果当前标记匹配预期类型，则前进并返回true
    void consume(TokenType type, const Str& message); // 消耗预期的标记，否则报错
    void synchronize();             // 错误恢复：跳过标记直到找到语句边界
    ParseError error(const Str& message); // 创建解析错误
    TokenType peekNext();           // 查看下一个标记类型
    TokenType peekNextNext();       // 查看下下个标记类型
    
    // 递归下降解析方法
    
    // 解析语句
    Ptr<Block> block();
    Ptr<Statement> statement();
    Ptr<Statement> simpleStatement();
    Ptr<Statement> assignOrCallStatement();
    Ptr<Statement> localStatement();
    Ptr<Statement> ifStatement();
    Ptr<Statement> whileStatement();
    Ptr<Statement> doStatement();
    Ptr<Statement> forStatement();
    Ptr<Statement> repeatStatement();
    Ptr<Statement> functionStatement();
    Ptr<Statement> returnStatement();
    Ptr<Statement> breakStatement();
    
    // 解析表达式
    Ptr<Expression> expression();
    Ptr<Expression> orExpr();
    Ptr<Expression> andExpr();
    Ptr<Expression> comparisonExpr();
    Ptr<Expression> concatExpr();
    Ptr<Expression> additiveExpr();
    Ptr<Expression> multiplicativeExpr();
    Ptr<Expression> unaryExpr();
    Ptr<Expression> powerExpr();
    Ptr<Expression> primaryExpr();
    Ptr<Expression> simpleExpr();
    Ptr<Expression> suffixedExpr();
    Ptr<Expression> functionExpr();
    Ptr<Expression> tableConstructorExpr();
    
    // 解析其他组件
    Ptr<ExpressionList> expressionList();
    Vec<Str> nameList();
    Vec<Ptr<Expression>> exprList();
    Vec<Str> paramList(bool& isVararg);
    Ptr<FunctionCallExpr> functionCall(Ptr<Expression> prefix);
    TableConstructorExpr::Field parseTableField();
};

} // namespace LuaCore
