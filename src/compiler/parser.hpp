#pragma once

#include "types.hpp"
#include "lexer.hpp"
#include "compiler/ast.hpp"

#include <memory>
#include <functional>
#include <stdexcept>

namespace Lua {
/**
 * @brief 解析错误异常
 */
class ParseError : public ::std::runtime_error {
public:
    ParseError(const Str& message, int line, int column)
        : ::std::runtime_error(message), m_line(line), m_column(column) {}
    
    int getLine() const { return m_line; }
    int getColumn() const { return m_column; }
    
private:
    int m_line;
    int m_column;
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
    Token m_previous;               // 上一个标记
    
    // 解析助手方法
    void advance();                           // 前进到下一个标记
    Token consume(TokenType type, const Str& message); // 消费一个预期类型的标记
    bool check(TokenType type) const;         // 检查当前标记类型
    bool match(TokenType type);               // 如果当前标记匹配预期类型，消费并返回true
    void error(const Str& message);           // 报告错误
    void synchronize();                       // 错误恢复
    
    // 解析各种语法结构
    Ptr<Block> parseBlock();
    Ptr<Statement> parseStatement();
    
    // 控制结构
    Ptr<IfStmt> parseIfStatement();
    Ptr<WhileStmt> parseWhileStatement();
    Ptr<RepeatStmt> parseRepeatStatement();
    Ptr<NumericForStmt> parseNumericForStatement(const Str& varName);
    Ptr<GenericForStmt> parseGenericForStatement(const Vec<Str>& varNames);
    Ptr<DoStmt> parseDoStatement();
    Ptr<FunctionDeclStmt> parseFunctionDeclaration(bool isLocal);
    
    // 语句
    Ptr<ReturnStmt> parseReturnStatement();
    Ptr<LocalVarDeclStmt> parseLocalVariableDeclaration();
    Ptr<Statement> parseAssignmentOrCallStatement();
    Ptr<BreakStmt> parseBreakStatement();
    
    // 表达式
    Ptr<Expression> parseExpression();
    Ptr<Expression> parseOr();
    Ptr<Expression> parseAnd();
    Ptr<Expression> parseComparison();
    Ptr<Expression> parseConcat();
    Ptr<Expression> parseAdditive();
    Ptr<Expression> parseMultiplicative();
    Ptr<Expression> parseUnary();
    Ptr<Expression> parsePower();
    Ptr<Expression> parsePrimary();
    
    // 复杂表达式
    Ptr<TableConstructorExpr> parseTableConstructor();
    Ptr<FunctionDefExpr> parseFunctionLiteral();
    Ptr<VariableExpr> parseVariable();
    Ptr<FunctionCallExpr> parseCall(Ptr<Expression> callee);
    
    // 辅助方法
    Vec<Ptr<Expression>> parseExpressionList();
    Vec<Str> parseNameList();
    Vec<Ptr<Expression>> parseArgumentList();
};

} // namespace Lua
